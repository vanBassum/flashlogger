#include "record_log.h"
#include "crc.h"
#include <cstring>

// Key values the record layer keeps for itself; the field layer stores keys
// opaquely and knows nothing of these. They exploit the flash's own states, so
// "empty" is all-ones for the key width — with 4-byte keys that is 0xFFFFFFFF
// and a plain 0xFF is ordinary user data.
static constexpr uint32_t KEY_ERASED = 0x00;  // tombstone: every bit cleared
static constexpr uint32_t KEY_MARKER = 0x01;  // start of a record

static constexpr size_t MAX_VALUE_SIZE = 255;  // value_size is stored in a uint8_t

// NOT STANDARD CRC BEHAVIOUR — read this before touching the CRC code.
//
// Two *stored* CRC values carry extra meaning, and it is easy to skip over:
//
//   all 0xFF  -> the CRC was never written. The record was opened but never
//                closed (power loss, forgotten close): torn, don't hand it out.
//   all 0x00  -> the CRC was deliberately cleared, which is how an edit is
//                recorded (clearing bits is the only rewrite NOR allows). The
//                record was valid when written but has changed since, so its
//                current contents are trusted rather than verified.
//
// This does NOT make those two values illegal CRC outputs, and they are never
// avoided or nudged. The reason it works: integrity is checked by *recomputing*
// the CRC and comparing, so a record whose genuine CRC really is 0xFF.. or 0
// still matches and reads as intact. The meanings above are only consulted when
// recompute and stored *disagree*.

static constexpr uint32_t empty_key(uint8_t key_size)
{
    return key_size >= 4 ? 0xFFFFFFFFu : (1u << (8 * key_size)) - 1u;
}

// Widest CRC that fits the value: 1 -> CRC8, 2-3 -> CRC16, 4+ -> CRC32. Derived
// from valueSize, so nothing extra is stored.
static uint8_t crc_width(uint8_t value_size)
{
    if (value_size == 1) return 1;
    if (value_size < 4)  return 2;
    return 4;
}

// Folds a record's bytes into whichever CRC its width calls for. Kept separate
// from crc.h, which stays pure: the choice of width and what gets covered is
// record-layer policy.
class RecordCrc {
public:
    explicit RecordCrc(uint8_t width) : width_(width) {}

    void update(const uint8_t* data, size_t len)
    {
        if (width_ == 1)      c8_  = crc8_update(c8_, data, len);
        else if (width_ == 2) c16_ = crc16_update(c16_, data, len);
        else                  c32_ = crc32_update(c32_, data, len);
    }

    void finalTo(uint8_t* out) const
    {
        if (width_ == 1) {
            out[0] = crc8_final(c8_);
        } else if (width_ == 2) {
            uint16_t v = crc16_final(c16_);
            out[0] = static_cast<uint8_t>(v);
            out[1] = static_cast<uint8_t>(v >> 8);
        } else {
            uint32_t v = crc32_final(c32_);
            for (uint8_t i = 0; i < 4; i++)
                out[i] = static_cast<uint8_t>(v >> (8 * i));
        }
    }

private:
    uint8_t  width_;
    uint8_t  c8_  = CRC8_INIT;
    uint16_t c16_ = CRC16_INIT;
    uint32_t c32_ = CRC32_INIT;
};

// Streams every data field of the record — keys as well as values, so a
// corrupted key is caught too — through the CRC. Nothing is buffered: one field
// is read at a time and folded in. Stops at whatever ends the record.
static FlashLogError compute_record_crc(FieldStore& store, uint32_t start, uint8_t* out)
{
    RecordCrc crc(crc_width(store.valueSize()));

    for (uint32_t index = start + 1; ; index++) {
        uint32_t      key = 0;
        uint8_t       value[MAX_VALUE_SIZE];
        FlashLogError err = store.read(index, &key, value);
        if (err == FlashLogError::ARG_OUT_OF_BOUNDS)
            break;                       // end of the store ends the record
        if (err != FlashLogError::OK)
            return err;
        if (key == KEY_MARKER || key == KEY_ERASED || key == empty_key(store.keySize()))
            break;

        uint8_t key_bytes[4];
        for (uint8_t i = 0; i < store.keySize(); i++)
            key_bytes[i] = static_cast<uint8_t>(key >> (8 * i));

        crc.update(key_bytes, store.keySize());
        crc.update(value, store.valueSize());
    }

    crc.finalTo(out);
    return FlashLogError::OK;
}


// Is the record at `start` intact? Recomputes and compares — the stored value is
// never assumed. Returns OK when it matches, and when it doesn't, says which of
// the two special stored values explains it. See the reserved-CRC comment above.
static FlashLogError classify_record(FieldStore& store, uint32_t start)
{
    uint32_t      marker = 0;
    uint8_t       stored[MAX_VALUE_SIZE];
    FlashLogError err = store.read(start, &marker, stored);
    if (err != FlashLogError::OK)
        return err;

    uint8_t fresh[MAX_VALUE_SIZE];
    err = compute_record_crc(store, start, fresh);
    if (err != FlashLogError::OK)
        return err;

    uint8_t width = crc_width(store.valueSize());
    if (memcmp(stored, fresh, width) == 0)
        return FlashLogError::OK;

    bool all_ones = true, all_zero = true;
    for (uint8_t i = 0; i < width; i++) {
        if (stored[i] != 0xFF) all_ones = false;
        if (stored[i] != 0x00) all_zero = false;
    }
    if (all_ones)
        return FlashLogError::RECORD_TORN;     // never back-filled
    if (!all_zero)
        return FlashLogError::RECORD_CORRUPT;  // a real CRC, data changed behind it
    return FlashLogError::OK;                  // cleared by an edit: trust it
}

RecordReader::RecordReader(FieldStore& store, uint32_t start)
    : store_(store), start_(start)
{
}

// Walks this record's fields looking for the key, starting after the marker and
// stopping at whatever ends the record. Bounded by the field layer, which
// refuses an index past the end of the store.
FlashLogError RecordReader::read(uint32_t key, void* value_out, size_t value_out_size)
{
    // Refuse up front rather than overrunning the caller's buffer: the field
    // layer always reads a whole value_size worth of bytes.
    if (value_out_size < store_.valueSize())
        return FlashLogError::ARG_INVALID;

    // Torn or corrupt is reported, not papered over: pointing straight at a bad
    // record tells you so. Only next() skips torn ones.
    FlashLogError err = classify_record(store_, start_);
    if (err != FlashLogError::OK)
        return err;

    for (uint32_t index = start_ + 1; ; index++) {
        uint32_t      found = 0;
        FlashLogError err   = store_.read(index, &found, value_out);
        if (err != FlashLogError::OK)
            return err;
        if (found == key)
            return FlashLogError::OK;
        // A marker starts the next record, empty means nothing was ever
        // written, a tombstone both ends this record and is skipped.
        if (found == KEY_MARKER || found == KEY_ERASED
            || found == empty_key(store_.keySize()))
            return FlashLogError::ARG_INVALID;  // not found — placeholder error
    }
}

// Steps to the next record by walking forward to the next marker. A tombstone
// ends a record but is itself skipped, so it doesn't stop the walk; an empty
// field or the end of the store does.
FlashLogError RecordReader::next()
{
    for (uint32_t index = start_ + 1; ; index++) {
        uint32_t      key = 0;
        uint8_t       value[MAX_VALUE_SIZE];
        FlashLogError err = store_.read(index, &key, value);
        if (err == FlashLogError::ARG_OUT_OF_BOUNDS)
            return FlashLogError::END_OF_LOG;
        if (err != FlashLogError::OK)
            return err;

        if (key == KEY_MARKER) {
            // A crash leaves a torn record behind, and anything written after it
            // sits further on — so a torn record is not the end of the log and
            // must not hide what follows. Corrupt records are *not* skipped:
            // silently dropping them would defeat the CRC.
            if (classify_record(store_, index) == FlashLogError::RECORD_TORN)
                continue;
            start_ = index;
            return FlashLogError::OK;
        }
        if (key == empty_key(store_.keySize()))
            return FlashLogError::END_OF_LOG;
    }
}

RecordWriter::RecordWriter(RecordLog* log)
    : log_(log)
{
}

RecordWriter::~RecordWriter() { close(); }

FlashLogError RecordWriter::field(uint32_t key, const void* value, size_t value_size)
{
    if (!log_)
        return FlashLogError::RECORD_ALREADY_OPEN;
    return log_->writeField(key, value, value_size);
}

// Dropping the log makes close() idempotent, so the destructor after an
// explicit close() is a no-op.
FlashLogError RecordWriter::close()
{
    if (!log_)
        return FlashLogError::RECORD_ALREADY_OPEN;
    RecordLog* log = log_;
    log_ = nullptr;
    return log->closeRecord();
}

RecordLog::RecordLog(IFlash& flash)
    : flash_(flash), store_(flash)
{
}

FlashLogError RecordLog::init()
{
    FlashLogError err = store_.init();
    if (err != FlashLogError::OK)
        return err;

    // Work out where writing left off, or a reopened log restarts at 0 and
    // writes over the records already there. The append point is the frontier
    // between written fields and erased ones — structural, so it needs no CRC
    // and works even if the last record was torn.
    next_index_ = 0;
    for (uint32_t index = 0; ; index++) {
        uint32_t key = 0;
        uint8_t  value[MAX_VALUE_SIZE];

        err = store_.read(index, &key, value);
        if (err == FlashLogError::ARG_OUT_OF_BOUNDS)
            break;                                  // the store is full
        if (err != FlashLogError::OK)
            return err;
        if (key == empty_key(store_.keySize()))
            break;                                  // first never-written field

        next_index_ = index + 1;
    }

    return FlashLogError::OK;
}

// A format wipes the store: every sector is erased, then the header written.
FlashLogError RecordLog::format(size_t key_size, size_t value_size)
{
    size_t sector_size = flash_.getSectorSize();
    for (size_t address = 0; address < flash_.getSize(); address += sector_size)
        flash_.erase(static_cast<uint32_t>(address), 0);

    // The cursor has to come back with it. Erasing alone left it past the data
    // that was just wiped, so the next record was written into the middle of an
    // empty store and nothing could find it.
    next_index_ = 0;

    return store_.format(key_size, value_size);
}

// Fields go straight to flash, so only one record can be written at a time:
// a second one has to wait until the first is closed.
RecordWriter RecordLog::createRecord()
{
    if (record_open_)
        return RecordWriter(nullptr);
    record_open_ = true;

    // Marker first, so a torn record is self-identifying and recovery can skip
    // it. Its value is left erased as a placeholder for the CRC, which is
    // back-filled on close.
    uint8_t placeholder[MAX_VALUE_SIZE];
    memset(placeholder, 0xFF, store_.valueSize());
    record_start_ = next_index_;
    store_.write(next_index_++, KEY_MARKER, placeholder);

    return RecordWriter(this);
}

// The first record starts at index 0: format() wipes the store and appends
// begin there. Finding it by scanning is only needed once reclaim can leave
// tombstones ahead of it.
RecordReader RecordLog::firstRecord() { return RecordReader(store_, 0); }

FlashLogError RecordLog::writeField(uint32_t key, const void* value, size_t value_size)
{
    // Refuse up front rather than over-reading the caller's buffer: the field
    // layer always writes a whole value_size worth of bytes, so a short source
    // would put whatever followed it on flash.
    if (value_size < store_.valueSize())
        return FlashLogError::ARG_INVALID;
    if (key == empty_key(store_.keySize()) || key == KEY_ERASED || key == KEY_MARKER)
        return FlashLogError::ARG_INVALID;
    return store_.write(next_index_++, key, value);
}

// Committing = back-filling the marker's value, which was left erased when the
// record opened. Legal on NOR: it only clears bits.
FlashLogError RecordLog::closeRecord()
{
    record_open_ = false;

    // Back-fill last: the CRC is what makes the record durable. Writing over the
    // erased placeholder only clears bits, so NOR allows it.
    uint8_t       stamp[MAX_VALUE_SIZE];
    memset(stamp, 0xFF, store_.valueSize());
    FlashLogError err = compute_record_crc(store_, record_start_, stamp);
    if (err != FlashLogError::OK)
        return err;

    return store_.write(record_start_, KEY_MARKER, stamp);
}

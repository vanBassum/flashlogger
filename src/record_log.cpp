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

// One sector erased ahead of the cursor, one being written, one holding older
// records. Fewer than that and reclaim eats the sector it just left.
static constexpr size_t MIN_SECTORS = 3;

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

// What a key means to the record layer. Every walk over the store asks this, and
// each one cares about a different subset — spelling the cases out makes those
// differences visible instead of hiding them in the order of if-statements.
enum class KeyKind {
    Data,       // ordinary user field
    Marker,     // start of a record
    Tombstone,  // cleared: ends a record and is skipped
    Empty,      // never written
};

// One step round the ring. Every walk is bounded by the ring size, because in a
// ring it can no longer stop by running off the end of the store.
static uint32_t step(uint32_t index, uint32_t total)
{
    return total == 0 ? 0 : (index + 1) % total;
}

static KeyKind classify_key(uint32_t key, uint8_t key_size)
{
    if (key == empty_key(key_size)) return KeyKind::Empty;
    if (key == KEY_MARKER)          return KeyKind::Marker;
    if (key == KEY_ERASED)          return KeyKind::Tombstone;
    return KeyKind::Data;
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
static FlashLogError compute_record_crc(FieldStore& store, uint32_t start,
                                       uint32_t total, uint8_t* out)
{
    RecordCrc crc(crc_width(store.valueSize()));

    uint32_t index = step(start, total);
    for (uint32_t steps = 1; steps < total; steps++, index = step(index, total)) {
        uint32_t      key = 0;
        uint8_t       value[MAX_VALUE_SIZE];
        FlashLogError err = store.read(index, &key, value);
        if (err != FlashLogError::OK)
            return err;
        if (classify_key(key, store.keySize()) != KeyKind::Data)
            break;                       // anything but data ends the record

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
static FlashLogError classify_record(FieldStore& store, uint32_t start, uint32_t total)
{
    uint32_t      marker = 0;
    uint8_t       stored[MAX_VALUE_SIZE];
    FlashLogError err = store.read(start, &marker, stored);
    if (err != FlashLogError::OK)
        return err;

    uint8_t fresh[MAX_VALUE_SIZE];
    err = compute_record_crc(store, start, total, fresh);
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

RecordReader::RecordReader(FieldStore& store, uint32_t start, uint32_t total_fields)
    : store_(store), start_(start), total_(total_fields)
{
}

// Walks this record's fields looking for the key, starting after the marker and
// stopping at whatever ends the record. Bounded by the ring size — a wrapping
// walk can't stop by running off the end of the store any more.
FlashLogError RecordReader::read(uint32_t key, void* value_out, size_t value_out_size)
{
    // Refuse up front rather than overrunning the caller's buffer: the field
    // layer always reads a whole value_size worth of bytes.
    if (value_out_size < store_.valueSize())
        return FlashLogError::ARG_INVALID;

    // Torn or corrupt is reported, not papered over: pointing straight at a bad
    // record tells you so. Only next() skips torn ones.
    FlashLogError err = classify_record(store_, start_, total_);
    if (err != FlashLogError::OK)
        return err;

    uint32_t index = step(start_, total_);
    for (uint32_t steps = 1; steps < total_; steps++, index = step(index, total_)) {
        uint32_t found = 0;
        err = store_.read(index, &found, value_out);
        if (err != FlashLogError::OK)
            return err;
        if (found == key)
            return FlashLogError::OK;
        // The record ended before the key turned up. A user key can never be a
        // reserved value, so a match is always a data field.
        if (classify_key(found, store_.keySize()) != KeyKind::Data)
            return FlashLogError::ARG_INVALID;  // not found — placeholder error
    }
    return FlashLogError::ARG_INVALID;          // walked the whole ring
}

// Steps to the next record by walking forward to the next marker. A tombstone
// ends a record but is itself skipped, so it doesn't stop the walk; an empty
// field or the end of the store does.
FlashLogError RecordReader::next()
{
    uint32_t index = step(start_, total_);
    for (uint32_t steps = 1; steps < total_; steps++, index = step(index, total_)) {
        uint32_t      key = 0;
        uint8_t       value[MAX_VALUE_SIZE];
        FlashLogError err = store_.read(index, &key, value);
        if (err != FlashLogError::OK)
            return err;

        switch (classify_key(key, store_.keySize())) {
        case KeyKind::Marker:
            // A crash leaves a torn record behind, and anything written after it
            // sits further on — so a torn record is not the end of the log and
            // must not hide what follows. Corrupt records are *not* skipped:
            // silently dropping them would defeat the CRC.
            if (classify_record(store_, index, total_) == FlashLogError::RECORD_TORN)
                continue;
            start_ = index;
            return FlashLogError::OK;

        case KeyKind::Empty:
            return FlashLogError::END_OF_LOG;

        case KeyKind::Tombstone:
        case KeyKind::Data:
            break;                       // part of a record we're leaving behind
        }
    }
    return FlashLogError::END_OF_LOG;    // walked the whole ring
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

bool RecordLog::fieldIsEmpty(uint32_t index, bool& ok)
{
    uint32_t key = 0;
    uint8_t  value[MAX_VALUE_SIZE];
    ok = store_.read(index, &key, value) == FlashLogError::OK;
    return ok && classify_key(key, store_.keySize()) == KeyKind::Empty;
}

// An erase stopped by power loss leaves a sector half erased, which puts a second
// hole in the store and makes the append point ambiguous. Such a sector is
// recognisable: fields are written from the start of a sector onwards, so empty
// followed by written *inside one sector* can only mean an unfinished erase.
// Finishing it restores the single-gap shape everything else relies on.
FlashLogError RecordLog::finishInterruptedErase()
{
    uint32_t per   = store_.fieldsPerUnit();
    uint32_t total = totalFields();
    if (per == 0)
        return FlashLogError::OK;

    for (uint32_t base = 0; base < total; base += per) {
        bool saw_empty = false;
        for (uint32_t index = base; index < base + per; index++) {
            bool ok    = false;
            bool empty = fieldIsEmpty(index, ok);
            if (!ok)
                return FlashLogError::FLASH_READ_ERROR;

            if (empty) {
                saw_empty = true;
                continue;
            }
            if (saw_empty) {
                FlashLogError err = store_.clear(base, per);
                if (err != FlashLogError::OK)
                    return err;
                // The erase took this sector's header copy with it.
                err = store_.format(store_.keySize(), store_.valueSize());
                if (err != FlashLogError::OK)
                    return err;
                break;
            }
        }
    }
    return FlashLogError::OK;
}

// The append point is the one place where written data gives way to erased space.
// Taking "the first empty field" instead would pick any hole, which is how an
// interrupted erase used to send the next record into the middle of the log.
FlashLogError RecordLog::findAppendPoint()
{
    uint32_t total = totalFields();
    next_index_ = 0;
    if (total == 0)
        return FlashLogError::OK;

    for (uint32_t index = 0; index < total; index++) {
        bool ok = false;
        bool previous_empty = fieldIsEmpty((index + total - 1) % total, ok);
        if (!ok)
            return FlashLogError::FLASH_READ_ERROR;
        bool empty = fieldIsEmpty(index, ok);
        if (!ok)
            return FlashLogError::FLASH_READ_ERROR;

        if (!previous_empty && empty) {
            next_index_ = index;
            return FlashLogError::OK;
        }
    }

    return FlashLogError::OK;   // nothing written at all, or no space left
}

FlashLogError RecordLog::init()
{
    FlashLogError err = store_.init();
    if (err != FlashLogError::OK)
        return err;

    err = finishInterruptedErase();
    if (err != FlashLogError::OK)
        return err;

    return findAppendPoint();
}

// A format wipes the store: every sector is erased, then the header written.
FlashLogError RecordLog::format(size_t key_size, size_t value_size)
{
    size_t sector_size = flash_.getSectorSize();

    // The ring needs three sectors: one always erased ahead of the cursor, one
    // being written, and one still holding older records. With two, erasing the
    // next sector erases the one just left — which can take the marker of the
    // record being written with it. Checked before the erase, so a refused format
    // leaves the flash alone.
    if (flash_.getSize() / sector_size < MIN_SECTORS)
        return FlashLogError::ARG_INVALID;
    for (size_t address = 0; address < flash_.getSize(); address += sector_size)
        flash_.erase(static_cast<uint32_t>(address), 0);

    // The cursor has to come back with it. Erasing alone left it past the data
    // that was just wiped, so the next record was written into the middle of an
    // empty store and nothing could find it.
    next_index_ = 0;

    return store_.format(key_size, value_size);
}

uint32_t RecordLog::totalFields() const
{
    size_t sectors = flash_.getSize() / flash_.getSectorSize();
    return store_.fieldsPerUnit() * static_cast<uint32_t>(sectors);
}

// The ring keeps one sector erased at all times: on stepping into a sector, the
// *next* one is erased. That erased sector is the gap, and the gap is what makes
// the append point findable after a reboot without storing any metadata. Erasing
// it is also the reclaim — the sector ahead holds the oldest records.
void RecordLog::reclaimAhead()
{
    uint32_t per = store_.fieldsPerUnit();
    if (per == 0 || next_index_ % per != 0)
        return;                          // not stepping into a sector

    uint32_t ahead = (next_index_ + per) % totalFields();

    // Skip the erase if it is already erased — on the first lap every sector is,
    // and erases cost flash life.
    uint32_t key = 0;
    uint8_t  value[MAX_VALUE_SIZE];
    if (store_.read(ahead, &key, value) != FlashLogError::OK)
        return;
    if (classify_key(key, store_.keySize()) == KeyKind::Empty)
        return;

    store_.clear(ahead, per);

    // The erase took that sector's copy of the format header with it, so put it
    // back. Losing power in this gap is survivable: every other sector still
    // holds a copy, and init() will use any of them.
    store_.format(store_.keySize(), store_.valueSize());
}

// Hands out the next field position, wrapping at the end of the store.
uint32_t RecordLog::takeSlot()
{
    uint32_t total = totalFields();
    if (total == 0)
        return next_index_;              // not mounted; the write will say so

    reclaimAhead();
    uint32_t slot = next_index_;
    next_index_ = (next_index_ + 1) % total;
    return slot;
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
    record_start_  = takeSlot();
    record_fields_ = 1;                  // the marker itself
    store_.write(record_start_, KEY_MARKER, placeholder);

    return RecordWriter(this);
}

// The oldest record is not at index 0 once the log has wrapped. It sits just past
// the gap, so the search starts at the append point and walks forward to the first
// marker. That also steps over the orphaned tail a reclaimed record can leave
// behind, since those are data fields with no marker of their own.
RecordReader RecordLog::firstRecord()
{
    uint32_t total = totalFields();
    uint32_t index = next_index_;

    for (uint32_t steps = 0; steps < total; steps++, index = step(index, total)) {
        uint32_t key = 0;
        uint8_t  value[MAX_VALUE_SIZE];
        if (store_.read(index, &key, value) != FlashLogError::OK)
            break;
        if (classify_key(key, store_.keySize()) == KeyKind::Marker)
            return RecordReader(store_, index, total);
    }

    return RecordReader(store_, next_index_, total);   // no records to read
}

FlashLogError RecordLog::writeField(uint32_t key, const void* value, size_t value_size)
{
    // Refuse up front rather than over-reading the caller's buffer: the field
    // layer always writes a whole value_size worth of bytes, so a short source
    // would put whatever followed it on flash.
    if (value_size < store_.valueSize())
        return FlashLogError::ARG_INVALID;
    if (classify_key(key, store_.keySize()) != KeyKind::Data)
        return FlashLogError::ARG_INVALID;   // user fields must not look like framing

    // A record must stay short enough that reclaim never reaches its own marker.
    // The marker's sector is erased once the cursor enters the sector before it,
    // which is at most (ring - one sector) fields after the record started, so
    // staying under that keeps the marker alive for the record's whole life.
    // Without this a record silently laps the ring and overwrites its own start.
    // reach is 0 on an unmounted store; leave the field layer to say so.
    uint32_t reach = totalFields() - store_.fieldsPerUnit();
    if (reach > 0 && record_fields_ + 1 >= reach)
        return FlashLogError::RECORD_TOO_LONG;

    FlashLogError err = store_.write(takeSlot(), key, value);
    if (err == FlashLogError::OK)
        record_fields_++;
    return err;
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
    FlashLogError err = compute_record_crc(store_, record_start_, totalFields(), stamp);
    if (err != FlashLogError::OK)
        return err;

    return store_.write(record_start_, KEY_MARKER, stamp);
}

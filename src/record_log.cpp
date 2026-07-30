#include "record_log.h"
#include <cstring>

// Key values the record layer keeps for itself; the field layer stores keys
// opaquely and knows nothing of these. They exploit the flash's own states, so
// "empty" is all-ones for the key width — with 4-byte keys that is 0xFFFFFFFF
// and a plain 0xFF is ordinary user data.
static constexpr uint32_t KEY_ERASED = 0x00;  // tombstone: every bit cleared
static constexpr uint32_t KEY_MARKER = 0x01;  // start of a record

static constexpr size_t MAX_VALUE_SIZE = 255;  // value_size is stored in a uint8_t

// Temporary commit stamp, written into the marker's value by close(). To be
// replaced by the record's CRC in the next step. Deliberately neither 0xFF
// (means never committed) nor 0x00 (reserved for "deliberately edited").
static constexpr uint8_t COMMIT_STAMP = 0xFE;

static constexpr uint32_t empty_key(uint8_t key_size)
{
    return key_size >= 4 ? 0xFFFFFFFFu : (1u << (8 * key_size)) - 1u;
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

    // The marker's value is the commit stamp. Still erased means close() never
    // ran, so this record is torn and must not be handed out.
    uint32_t      marker = 0;
    uint8_t       stamp[MAX_VALUE_SIZE];
    FlashLogError err = store_.read(start_, &marker, stamp);
    if (err != FlashLogError::OK)
        return err;
    bool committed = false;
    for (uint8_t i = 0; i < store_.valueSize(); i++)
        if (stamp[i] != 0xFF)
            committed = true;
    if (!committed)
        return FlashLogError::RECORD_TORN;

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

FlashLogError RecordLog::init() { return store_.init(); }

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

    uint8_t stamp[MAX_VALUE_SIZE];
    memset(stamp, COMMIT_STAMP, store_.valueSize());
    return store_.write(record_start_, KEY_MARKER, stamp);
}

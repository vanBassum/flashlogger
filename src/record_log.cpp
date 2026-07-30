#include "record_log.h"
#include <cstring>

// Key values the record layer keeps for itself; the field layer stores keys
// opaquely and knows nothing of these. They exploit the flash's own states, so
// "empty" is all-ones for the key width — with 4-byte keys that is 0xFFFFFFFF
// and a plain 0xFF is ordinary user data.
static constexpr uint32_t KEY_ERASED = 0x00;  // tombstone: every bit cleared
static constexpr uint32_t KEY_MARKER = 0x01;  // start of a record

static constexpr size_t MAX_VALUE_SIZE = 255;  // value_size is stored in a uint8_t

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
FlashLogError RecordReader::read(uint32_t key, void* value_out)
{
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

FlashLogError RecordWriter::field(uint32_t key, const void* value)
{
    if (!log_)
        return FlashLogError::RECORD_ALREADY_OPEN;
    return log_->writeField(key, value);
}

// Dropping the log makes close() idempotent, so the destructor after an
// explicit close() is a no-op.
FlashLogError RecordWriter::close()
{
    if (!log_)
        return FlashLogError::RECORD_ALREADY_OPEN;
    log_->closeRecord();
    log_ = nullptr;
    return FlashLogError::OK;
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
    store_.write(next_index_++, KEY_MARKER, placeholder);

    return RecordWriter(this);
}

// The first record starts at index 0: format() wipes the store and appends
// begin there. Finding it by scanning is only needed once reclaim can leave
// tombstones ahead of it.
RecordReader RecordLog::firstRecord() { return RecordReader(store_, 0); }

FlashLogError RecordLog::writeField(uint32_t key, const void* value)
{
    if (key == empty_key(store_.keySize()) || key == KEY_ERASED || key == KEY_MARKER)
        return FlashLogError::ARG_INVALID;
    return store_.write(next_index_++, key, value);
}

void RecordLog::closeRecord() { record_open_ = false; }

#pragma once

#include "field_store.h"
#include "flashlog_error.h"

// Key values the record layer keeps for itself; the field layer stores keys
// opaquely and knows nothing of these. They exploit the flash's own states, so
// "empty" is all-ones for the key width — with 4-byte keys that is 0xFFFFFFFF
// and a plain 0xFF is ordinary user data.
static constexpr uint32_t KEY_ERASED = 0x00;  // tombstone: every bit cleared
static constexpr uint32_t KEY_MARKER = 0x01;  // start of a record

static constexpr uint32_t empty_key(uint8_t key_size)
{
    return key_size >= 4 ? 0xFFFFFFFFu : (1u << (8 * key_size)) - 1u;
}

class RecordLog;

// Handle for the record currently being written. Name provisional.
// Holds no cursor of its own — the log owns the append position.
class RecordWriter {
public:
    // A null log means the handle was refused: a record was already open.
    RecordWriter(RecordLog* log) : log_(log) {}
    ~RecordWriter();

    FlashLogError field(uint32_t key, const void* value);
    FlashLogError close();

private:
    RecordLog* log_;
};

class RecordLog {
public:
    RecordLog(IFlash& flash) : flash_(flash), store_(flash) {}

    FlashLogError init() { return store_.init(); }

    // Fields go straight to flash, so only one record can be written at a time:
    // a second one has to wait until the first is closed.
    RecordWriter createRecord()
    {
        if (record_open_)
            return RecordWriter(nullptr);
        record_open_ = true;
        return RecordWriter(this);
    }

    // A format wipes the store: every sector is erased, then the header written.
    FlashLogError format(size_t key_size, size_t value_size)
    {
        size_t sector_size = flash_.getSectorSize();
        for (size_t address = 0; address < flash_.getSize(); address += sector_size)
            flash_.erase(static_cast<uint32_t>(address), 0);
        return store_.format(key_size, value_size);
    }

private:
    friend class RecordWriter;

    FlashLogError writeField(uint32_t key, const void* value)
    {
        if (key == empty_key(store_.keySize()) || key == KEY_ERASED || key == KEY_MARKER)
            return FlashLogError::ARG_INVALID;
        return store_.write(next_index_++, key, value);
    }

    void closeRecord() { record_open_ = false; }

    IFlash&    flash_;
    FieldStore store_;
    bool       record_open_ = false;
    uint32_t   next_index_  = 0;
};

inline RecordWriter::~RecordWriter() { close(); }

inline FlashLogError RecordWriter::field(uint32_t key, const void* value)
{
    if (!log_)
        return FlashLogError::RECORD_ALREADY_OPEN;
    return log_->writeField(key, value);
}

// Dropping the log makes close() idempotent, so the destructor after an
// explicit close() is a no-op.
inline FlashLogError RecordWriter::close()
{
    if (!log_)
        return FlashLogError::RECORD_ALREADY_OPEN;
    log_->closeRecord();
    log_ = nullptr;
    return FlashLogError::OK;
}

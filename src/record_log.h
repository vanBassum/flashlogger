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

// Handle for the record currently being written. Name provisional.
class RecordWriter {
public:
    RecordWriter(FieldStore& store, bool open) : store_(store), open_(open) {}

    FlashLogError field(uint32_t key, const void* value)
    {
        if (!open_)
            return FlashLogError::RECORD_ALREADY_OPEN;
        if (key == empty_key(store_.keySize()) || key == KEY_ERASED || key == KEY_MARKER)
            return FlashLogError::ARG_INVALID;
        return store_.write(next_index_++, key, value);
    }

private:
    FieldStore& store_;
    bool        open_;
    uint32_t    next_index_ = 0;
};

class RecordLog {
public:
    RecordLog(IFlash& flash) : flash_(flash), store_(flash) {}

    FlashLogError init() { return store_.init(); }

    // Fields go straight to flash, so only one record can be written at a time:
    // a second one has to wait until the first is closed.
    RecordWriter WriteRecord()
    {
        if (record_open_)
            return RecordWriter(store_, false);
        record_open_ = true;
        return RecordWriter(store_, true);
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
    IFlash&    flash_;
    FieldStore store_;
    bool       record_open_ = false;
};

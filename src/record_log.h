#pragma once

#include "field_store.h"
#include "flashlog_error.h"

// Handle for the record currently being written. Name provisional.
class RecordWriter {
public:
    RecordWriter(FieldStore& store) : store_(store) {}

    FlashLogError field(uint32_t key, const void* value)
    {
        return store_.write(next_index_++, key, value);
    }

private:
    FieldStore& store_;
    uint32_t    next_index_ = 0;
};

class RecordLog {
public:
    RecordLog(IFlash& flash) : flash_(flash), store_(flash) {}

    FlashLogError init() { return store_.init(); }

    RecordWriter WriteRecord() { return RecordWriter(store_); }

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
};

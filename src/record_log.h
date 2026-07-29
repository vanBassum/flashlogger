#pragma once

#include "field_store.h"
#include "flashlog_error.h"

class RecordLog {
public:
    RecordLog(IFlash& flash) : flash_(flash), store_(flash) {}

    FlashLogError init() { return store_.init(); }

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

#pragma once

#include "field_store.h"
#include "flashlog_error.h"

class RecordLog {
public:
    RecordLog(IFlash& flash) : store_(flash) {}

    FlashLogError init() { return store_.init(); }
    FlashLogError format(size_t key_size, size_t value_size) { return store_.format(key_size, value_size); }

private:
    FieldStore store_;
};

#pragma once

#include "field_store.h"
#include "flashlog_error.h"

class RecordLog {
public:
    RecordLog(FieldStore& store) : store_(store) {}

    FlashLogError init() { return store_.init(); }

private:
    FieldStore& store_;
};

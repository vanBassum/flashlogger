#pragma once

#include <cstddef>
#include <cstdint>
#include "field_store.h"
#include "flashlog_error.h"

class RecordLog;

// Handle for reading a record. Separate from RecordWriter: reading never
// mutates the append cursor, and a record may be read long after it was closed.
class RecordReader {
public:
    RecordReader(FieldStore& store, uint32_t start);

    FlashLogError read(uint32_t key, void* value_out, size_t value_out_size);
    FlashLogError next();

private:
    FieldStore& store_;
    uint32_t    start_;
};

// Handle for the record currently being written.
// Holds no cursor of its own — the log owns the append position.
class RecordWriter {
public:
    // A null log means the handle was refused: a record was already open.
    RecordWriter(RecordLog* log);
    ~RecordWriter();

    FlashLogError field(uint32_t key, const void* value, size_t value_size);
    FlashLogError close();

private:
    RecordLog* log_;
};

class RecordLog {
public:
    RecordLog(IFlash& flash);

    FlashLogError init();
    FlashLogError format(size_t key_size, size_t value_size);

    RecordWriter createRecord();
    RecordReader firstRecord();

private:
    friend class RecordWriter;

    FlashLogError writeField(uint32_t key, const void* value, size_t value_size);
    FlashLogError closeRecord();

    IFlash&    flash_;
    FieldStore store_;
    bool       record_open_  = false;
    uint32_t   next_index_   = 0;
    uint32_t   record_start_ = 0;   // index of the open record's marker field
};

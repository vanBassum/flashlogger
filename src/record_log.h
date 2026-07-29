#pragma once

#include <cstdint>
#include <cstring>
#include "field_store.h"

// Record layer: records (1+ fields) appended over a FieldStore. A record is a
// marker header field followed by its data fields.
class RecordLog {
public:
    static constexpr uint32_t MARKER = 0x01;  // reserved key: record-start header

    class Iterator {
    public:
        bool valid() const { return fields_ != nullptr; }

        // Read data field #index of this record (the header is not a data field).
        FlashLogError getField(uint32_t index, void* key_out, void* value_out) const {
            return fields_->read(start_ + 1 + index, key_out, value_out);
        }

    private:
        friend class RecordLog;
        FieldStore* fields_ = nullptr;
        uint32_t    start_  = 0;   // field index of this record's header
    };

    explicit RecordLog(FieldStore& fields) : fields_(fields) {}

    // Marker header written first (claims the record start; back-fill of the
    // CRC comes later), then data fields appended after it.
    void beginRecord() {
        uint8_t placeholder[256];
        std::memset(placeholder, 0xFF, sizeof(placeholder));
        fields_.write(cursor_, MARKER, placeholder);
        cursor_++;
    }

    void field(uint32_t key, const void* value) {
        fields_.write(cursor_, key, value);
        cursor_++;
    }

    void finishRecord() {}  // CRC back-fill comes later

    Iterator first() {
        Iterator it;
        uint32_t key = 0;
        uint8_t  value[256];
        if (fields_.read(0, &key, value) == FlashLogError::OK && key == MARKER) {
            it.fields_ = &fields_;
            it.start_  = 0;
        }
        return it;
    }

private:
    FieldStore& fields_;
    uint32_t    cursor_ = 0;
};

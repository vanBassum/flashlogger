#pragma once

#include <cstddef>
#include <cstdint>
#include "iflash.h"
#include "flashlog_error.h"

class FieldStore {
public:
    FieldStore(IFlash& flash);

    FlashLogError init();
    FlashLogError format(size_t key_size, size_t value_size);
    FlashLogError append(uint32_t key, const void* value);
    uint8_t key_size() const;
    uint8_t value_size() const;

private:
    IFlash&  flash_;
    bool     initialized_ = false;
    uint8_t  key_size_    = 0;
    uint8_t  value_size_  = 0;
    uint32_t next_index_  = 0;
};

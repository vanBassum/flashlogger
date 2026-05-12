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

private:
    IFlash& flash_;
    uint8_t key_size_  = 0;
    uint8_t value_size_ = 0;
};

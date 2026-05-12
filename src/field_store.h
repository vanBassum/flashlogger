#pragma once

#include <cstddef>
#include "iflash.h"
#include "flashlog_error.h"

class FieldStore {
public:
    FieldStore(IFlash& flash);

    FlashLogError init();
    FlashLogError format(size_t key_size, size_t value_size);

private:
    IFlash& flash_;
};

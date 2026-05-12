#pragma once

#include <cstddef>
#include "iflash.h"

class FieldStore {
public:
    FieldStore(IFlash& flash, size_t key_size, size_t value_size);

private:
    IFlash& flash_;
    size_t key_size_;
    size_t value_size_;
};

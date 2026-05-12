#pragma once

#include <cstddef>
#include "iflash.h"

enum class FieldStoreError {
    OK,
    READ_ERROR,
    NOT_FORMATTED,
    WRITE_ERROR,
};

class FieldStore {
public:
    FieldStore(IFlash& flash);

    FieldStoreError init();
    FieldStoreError format(size_t key_size, size_t value_size);

private:
    IFlash& flash_;
};

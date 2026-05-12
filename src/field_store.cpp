#include "field_store.h"

FieldStore::FieldStore(IFlash& flash)
    : flash_(flash)
{
}

FlashLogError FieldStore::init()
{
    return FlashLogError::NOT_FORMATTED;
}

FlashLogError FieldStore::format(size_t key_size, size_t value_size)
{
    return FlashLogError::OK;
}

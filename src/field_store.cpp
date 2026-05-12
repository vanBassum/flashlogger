#include "field_store.h"

FieldStore::FieldStore(IFlash& flash)
    : flash_(flash)
{
}

FieldStoreError FieldStore::init()
{
    return FieldStoreError::NOT_FORMATTED;
}

FieldStoreError FieldStore::format(size_t key_size, size_t value_size)
{
    return FieldStoreError::OK;
}

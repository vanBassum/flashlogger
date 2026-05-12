#include "field_store.h"

FieldStore::FieldStore(IFlash& flash, size_t key_size, size_t value_size)
    : flash_(flash), key_size_(key_size), value_size_(value_size)
{
}

#include "field_store.h"

FieldStore::FieldStore(IFlash& flash)
    : flash_(flash)
{
}

#pragma once

#include "iflash.h"

class FieldStore {
public:
    FieldStore(IFlash& flash);

private:
    IFlash& flash_;
};

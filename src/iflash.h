#pragma once

#include <cstddef>
#include <cstdint>

#ifndef FLASHLOGGER_TIMEOUT_T
#define FLASHLOGGER_TIMEOUT_T uint32_t
#endif

using timeout_t = FLASHLOGGER_TIMEOUT_T;

class IFlash {
public:
    virtual ~IFlash() = default;

    virtual size_t read(uint32_t address, void* buf, size_t size, timeout_t timeout) = 0;
    virtual size_t write(uint32_t address, const void* buf, size_t size, timeout_t timeout) = 0;
    virtual void erase(uint32_t address, timeout_t timeout) = 0;

    virtual size_t getSectorSize() const = 0;
    virtual size_t getSize() const = 0;
};

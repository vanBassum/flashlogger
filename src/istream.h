#pragma once

#include <cstddef>
#include <cstdint>

#ifndef FLASHLOGGER_TIMEOUT_T
#define FLASHLOGGER_TIMEOUT_T uint32_t
#endif

using timeout_t = FLASHLOGGER_TIMEOUT_T;

class IStream {
public:
    virtual ~IStream() = default;
    virtual size_t read(void* buf, size_t size, timeout_t timeout) = 0;
    virtual size_t write(const void* buf, size_t size, timeout_t timeout) = 0;
    virtual void flush() = 0;
};

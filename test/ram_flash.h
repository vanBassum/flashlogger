#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include "../src/iflash.h"

template<size_t TotalSize, size_t SectorSize>
class RamFlash : public IFlash {
public:
    RamFlash() {
        memset(buf_, 0xFF, TotalSize);
    }

    size_t read(uint32_t address, void* buf, size_t size, timeout_t timeout) override {
        assert(address + size <= TotalSize);
        memcpy(buf, buf_ + address, size);
        return size;
    }

    size_t write(uint32_t address, const void* buf, size_t size, timeout_t timeout) override {
        assert(address + size <= TotalSize);
        const uint8_t* src = static_cast<const uint8_t*>(buf);
        for (size_t i = 0; i < size; i++) {
            assert((~buf_[address + i] & src[i]) == 0); // no 0→1 transitions
            buf_[address + i] &= src[i];
        }
        return size;
    }

    void erase(uint32_t address, timeout_t timeout) override {
        assert(address % SectorSize == 0);
        assert(address + SectorSize <= TotalSize);
        memset(buf_ + address, 0xFF, SectorSize);
    }

    size_t getSectorSize() const override { return SectorSize; }
    size_t getSize() const override { return TotalSize; }

private:
    uint8_t buf_[TotalSize];
};

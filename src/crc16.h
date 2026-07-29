#pragma once

#include <cstddef>
#include <cstdint>

inline uint16_t crc16_init() { return 0xFFFF; }

inline uint16_t crc16_update(uint16_t crc, const uint8_t* data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
        }
    }
    return crc;
}

// One-shot convenience wrapper (unchanged output).
inline uint16_t crc16(const uint8_t* data, size_t len)
{
    return crc16_update(crc16_init(), data, len);
}

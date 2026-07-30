#pragma once

#include <cstddef>
#include <cstdint>

// Pure CRC primitives — no policy, no reserved-value handling. Split into
// update/final so a CRC can be folded across data that isn't contiguous in
// memory (a record's fields are spread over flash), without buffering it.
//
// All three are published standards, so a flash dump can be checked against
// any off-the-shelf tool.

// CRC-8/AUTOSAR: poly 0x2F, init 0xFF, xorout 0xFF, MSB-first. Chosen over the
// more common 0x07 because it detects more errors on short messages.
static constexpr uint8_t CRC8_INIT = 0xFF;

inline uint8_t crc8_update(uint8_t crc, const uint8_t* data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++)
            crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x2F)
                               : static_cast<uint8_t>(crc << 1);
    }
    return crc;
}

inline uint8_t crc8_final(uint8_t crc) { return static_cast<uint8_t>(crc ^ 0xFF); }

// CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no xorout, MSB-first.
static constexpr uint16_t CRC16_INIT = 0xFFFF;

inline uint16_t crc16_update(uint16_t crc, const uint8_t* data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int bit = 0; bit < 8; bit++)
            crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                                 : static_cast<uint16_t>(crc << 1);
    }
    return crc;
}

inline uint16_t crc16_final(uint16_t crc) { return crc; }

// CRC-32 (IEEE 802.3 / zlib): poly 0xEDB88320 reflected, init and xorout
// 0xFFFFFFFF.
static constexpr uint32_t CRC32_INIT = 0xFFFFFFFFu;

inline uint32_t crc32_update(uint32_t crc, const uint8_t* data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++)
            crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
    }
    return crc;
}

inline uint32_t crc32_final(uint32_t crc) { return crc ^ 0xFFFFFFFFu; }

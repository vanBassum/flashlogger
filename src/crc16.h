#pragma once

#include "crc.h"

// One-shot CRC16 over a contiguous buffer — what the field-layer header needs.
inline uint16_t crc16(const uint8_t* data, size_t len)
{
    return crc16_final(crc16_update(CRC16_INIT, data, len));
}

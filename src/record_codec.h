#pragma once

#include <cstdint>

// Reserved key values interpreted by the record layer (the field layer stores
// keys opaquely and is unaware of these).
static constexpr uint32_t RECORD_MARKER = 0x01;  // record-start header field

// Classification of a field, by its key, during a record-layer scan.
enum class FieldKind {
    Data,       // user key/value
    Empty,      // key all 1s (0xFF…) — never written / erased
    Tombstone,  // key all 0s (0x00…) — deleted
    Header,     // key == record-start marker
};

inline FieldKind classifyField(uint32_t key, uint8_t key_size) {
    uint32_t empty = (key_size >= 4) ? 0xFFFFFFFFu
                                     : ((1u << (key_size * 8)) - 1u);
    if (key == empty)         return FieldKind::Empty;
    if (key == 0x00)          return FieldKind::Tombstone;
    if (key == RECORD_MARKER) return FieldKind::Header;
    return FieldKind::Data;
}

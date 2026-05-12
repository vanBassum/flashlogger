#include "field_store.h"
#include "crc16.h"
#include <cstring>

static constexpr uint32_t MAGIC = 0x464C4F47;

#pragma pack(push, 1)
struct Header {
    uint32_t magic;
    uint8_t  key_size;
    uint8_t  value_size;
    uint16_t crc;
};
#pragma pack(pop)
static_assert(sizeof(Header) == 8, "Header must be 8 bytes");

FieldStore::FieldStore(IFlash& flash)
    : flash_(flash)
{
}

FlashLogError FieldStore::format(size_t key_size, size_t value_size)
{
    Header h;
    h.magic      = MAGIC;
    h.key_size   = static_cast<uint8_t>(key_size);
    h.value_size = static_cast<uint8_t>(value_size);
    h.crc        = crc16(reinterpret_cast<const uint8_t*>(&h), offsetof(Header, crc));

    size_t written = flash_.write(0, &h, sizeof(h), 0);
    return written == sizeof(h) ? FlashLogError::OK : FlashLogError::WRITE_ERROR;
}

FlashLogError FieldStore::init()
{
    Header h;
    size_t read = flash_.read(0, &h, sizeof(h), 0);
    if (read != sizeof(h))
        return FlashLogError::READ_ERROR;

    if (h.magic != MAGIC)
        return FlashLogError::NOT_FORMATTED;

    uint16_t expected = crc16(reinterpret_cast<const uint8_t*>(&h), offsetof(Header, crc));
    if (h.crc != expected)
        return FlashLogError::NOT_FORMATTED;

    key_size_   = h.key_size;
    value_size_ = h.value_size;
    return FlashLogError::OK;
}

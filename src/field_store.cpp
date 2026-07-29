#include "field_store.h"
#include "crc16.h"
#include <cstring>

static constexpr uint32_t MAGIC = 0x464C4F47;

static constexpr size_t HEADER_SIZE = 8;

static void encode_header(uint8_t buf[HEADER_SIZE], uint8_t key_size, uint8_t value_size)
{
    buf[0] = static_cast<uint8_t>(MAGIC);
    buf[1] = static_cast<uint8_t>(MAGIC >> 8);
    buf[2] = static_cast<uint8_t>(MAGIC >> 16);
    buf[3] = static_cast<uint8_t>(MAGIC >> 24);
    buf[4] = key_size;
    buf[5] = value_size;
    uint16_t crc = crc16(buf, 6);
    buf[6] = static_cast<uint8_t>(crc);
    buf[7] = static_cast<uint8_t>(crc >> 8);
}

static uint32_t decode_u32_le(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

static uint16_t decode_u16_le(const uint8_t* p)
{
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

FieldStore::FieldStore(IFlash& flash)
    : flash_(flash)
{
}

static size_t fields_in_sector(size_t sector_usable_bytes, size_t field_size)
{
    return sector_usable_bytes / field_size;
}

// Every sector reserves HEADER_SIZE bytes at its top, so field placement is
// uniform across all sectors (sector 0's reserved bytes hold the real header;
// the others are left erased).
static uint32_t address_of_field(uint32_t index, size_t field_size, size_t sector_size)
{
    size_t   per_sector = fields_in_sector(sector_size - HEADER_SIZE, field_size);
    uint32_t sector     = index / static_cast<uint32_t>(per_sector);
    uint32_t offset     = index % static_cast<uint32_t>(per_sector);
    return sector * static_cast<uint32_t>(sector_size)
         + HEADER_SIZE
         + offset * static_cast<uint32_t>(field_size);
}

FlashLogError FieldStore::format(size_t key_size, size_t value_size)
{
    if (key_size == 0 || value_size == 0 || key_size > 4)
        return FlashLogError::ARG_INVALID;

    uint8_t buf[HEADER_SIZE];
    encode_header(buf, static_cast<uint8_t>(key_size), static_cast<uint8_t>(value_size));

    size_t written = flash_.write(0, buf, HEADER_SIZE, 0);
    return written == HEADER_SIZE ? FlashLogError::OK : FlashLogError::FLASH_WRITE_ERROR;
}

FlashLogError FieldStore::init()
{
    uint8_t buf[HEADER_SIZE];
    size_t n = flash_.read(0, buf, HEADER_SIZE, 0);
    if (n != HEADER_SIZE)
        return FlashLogError::FLASH_READ_ERROR;

    uint32_t magic = decode_u32_le(buf);
    if (magic == 0xFFFFFFFF)
        return FlashLogError::FORMAT_MISSING;
    if (magic != MAGIC)
        return FlashLogError::FORMAT_CORRUPT;

    uint16_t expected = crc16(buf, 6);
    if (decode_u16_le(buf + 6) != expected)
        return FlashLogError::FORMAT_CORRUPT;

    key_size_     = buf[4];
    value_size_   = buf[5];
    initialized_  = true;

    size_t field_size    = key_size_ + value_size_;
    size_t sector_size   = flash_.getSectorSize();
    size_t total_sectors = flash_.getSize() / sector_size;
    total_fields_ = static_cast<uint32_t>(
        total_sectors * fields_in_sector(sector_size - HEADER_SIZE, field_size));

    return FlashLogError::OK;
}

FlashLogError FieldStore::write(uint32_t index, uint32_t key, const void* value)
{
    if (!initialized_)
        return FlashLogError::STORE_NOT_INITIALIZED;
    if (index >= total_fields_)
        return FlashLogError::ARG_OUT_OF_BOUNDS;

    size_t   field_size    = key_size_ + value_size_;
    size_t   sector_size   = flash_.getSectorSize();
    uint32_t field_address = address_of_field(index, field_size, sector_size);

    uint8_t key_bytes[4];
    key_bytes[0] = static_cast<uint8_t>(key);
    key_bytes[1] = static_cast<uint8_t>(key >> 8);
    key_bytes[2] = static_cast<uint8_t>(key >> 16);
    key_bytes[3] = static_cast<uint8_t>(key >> 24);

    flash_.write(field_address, key_bytes, key_size_, 0);
    flash_.write(field_address + key_size_, value, value_size_, 0);

    uint8_t key_readback[4]   = {0xFF, 0xFF, 0xFF, 0xFF};
    flash_.read(field_address, key_readback, key_size_, 0);
    if (memcmp(key_readback, key_bytes, key_size_) != 0)
        return FlashLogError::FLASH_WRITE_ERROR;

    uint8_t value_readback[256];
    flash_.read(field_address + key_size_, value_readback, value_size_, 0);
    if (memcmp(value_readback, value, value_size_) != 0)
        return FlashLogError::FLASH_WRITE_ERROR;

    return FlashLogError::OK;
}

FlashLogError FieldStore::read(uint32_t index, void* key_out, void* value_out)
{
    if (!initialized_)
        return FlashLogError::STORE_NOT_INITIALIZED;
    if (index >= total_fields_)
        return FlashLogError::ARG_OUT_OF_BOUNDS;

    size_t   field_size    = key_size_ + value_size_;
    size_t   sector_size   = flash_.getSectorSize();
    uint32_t field_address = address_of_field(index, field_size, sector_size);

    flash_.read(field_address,             key_out,   key_size_,   0);
    flash_.read(field_address + key_size_, value_out, value_size_, 0);

    return FlashLogError::OK;
}

FlashLogError FieldStore::clear(uint32_t first_field, uint32_t field_count)
{
    if (!initialized_)
        return FlashLogError::STORE_NOT_INITIALIZED;
    if (first_field >= total_fields_)
        return FlashLogError::ARG_OUT_OF_BOUNDS;

    size_t field_size  = key_size_ + value_size_;
    size_t sector_size = flash_.getSectorSize();
    size_t per_sector  = fields_in_sector(sector_size - HEADER_SIZE, field_size);

    // A clear must cover whole erase-units: aligned start, whole-unit count.
    if (first_field % per_sector != 0 || field_count % per_sector != 0)
        return FlashLogError::ARG_INVALID;
    if (first_field + field_count > total_fields_)
        return FlashLogError::ARG_OUT_OF_BOUNDS;

    uint32_t first_sector = first_field / static_cast<uint32_t>(per_sector);
    uint32_t units        = field_count / static_cast<uint32_t>(per_sector);
    for (uint32_t i = 0; i < units; i++)
        flash_.erase((first_sector + i) * static_cast<uint32_t>(sector_size), 0);
    return FlashLogError::OK;
}

uint8_t FieldStore::key_size() const   { return key_size_; }
uint8_t FieldStore::value_size() const { return value_size_; }

uint32_t FieldStore::fieldsPerUnit() const
{
    size_t field_size = key_size_ + value_size_;
    if (field_size == 0)
        return 0;
    return static_cast<uint32_t>(fields_in_sector(flash_.getSectorSize() - HEADER_SIZE, field_size));
}

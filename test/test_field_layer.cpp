#include <gtest/gtest.h>
#include "ram_flash.h"
#include "../src/field_store.h"

TEST(FieldStore, init_returns_not_formatted_on_fresh_flash) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    EXPECT_EQ(store.init(), FlashLogError::NOT_FORMATTED);
}

TEST(FieldStore, init_returns_ok_after_format) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    EXPECT_EQ(store.format(1, 4), FlashLogError::OK);
    EXPECT_EQ(store.init(), FlashLogError::OK);
}

TEST(FieldStore, init_returns_unknown_format_when_magic_is_wrong) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    uint8_t bad_magic[4] = {0x00, 0x00, 0x00, 0x00};
    flash.write(0, bad_magic, 4, 0);
    EXPECT_EQ(store.init(), FlashLogError::UNKNOWN_FORMAT);
}

TEST(FieldStore, init_returns_unknown_format_when_crc_is_corrupt) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(1, 4);
    uint8_t zeros[2] = {0x00, 0x00};
    flash.write(6, zeros, 2, 0);
    EXPECT_EQ(store.init(), FlashLogError::UNKNOWN_FORMAT);
}

TEST(FieldStore, format_returns_invalid_argument_for_zero_key_size) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    EXPECT_EQ(store.format(0, 4), FlashLogError::INVALID_ARGUMENT);
}

TEST(FieldStore, format_returns_invalid_argument_for_zero_value_size) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    EXPECT_EQ(store.format(1, 0), FlashLogError::INVALID_ARGUMENT);
}

TEST(FieldStore, key_size_and_value_size_return_zero_before_init) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    EXPECT_EQ(store.key_size(), 0);
    EXPECT_EQ(store.value_size(), 0);
}

TEST(FieldStore, key_size_and_value_size_reflect_format_after_init) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(2, 8);
    store.init();
    EXPECT_EQ(store.key_size(), 2);
    EXPECT_EQ(store.value_size(), 8);
}



TEST(FieldStore, format_returns_invalid_argument_when_key_size_exceeds_4) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    EXPECT_EQ(store.format(5, 4), FlashLogError::INVALID_ARGUMENT);
}

TEST(FieldStore, append_returns_not_initialized_before_init) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(1, 4);
    uint8_t value[4] = {1, 2, 3, 4};
    EXPECT_EQ(store.append(0x01, value), FlashLogError::NOT_INITIALIZED);
}

TEST(FieldStore, append_writes_field_immediately_after_header) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(1, 4);
    store.init();
    uint8_t value[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    EXPECT_EQ(store.append(0x01, value), FlashLogError::OK);
    uint8_t field[5];
    flash.read(8, field, 5, 0);
    EXPECT_EQ(field[0], 0x01);
    EXPECT_EQ(field[1], 0xAA);
    EXPECT_EQ(field[2], 0xBB);
    EXPECT_EQ(field[3], 0xCC);
    EXPECT_EQ(field[4], 0xDD);
}

TEST(FieldStore, second_append_follows_first_field) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(1, 4);
    store.init();
    uint8_t first_value[4]  = {0x11, 0x22, 0x33, 0x44};
    uint8_t second_value[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    store.append(0x01, first_value);
    EXPECT_EQ(store.append(0x02, second_value), FlashLogError::OK);
    uint8_t field[5];
    flash.read(13, field, 5, 0);
    EXPECT_EQ(field[0], 0x02);
    EXPECT_EQ(field[1], 0xAA);
    EXPECT_EQ(field[2], 0xBB);
    EXPECT_EQ(field[3], 0xCC);
    EXPECT_EQ(field[4], 0xDD);
}

// RamFlash<32,16>: sector_size=16, field_size=3 (key=1, value=2)
// Sector 0: header at bytes 0-7, fields at 8-10 and 11-13, bytes 14-15 too small for another field
// Sector 1: fields start at byte 16
TEST(FieldStore, append_skips_to_next_sector_when_field_would_cross_boundary) {
    RamFlash<32, 16> flash;
    FieldStore store(flash);
    store.format(1, 2);
    store.init();
    uint8_t dummy[2] = {0};
    store.append(0x01, dummy);
    store.append(0x02, dummy);
    uint8_t value[2] = {0xAA, 0xBB};
    EXPECT_EQ(store.append(0x03, value), FlashLogError::OK);
    uint8_t field[3];
    flash.read(16, field, 3, 0);
    EXPECT_EQ(field[0], 0x03);
    EXPECT_EQ(field[1], 0xAA);
    EXPECT_EQ(field[2], 0xBB);
}

// RamFlash<32,16>, field_size=3: 2 fields in sector 0 + 5 in sector 1 = 7 total
TEST(FieldStore, append_returns_flash_full_when_all_slots_used) {
    RamFlash<32, 16> flash;
    FieldStore store(flash);
    store.format(1, 2);
    store.init();
    uint8_t value[2] = {0};
    for (int i = 0; i < 7; i++)
        EXPECT_EQ(store.append(0x01, value), FlashLogError::OK);
    EXPECT_EQ(store.append(0x01, value), FlashLogError::FLASH_FULL);
}






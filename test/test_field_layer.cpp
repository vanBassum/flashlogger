#include <gtest/gtest.h>
#include "ram_flash.h"
#include "../src/field_store.h"

TEST(FieldStore, init_returns_not_formatted_on_fresh_flash) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    EXPECT_EQ(store.init(), FlashLogError::FORMAT_MISSING);
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
    EXPECT_EQ(store.init(), FlashLogError::FORMAT_CORRUPT);
}

TEST(FieldStore, init_returns_unknown_format_when_crc_is_corrupt) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(1, 4);
    uint8_t zeros[2] = {0x00, 0x00};
    flash.write(6, zeros, 2, 0);
    EXPECT_EQ(store.init(), FlashLogError::FORMAT_CORRUPT);
}

TEST(FieldStore, format_returns_invalid_argument_for_zero_key_size) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    EXPECT_EQ(store.format(0, 4), FlashLogError::ARG_INVALID);
}

TEST(FieldStore, format_returns_invalid_argument_for_zero_value_size) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    EXPECT_EQ(store.format(1, 0), FlashLogError::ARG_INVALID);
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
    EXPECT_EQ(store.format(5, 4), FlashLogError::ARG_INVALID);
}

TEST(FieldStore, write_returns_not_initialized_before_init) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(1, 4);
    uint8_t value[4] = {1, 2, 3, 4};
    EXPECT_EQ(store.write(0, 0x01, value), FlashLogError::STORE_NOT_INITIALIZED);
}

TEST(FieldStore, write_returns_out_of_bounds_for_invalid_index) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(1, 4);
    store.init();
    uint8_t value[4] = {0};
    uint32_t total_fields = (4096 - 8) / 5 + 0;  // sector 0 only (1 sector)
    // Actually 4096/256=16 sectors; sector0: (256-8)/5=49, others: 256/5=51 each
    uint32_t out_of_range = 49 + 15 * 51;
    EXPECT_EQ(store.write(out_of_range, 0x01, value), FlashLogError::ARG_OUT_OF_BOUNDS);
}

TEST(FieldStore, write_stores_key_and_value_at_index_zero) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(1, 4);
    store.init();
    uint8_t value[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    EXPECT_EQ(store.write(0, 0x01, value), FlashLogError::OK);
    uint8_t field[5];
    flash.read(8, field, 5, 0);
    EXPECT_EQ(field[0], 0x01);
    EXPECT_EQ(field[1], 0xAA);
    EXPECT_EQ(field[2], 0xBB);
    EXPECT_EQ(field[3], 0xCC);
    EXPECT_EQ(field[4], 0xDD);
}

TEST(FieldStore, write_stores_key_and_value_at_index_one) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(1, 4);
    store.init();
    uint8_t value[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    EXPECT_EQ(store.write(1, 0x02, value), FlashLogError::OK);
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
// Sector 1: fields start at byte 16 — index 2 maps there
TEST(FieldStore, write_places_field_correctly_across_sector_boundary) {
    RamFlash<32, 16> flash;
    FieldStore store(flash);
    store.format(1, 2);
    store.init();
    uint8_t value[2] = {0xAA, 0xBB};
    EXPECT_EQ(store.write(2, 0x03, value), FlashLogError::OK);
    uint8_t field[3];
    flash.read(16, field, 3, 0);
    EXPECT_EQ(field[0], 0x03);
    EXPECT_EQ(field[1], 0xAA);
    EXPECT_EQ(field[2], 0xBB);
}

TEST(FieldStore, read_returns_not_initialized_before_init) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(1, 4);
    uint8_t key_out;
    uint8_t value_out[4];
    EXPECT_EQ(store.read(0, &key_out, value_out), FlashLogError::STORE_NOT_INITIALIZED);
}

TEST(FieldStore, read_returns_out_of_bounds_for_invalid_index) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(1, 4);
    store.init();
    uint8_t key_out;
    uint8_t value_out[4];
    uint32_t out_of_range = 49 + 15 * 51;
    EXPECT_EQ(store.read(out_of_range, &key_out, value_out), FlashLogError::ARG_OUT_OF_BOUNDS);
}

TEST(FieldStore, read_retrieves_what_was_written) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(1, 4);
    store.init();
    uint8_t written_value[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    store.write(0, 0x07, written_value);
    uint8_t key_out      = 0;
    uint8_t value_out[4] = {0};
    EXPECT_EQ(store.read(0, &key_out, value_out), FlashLogError::OK);
    EXPECT_EQ(key_out,      0x07);
    EXPECT_EQ(value_out[0], 0xAA);
    EXPECT_EQ(value_out[1], 0xBB);
    EXPECT_EQ(value_out[2], 0xCC);
    EXPECT_EQ(value_out[3], 0xDD);
}






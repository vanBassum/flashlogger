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

// clear() takes a field index and a count, mirroring how IFlash erases a
// range rather than a named sector. RamFlash<32,16> with 1+2 byte fields
// gives sector 0 = indices 0,1 and sector 1 = indices 2..6; clear(2,5)
// covers exactly the fields in sector 1, leaving the header untouched.
TEST(FieldStore, clear_erases_the_given_field_range) {
    RamFlash<32, 16> flash;
    FieldStore store(flash);
    store.format(1, 2);
    store.init();

    uint8_t v0[2] = {0x11, 0x22};
    uint8_t v2[2] = {0xAA, 0xBB};
    EXPECT_EQ(store.write(0, 0x01, v0), FlashLogError::OK);  // index 0 (sector 0)
    EXPECT_EQ(store.write(2, 0x03, v2), FlashLogError::OK);  // index 2 (sector 1)

    EXPECT_EQ(store.clear(2, 5), FlashLogError::OK);

    // The cleared range reads back erased (0xFF).
    uint8_t key_out = 0;
    uint8_t val_out[2] = {0};
    EXPECT_EQ(store.read(2, &key_out, val_out), FlashLogError::OK);
    EXPECT_EQ(key_out,    0xFF);
    EXPECT_EQ(val_out[0], 0xFF);
    EXPECT_EQ(val_out[1], 0xFF);

    // A field outside the cleared range survives.
    EXPECT_EQ(store.read(0, &key_out, val_out), FlashLogError::OK);
    EXPECT_EQ(key_out,    0x01);
    EXPECT_EQ(val_out[0], 0x11);
    EXPECT_EQ(val_out[1], 0x22);
}

// A clear range must cover whole erase-units (sectors). A range that starts
// mid-sector is rejected and erases nothing. (sector 1 = indices 2..6, so
// index 3 is mid-sector.)
TEST(FieldStore, clear_returns_invalid_when_range_starts_mid_sector) {
    RamFlash<32, 16> flash;
    FieldStore store(flash);
    store.format(1, 2);
    store.init();
    uint8_t v2[2] = {0xAA, 0xBB};
    EXPECT_EQ(store.write(2, 0x03, v2), FlashLogError::OK);

    EXPECT_EQ(store.clear(3, 2), FlashLogError::ARG_INVALID);

    uint8_t key_out = 0;
    uint8_t val_out[2] = {0};
    EXPECT_EQ(store.read(2, &key_out, val_out), FlashLogError::OK);
    EXPECT_EQ(key_out,    0x03);
    EXPECT_EQ(val_out[0], 0xAA);
    EXPECT_EQ(val_out[1], 0xBB);
}

// A range aligned at the start but not spanning the whole sector is also
// rejected. (sector 1 holds 5 fields; count 3 leaves a partial sector.)
TEST(FieldStore, clear_returns_invalid_when_range_is_partial_sector) {
    RamFlash<32, 16> flash;
    FieldStore store(flash);
    store.format(1, 2);
    store.init();
    uint8_t v2[2] = {0xAA, 0xBB};
    EXPECT_EQ(store.write(2, 0x03, v2), FlashLogError::OK);

    EXPECT_EQ(store.clear(2, 3), FlashLogError::ARG_INVALID);

    uint8_t key_out = 0;
    uint8_t val_out[2] = {0};
    EXPECT_EQ(store.read(2, &key_out, val_out), FlashLogError::OK);
    EXPECT_EQ(key_out,    0x03);
    EXPECT_EQ(val_out[0], 0xAA);
    EXPECT_EQ(val_out[1], 0xBB);
}






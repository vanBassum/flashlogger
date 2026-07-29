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
    EXPECT_EQ(store.keySize(), 0);
    EXPECT_EQ(store.valueSize(), 0);
}

TEST(FieldStore, fields_per_unit_returns_zero_before_init) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    EXPECT_EQ(store.fieldsPerUnit(), 0u);
}

TEST(FieldStore, key_size_and_value_size_reflect_format_after_init) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(2, 8);
    store.init();
    EXPECT_EQ(store.keySize(), 2);
    EXPECT_EQ(store.valueSize(), 8);
}



TEST(FieldStore, format_returns_invalid_argument_when_key_size_exceeds_4) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    EXPECT_EQ(store.format(5, 4), FlashLogError::ARG_INVALID);
}

TEST(FieldStore, format_returns_invalid_argument_when_value_size_exceeds_255) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    EXPECT_EQ(store.format(1, 256), FlashLogError::ARG_INVALID);
}

// A field that can't fit in a sector's usable space is rejected at format,
// rather than silently producing a zero-capacity store.
TEST(FieldStore, format_returns_invalid_when_field_larger_than_sector) {
    RamFlash<32, 16> flash;   // usable per sector = 16 - 8 header = 8 bytes
    FieldStore store(flash);
    EXPECT_EQ(store.format(1, 8), FlashLogError::ARG_INVALID);  // field_size 9 > 8
}

// The maximum key (4) and value (255) sizes are accepted when they fit.
TEST(FieldStore, format_accepts_max_key_and_value_sizes) {
    RamFlash<8192, 4096> flash;   // usable 4088 bytes, fits a 259-byte field
    FieldStore store(flash);
    EXPECT_EQ(store.format(4, 255), FlashLogError::OK);
    EXPECT_EQ(store.init(),         FlashLogError::OK);
    EXPECT_EQ(store.keySize(),      4);
    EXPECT_EQ(store.valueSize(),    255);
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
    // 16 sectors, each holding (256-8)/5 = 49 fields → 784 total.
    uint32_t out_of_range = 16 * 49;  // first index past the last field
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

// A field in the second erase-unit round-trips through write/read. Uses the
// exposed unit size instead of a hardcoded address, so it doesn't care where
// the unit physically sits.
TEST(FieldStore, round_trips_a_field_in_the_second_unit) {
    RamFlash<32, 16> flash;
    FieldStore store(flash);
    store.format(1, 2);
    store.init();
    uint32_t n = store.fieldsPerUnit();  // first field of the second unit

    uint8_t value[2] = {0xAA, 0xBB};
    EXPECT_EQ(store.write(n, 0x03, value), FlashLogError::OK);

    uint8_t key_out = 0;
    uint8_t val_out[2] = {0};
    EXPECT_EQ(store.read(n, &key_out, val_out), FlashLogError::OK);
    EXPECT_EQ(key_out,    0x03);
    EXPECT_EQ(val_out[0], 0xAA);
    EXPECT_EQ(val_out[1], 0xBB);
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
    uint32_t out_of_range = 16 * 49;  // first index past the last field
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

// A multi-byte key is stored little-endian and round-trips intact.
TEST(FieldStore, multi_byte_key_round_trips_little_endian) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(2, 4);   // 2-byte key
    store.init();

    uint8_t value[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    EXPECT_EQ(store.write(0, 0x1234, value), FlashLogError::OK);

    uint8_t key_out[2] = {0};
    uint8_t val_out[4] = {0};
    EXPECT_EQ(store.read(0, key_out, val_out), FlashLogError::OK);
    EXPECT_EQ(key_out[0], 0x34);   // little-endian: low byte first
    EXPECT_EQ(key_out[1], 0x12);
    EXPECT_EQ(val_out[0], 0xDE);
    EXPECT_EQ(val_out[1], 0xAD);
    EXPECT_EQ(val_out[2], 0xBE);
    EXPECT_EQ(val_out[3], 0xEF);
}

// A fresh FieldStore over the same flash reads the existing format and data,
// as it would after a reboot (in-RAM store gone, flash contents persist).
TEST(FieldStore, reopening_reads_existing_format_and_data) {
    RamFlash<4096, 256> flash;
    {
        FieldStore store(flash);
        ASSERT_EQ(store.format(1, 4), FlashLogError::OK);
        ASSERT_EQ(store.init(),       FlashLogError::OK);
        uint8_t v[4] = {0xDE, 0xAD, 0xBE, 0xEF};
        ASSERT_EQ(store.write(0, 0x07, v), FlashLogError::OK);
    }

    FieldStore reopened(flash);  // same flash contents, new store
    EXPECT_EQ(reopened.init(),      FlashLogError::OK);
    EXPECT_EQ(reopened.keySize(),   1);
    EXPECT_EQ(reopened.valueSize(), 4);

    uint8_t key_out = 0;
    uint8_t val_out[4] = {0};
    EXPECT_EQ(reopened.read(0, &key_out, val_out), FlashLogError::OK);
    EXPECT_EQ(key_out,    0x07);
    EXPECT_EQ(val_out[0], 0xDE);
    EXPECT_EQ(val_out[1], 0xAD);
    EXPECT_EQ(val_out[2], 0xBE);
    EXPECT_EQ(val_out[3], 0xEF);
}

// Clearing the second erase-unit erases the fields there and leaves the first
// unit intact. Expressed with the exposed unit size, not a hardcoded layout.
TEST(FieldStore, clear_erases_the_second_unit) {
    RamFlash<32, 16> flash;
    FieldStore store(flash);
    store.format(1, 2);
    store.init();
    uint32_t n = store.fieldsPerUnit();

    uint8_t v0[2] = {0x11, 0x22};
    uint8_t v1[2] = {0xAA, 0xBB};
    EXPECT_EQ(store.write(0, 0x01, v0), FlashLogError::OK);  // unit 0
    EXPECT_EQ(store.write(n, 0x03, v1), FlashLogError::OK);  // unit 1, first field

    EXPECT_EQ(store.clear(n, n), FlashLogError::OK);         // clear unit 1

    // The cleared unit reads back erased (0xFF).
    uint8_t key_out = 0;
    uint8_t val_out[2] = {0};
    EXPECT_EQ(store.read(n, &key_out, val_out), FlashLogError::OK);
    EXPECT_EQ(key_out,    0xFF);
    EXPECT_EQ(val_out[0], 0xFF);
    EXPECT_EQ(val_out[1], 0xFF);

    // A field in the untouched first unit survives.
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

TEST(FieldStore, clear_returns_not_initialized_before_init) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(1, 4);
    EXPECT_EQ(store.clear(0, 49), FlashLogError::STORE_NOT_INITIALIZED);
}

TEST(FieldStore, clear_returns_out_of_bounds_for_invalid_index) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    store.format(1, 4);
    store.init();
    uint32_t out_of_range = 16 * 49;  // first index past the last field
    EXPECT_EQ(store.clear(out_of_range, 49), FlashLogError::ARG_OUT_OF_BOUNDS);
}

// clear() can span multiple whole erase-units in one call.
TEST(FieldStore, clear_erases_multiple_units) {
    RamFlash<48, 16> flash;   // 3 erase-units
    FieldStore store(flash);
    store.format(1, 2);
    store.init();
    uint32_t n = store.fieldsPerUnit();

    uint8_t v[2] = {0xAA, 0xBB};
    EXPECT_EQ(store.write(0,     0x01, v), FlashLogError::OK);  // unit 0
    EXPECT_EQ(store.write(n,     0x02, v), FlashLogError::OK);  // unit 1
    EXPECT_EQ(store.write(2 * n, 0x03, v), FlashLogError::OK);  // unit 2

    EXPECT_EQ(store.clear(0, 2 * n), FlashLogError::OK);        // clear units 0 and 1

    uint8_t key_out = 0;
    uint8_t val_out[2] = {0};
    EXPECT_EQ(store.read(0, &key_out, val_out), FlashLogError::OK);
    EXPECT_EQ(key_out, 0xFF);
    EXPECT_EQ(store.read(n, &key_out, val_out), FlashLogError::OK);
    EXPECT_EQ(key_out, 0xFF);
    // unit 2 was outside the range and survives.
    EXPECT_EQ(store.read(2 * n, &key_out, val_out), FlashLogError::OK);
    EXPECT_EQ(key_out, 0x03);
}

// A multi-unit range that runs past the end of the store is out of bounds.
TEST(FieldStore, clear_returns_out_of_bounds_when_range_exceeds_store) {
    RamFlash<48, 16> flash;   // 3 erase-units
    FieldStore store(flash);
    store.format(1, 2);
    store.init();
    uint32_t n = store.fieldsPerUnit();
    // start at the last unit and ask for two — spills past the store
    EXPECT_EQ(store.clear(2 * n, 2 * n), FlashLogError::ARG_OUT_OF_BOUNDS);
}

// A whole-unit count so large that first_field + field_count overflows must
// still be rejected, not wrap past the bounds check.
TEST(FieldStore, clear_rejects_count_that_overflows_range) {
    RamFlash<48, 16> flash;   // 3 erase-units
    FieldStore store(flash);
    store.format(1, 2);
    store.init();
    uint32_t n = store.fieldsPerUnit();
    // aligned start, whole-unit count that wraps uint32 when added to it.
    EXPECT_EQ(store.clear(n, 0xFFFFFFFE), FlashLogError::ARG_OUT_OF_BOUNDS);
}

// After clearing a unit, fresh fields can be written into it again — the
// clear truly returns the flash to a writable (0xFF) state.
TEST(FieldStore, can_write_into_a_cleared_unit) {
    RamFlash<32, 16> flash;
    FieldStore store(flash);
    store.format(1, 2);
    store.init();
    uint32_t n = store.fieldsPerUnit();

    uint8_t v1[2] = {0xAA, 0xBB};
    EXPECT_EQ(store.write(n, 0x03, v1), FlashLogError::OK);  // write unit 1
    EXPECT_EQ(store.clear(n, n),        FlashLogError::OK);  // clear it

    uint8_t v2[2] = {0x11, 0x22};
    EXPECT_EQ(store.write(n, 0x05, v2), FlashLogError::OK);  // rewrite after clear

    uint8_t key_out = 0;
    uint8_t val_out[2] = {0};
    EXPECT_EQ(store.read(n, &key_out, val_out), FlashLogError::OK);
    EXPECT_EQ(key_out,    0x05);
    EXPECT_EQ(val_out[0], 0x11);
    EXPECT_EQ(val_out[1], 0x22);
}






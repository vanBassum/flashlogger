#include <gtest/gtest.h>
#include "ram_flash.h"
#include "../src/record_log.h"

TEST(RecordLog, init_reports_format_missing_on_fresh_flash) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    EXPECT_EQ(log.init(), FlashLogError::FORMAT_MISSING);
}

TEST(RecordLog, init_reports_ok_after_format) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    EXPECT_EQ(log.format(1, 4), FlashLogError::OK);
    EXPECT_EQ(log.init(), FlashLogError::OK);
}

TEST(RecordLog, a_record_takes_a_field) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record = log.createRecord();
    EXPECT_EQ(record.field(7, &value), FlashLogError::OK);
}

TEST(RecordLog, a_field_reads_back_what_was_written) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record = log.createRecord();
    ASSERT_EQ(record.field(7, &value), FlashLogError::OK);
    ASSERT_EQ(record.close(), FlashLogError::OK);

    auto reader = log.firstRecord();
    uint32_t out = 0;
    EXPECT_EQ(reader.read(7, &out, sizeof(out)), FlashLogError::OK);
    EXPECT_EQ(out, 0x11223344u);
}

TEST(RecordLog, read_refuses_a_buffer_too_small_for_the_value) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);   // 4-byte values
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    {
        auto record = log.createRecord();
        ASSERT_EQ(record.field(7, &value), FlashLogError::OK);
    }

    auto reader = log.firstRecord();

    uint16_t too_small = 0;
    EXPECT_EQ(reader.read(7, &too_small, sizeof(too_small)), FlashLogError::ARG_INVALID);

    uint32_t big_enough = 0;                          // the right size still works
    EXPECT_EQ(reader.read(7, &big_enough, sizeof(big_enough)), FlashLogError::OK);
    EXPECT_EQ(big_enough, 0x11223344u);
}

TEST(RecordLog, reading_one_record_does_not_find_another_records_field) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t first = 0x11223344;
    {
        auto record = log.createRecord();
        ASSERT_EQ(record.field(7, &first), FlashLogError::OK);
    }

    uint32_t second = 0x55667788;
    {
        auto record = log.createRecord();
        ASSERT_EQ(record.field(8, &second), FlashLogError::OK);
    }

    auto reader = log.firstRecord();

    uint32_t out = 0;
    EXPECT_NE(reader.read(8, &out, sizeof(out)), FlashLogError::OK);   // key 8 is record 2's

    uint32_t own = 0;                                     // record 1's own field still reads
    EXPECT_EQ(reader.read(7, &own, sizeof(own)), FlashLogError::OK);
    EXPECT_EQ(own, 0x11223344u);
}

TEST(RecordLog, a_second_record_cannot_open_while_one_is_still_open) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record_1 = log.createRecord();
    ASSERT_EQ(record_1.field(7, &value), FlashLogError::OK);

    auto record_2 = log.createRecord();   // record_1 is still open
    EXPECT_EQ(record_2.field(8, &value), FlashLogError::RECORD_ALREADY_OPEN);
}

TEST(RecordLog, the_destructor_closes_a_record_so_the_next_one_can_open) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    {
        auto record_1 = log.createRecord();
        ASSERT_EQ(record_1.field(7, &value), FlashLogError::OK);
    }   // no close() call — the destructor has to do it

    auto record_2 = log.createRecord();
    EXPECT_EQ(record_2.field(8, &value), FlashLogError::OK);
}

TEST(RecordLog, close_ends_a_record_without_waiting_for_the_destructor) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record_1 = log.createRecord();
    ASSERT_EQ(record_1.field(7, &value), FlashLogError::OK);
    ASSERT_EQ(record_1.close(), FlashLogError::OK);

    auto record_2 = log.createRecord();       // record_1 still in scope, but closed
    EXPECT_EQ(record_2.field(8, &value), FlashLogError::OK);
}

TEST(RecordLog, a_field_cannot_use_a_reserved_key_with_one_byte_keys) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record = log.createRecord();
    EXPECT_EQ(record.field(0xFF, &value), FlashLogError::ARG_INVALID);  // empty
    EXPECT_EQ(record.field(0x00, &value), FlashLogError::ARG_INVALID);  // tombstone
    EXPECT_EQ(record.field(0x01, &value), FlashLogError::ARG_INVALID);  // record start
}

TEST(RecordLog, a_field_cannot_use_a_reserved_key_with_four_byte_keys) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(4, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record = log.createRecord();
    EXPECT_EQ(record.field(0xFFFFFFFF, &value), FlashLogError::ARG_INVALID);  // empty
    EXPECT_EQ(record.field(0x00000000, &value), FlashLogError::ARG_INVALID);  // tombstone
    EXPECT_EQ(record.field(0x00000001, &value), FlashLogError::ARG_INVALID);  // record start
}

// "Empty" is all-ones for the key width, so a narrower all-ones value is
// ordinary user data once keys are wider.
TEST(RecordLog, reserved_keys_scale_with_key_width) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(4, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record = log.createRecord();
    EXPECT_EQ(record.field(0xFF, &value), FlashLogError::OK);
}

TEST(RecordLog, a_rejected_format_leaves_the_store_unformatted) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    EXPECT_EQ(log.format(0, 4), FlashLogError::ARG_INVALID);
    EXPECT_EQ(log.init(), FlashLogError::FORMAT_MISSING);
}

TEST(RecordLog, init_tells_junk_apart_from_blank_flash) {
    RamFlash<4096, 256> flash;
    uint8_t junk[4] = {0x00, 0x00, 0x00, 0x00};
    flash.write(0, junk, 4, 0);
    RecordLog log(flash);
    EXPECT_EQ(log.init(), FlashLogError::FORMAT_CORRUPT);
}



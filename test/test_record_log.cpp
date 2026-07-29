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
    auto record = log.WriteRecord();
    EXPECT_EQ(record.field(7, &value), FlashLogError::OK);
}

TEST(RecordLog, a_second_record_cannot_open_while_one_is_still_open) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record_1 = log.WriteRecord();
    ASSERT_EQ(record_1.field(7, &value), FlashLogError::OK);

    auto record_2 = log.WriteRecord();   // record_1 is still open
    EXPECT_EQ(record_2.field(8, &value), FlashLogError::RECORD_ALREADY_OPEN);
}

TEST(RecordLog, the_destructor_closes_a_record_so_the_next_one_can_open) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    {
        auto record_1 = log.WriteRecord();
        ASSERT_EQ(record_1.field(7, &value), FlashLogError::OK);
    }   // no close() call — the destructor has to do it

    auto record_2 = log.WriteRecord();
    EXPECT_EQ(record_2.field(8, &value), FlashLogError::OK);
}

TEST(RecordLog, close_ends_a_record_without_waiting_for_the_destructor) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record_1 = log.WriteRecord();
    ASSERT_EQ(record_1.field(7, &value), FlashLogError::OK);
    ASSERT_EQ(record_1.close(), FlashLogError::OK);

    auto record_2 = log.WriteRecord();       // record_1 still in scope, but closed
    EXPECT_EQ(record_2.field(8, &value), FlashLogError::OK);
}

TEST(RecordLog, a_field_cannot_use_a_reserved_key_with_one_byte_keys) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record = log.WriteRecord();
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
    auto record = log.WriteRecord();
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
    auto record = log.WriteRecord();
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



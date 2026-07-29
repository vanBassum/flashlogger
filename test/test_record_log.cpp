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



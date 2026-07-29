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

TEST(RecordLog, format_erases_the_whole_flash) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);

    uint8_t stale[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    flash.write(3000, stale, 4, 0);   // leftovers in a late sector

    EXPECT_EQ(log.format(1, 4), FlashLogError::OK);

    uint8_t back[4] = {0, 0, 0, 0};
    flash.read(3000, back, 4, 0);
    EXPECT_EQ(back[0], 0xFF);
    EXPECT_EQ(back[1], 0xFF);
    EXPECT_EQ(back[2], 0xFF);
    EXPECT_EQ(back[3], 0xFF);
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

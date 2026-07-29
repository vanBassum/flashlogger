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

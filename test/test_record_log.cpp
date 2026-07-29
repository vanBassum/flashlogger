#include <gtest/gtest.h>
#include "ram_flash.h"
#include "../src/field_store.h"
#include "../src/record_log.h"

TEST(RecordLog, init_reports_format_missing_on_fresh_flash) {
    RamFlash<4096, 256> flash;
    FieldStore store(flash);
    RecordLog log(store);
    EXPECT_EQ(log.init(), FlashLogError::FORMAT_MISSING);
}

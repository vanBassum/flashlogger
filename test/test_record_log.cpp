#include <gtest/gtest.h>
#include "ram_flash.h"
#include "field_store.h"
#include "record_log.h"

// The core loop at its smallest: append a record with one field, read it back.
TEST(RecordLog, append_one_field_record_and_read_it_back) {
    RamFlash<4096, 256> flash;
    FieldStore fields(flash);
    fields.format(1, 4);
    fields.init();
    RecordLog log(fields);

    uint8_t value[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    log.beginRecord();
    log.field(0x10, value);
    log.finishRecord();

    auto it = log.first();
    ASSERT_TRUE(it.valid());

    uint32_t key = 0;
    uint8_t out[4] = {0};
    EXPECT_EQ(it.getField(0, &key, out), FlashLogError::OK);
    EXPECT_EQ(key,    0x10u);
    EXPECT_EQ(out[0], 0xDE);
    EXPECT_EQ(out[1], 0xAD);
    EXPECT_EQ(out[2], 0xBE);
    EXPECT_EQ(out[3], 0xEF);
}

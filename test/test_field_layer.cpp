#include <gtest/gtest.h>
#include "ram_flash.h"

TEST(FieldLayer, setup_works) {
    EXPECT_TRUE(true);
}

TEST(RamFlash, erased_state_is_0xFF) {
    RamFlash<4096, 256> flash;
    uint8_t buf[4];
    flash.read(0, buf, sizeof(buf), 0);
    EXPECT_EQ(buf[0], 0xFF);
    EXPECT_EQ(buf[3], 0xFF);
}

TEST(RamFlash, write_clears_bits) {
    RamFlash<4096, 256> flash;
    uint8_t val = 0xA5;
    flash.write(0, &val, 1, 0);
    uint8_t result;
    flash.read(0, &result, 1, 0);
    EXPECT_EQ(result, 0xA5);
}

TEST(RamFlash, erase_restores_0xFF) {
    RamFlash<4096, 256> flash;
    uint8_t val = 0x00;
    flash.write(0, &val, 1, 0);
    flash.erase(0, 0);
    uint8_t result;
    flash.read(0, &result, 1, 0);
    EXPECT_EQ(result, 0xFF);
}

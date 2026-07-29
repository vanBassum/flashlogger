#include <gtest/gtest.h>
#include "record_codec.h"
#include "crc16.h"

// A field is classified by its key: reserved values mark empty / tombstone /
// record-start; anything else is user data. (keySize 1 for now.)

TEST(RecordCodec, classifies_empty_key) {
    EXPECT_EQ(classifyField(0xFF, 1), FieldKind::Empty);
}

TEST(RecordCodec, classifies_tombstone_key) {
    EXPECT_EQ(classifyField(0x00, 1), FieldKind::Tombstone);
}

TEST(RecordCodec, classifies_record_marker_key) {
    EXPECT_EQ(classifyField(0x01, 1), FieldKind::Header);
}

TEST(RecordCodec, classifies_user_data_key) {
    EXPECT_EQ(classifyField(0x42, 1), FieldKind::Data);
}

// The record CRC is computed incrementally so a record can be checksummed
// field-by-field without buffering the whole thing. Streaming in chunks must
// equal a one-shot CRC over the same bytes.
TEST(RecordCodec, incremental_crc16_matches_one_shot) {
    uint8_t data[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint16_t one_shot    = crc16(data, 6);
    uint16_t incremental = crc16_update(crc16_update(crc16_init(), data, 3), data + 3, 3);
    EXPECT_EQ(incremental, one_shot);
}

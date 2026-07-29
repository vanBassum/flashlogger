#include <gtest/gtest.h>
#include "record_codec.h"

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

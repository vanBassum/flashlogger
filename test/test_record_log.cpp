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
    EXPECT_EQ(record.field(7, &value, sizeof(value)), FlashLogError::OK);
}

TEST(RecordLog, a_field_reads_back_what_was_written) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record = log.createRecord();
    ASSERT_EQ(record.field(7, &value, sizeof(value)), FlashLogError::OK);
    ASSERT_EQ(record.close(), FlashLogError::OK);

    auto reader = log.firstRecord();
    uint32_t out = 0;
    EXPECT_EQ(reader.read(7, &out, sizeof(out)), FlashLogError::OK);
    EXPECT_EQ(out, 0x11223344u);
}

TEST(RecordLog, field_refuses_a_buffer_too_small_for_the_value) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);   // 4-byte values
    ASSERT_EQ(log.init(), FlashLogError::OK);

    auto record = log.createRecord();

    uint16_t too_small = 0x1122;                     // would be read 2 bytes past
    EXPECT_EQ(record.field(7, &too_small, sizeof(too_small)), FlashLogError::ARG_INVALID);

    uint32_t big_enough = 0x11223344;                // the right size still works
    EXPECT_EQ(record.field(7, &big_enough, sizeof(big_enough)), FlashLogError::OK);
}

TEST(RecordLog, read_refuses_a_buffer_too_small_for_the_value) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);   // 4-byte values
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    {
        auto record = log.createRecord();
        ASSERT_EQ(record.field(7, &value, sizeof(value)), FlashLogError::OK);
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
        ASSERT_EQ(record.field(7, &first, sizeof(first)), FlashLogError::OK);
    }

    uint32_t second = 0x55667788;
    {
        auto record = log.createRecord();
        ASSERT_EQ(record.field(8, &second, sizeof(second)), FlashLogError::OK);
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
    ASSERT_EQ(record_1.field(7, &value, sizeof(value)), FlashLogError::OK);

    auto record_2 = log.createRecord();   // record_1 is still open
    EXPECT_EQ(record_2.field(8, &value, sizeof(value)), FlashLogError::RECORD_ALREADY_OPEN);
}

TEST(RecordLog, the_destructor_closes_a_record_so_the_next_one_can_open) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    {
        auto record_1 = log.createRecord();
        ASSERT_EQ(record_1.field(7, &value, sizeof(value)), FlashLogError::OK);
    }   // no close() call — the destructor has to do it

    auto record_2 = log.createRecord();
    EXPECT_EQ(record_2.field(8, &value, sizeof(value)), FlashLogError::OK);
}

TEST(RecordLog, close_ends_a_record_without_waiting_for_the_destructor) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record_1 = log.createRecord();
    ASSERT_EQ(record_1.field(7, &value, sizeof(value)), FlashLogError::OK);
    ASSERT_EQ(record_1.close(), FlashLogError::OK);

    auto record_2 = log.createRecord();       // record_1 still in scope, but closed
    EXPECT_EQ(record_2.field(8, &value, sizeof(value)), FlashLogError::OK);
}

TEST(RecordLog, a_field_cannot_use_a_reserved_key_with_one_byte_keys) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record = log.createRecord();
    EXPECT_EQ(record.field(0xFF, &value, sizeof(value)), FlashLogError::ARG_INVALID);  // empty
    EXPECT_EQ(record.field(0x00, &value, sizeof(value)), FlashLogError::ARG_INVALID);  // tombstone
    EXPECT_EQ(record.field(0x01, &value, sizeof(value)), FlashLogError::ARG_INVALID);  // record start
}

TEST(RecordLog, a_field_cannot_use_a_reserved_key_with_four_byte_keys) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(4, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record = log.createRecord();
    EXPECT_EQ(record.field(0xFFFFFFFF, &value, sizeof(value)), FlashLogError::ARG_INVALID);  // empty
    EXPECT_EQ(record.field(0x00000000, &value, sizeof(value)), FlashLogError::ARG_INVALID);  // tombstone
    EXPECT_EQ(record.field(0x00000001, &value, sizeof(value)), FlashLogError::ARG_INVALID);  // record start
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
    EXPECT_EQ(record.field(0xFF, &value, sizeof(value)), FlashLogError::OK);
}

TEST(RecordLog, format_leaves_no_trace_of_earlier_records) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t old_value = 0x11223344;
    for (int i = 0; i < 3; i++) {
        auto record = log.createRecord();
        ASSERT_EQ(record.field(7, &old_value, sizeof(old_value)), FlashLogError::OK);
    }

    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);      // wipe
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t fresh = 0x55667788;
    {
        auto record = log.createRecord();
        ASSERT_EQ(record.field(9, &fresh, sizeof(fresh)), FlashLogError::OK);
    }

    auto reader = log.firstRecord();

    uint32_t out = 0;                                    // the fresh record IS the first record
    EXPECT_EQ(reader.read(9, &out, sizeof(out)), FlashLogError::OK);
    EXPECT_EQ(out, 0x55667788u);

    EXPECT_NE(reader.read(7, &out, sizeof(out)), FlashLogError::OK);  // nothing from before
}

TEST(RecordLog, a_rejected_field_does_not_consume_a_position) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value     = 0x11223344;
    uint16_t too_small = 0x1122;

    auto record = log.createRecord();
    ASSERT_EQ(record.field(0xFF, &value, sizeof(value)), FlashLogError::ARG_INVALID);
    ASSERT_EQ(record.field(7, &too_small, sizeof(too_small)), FlashLogError::ARG_INVALID);
    ASSERT_EQ(record.field(7, &value, sizeof(value)), FlashLogError::OK);
    ASSERT_EQ(record.close(), FlashLogError::OK);

    // A rejected field that had eaten a position would leave an empty field
    // between the marker and key 7, and the reader would stop there.
    auto reader = log.firstRecord();
    uint32_t out = 0;
    EXPECT_EQ(reader.read(7, &out, sizeof(out)), FlashLogError::OK);
    EXPECT_EQ(out, 0x11223344u);
}

TEST(RecordLog, writing_and_reading_are_refused_before_init) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    // deliberately no init()

    uint32_t value = 0x11223344;
    auto record = log.createRecord();
    EXPECT_EQ(record.field(7, &value, sizeof(value)), FlashLogError::STORE_NOT_INITIALIZED);

    auto reader = log.firstRecord();
    uint32_t out = 0;
    EXPECT_EQ(reader.read(7, &out, sizeof(out)), FlashLogError::STORE_NOT_INITIALIZED);
}

// The "never derails on any flash contents" promise, smallest version: a store
// with no empty field left must still make a read return rather than spin.
TEST(RecordLog, a_read_terminates_on_a_store_with_no_empty_field) {
    RamFlash<512, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record = log.createRecord();
    while (record.field(7, &value, sizeof(value)) == FlashLogError::OK) { }  // fill it up

    auto reader = log.firstRecord();
    uint32_t out = 0;
    EXPECT_NE(reader.read(9, &out, sizeof(out)), FlashLogError::OK);   // returns, doesn't hang
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



TEST(RecordLog, an_unclosed_record_cannot_be_read) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record = log.createRecord();
    ASSERT_EQ(record.field(7, &value, sizeof(value)), FlashLogError::OK);
    // deliberately never closed

    auto reader = log.firstRecord();
    uint32_t out = 0;
    EXPECT_EQ(reader.read(7, &out, sizeof(out)), FlashLogError::RECORD_TORN);
}

// Corruption is detected by recomputing the CRC, not by trusting the stored one.
// Clearing bits in a value on flash is the only way to fake bit rot on NOR.
TEST(RecordLog, a_corrupted_record_is_reported_not_returned) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    {
        auto record = log.createRecord();
        ASSERT_EQ(record.field(7, &value, sizeof(value)), FlashLogError::OK);
    }

    // field index 1 sits at 8 (sector header) + 1 * 5 (key+value); its value
    // starts one byte later.
    uint8_t cleared = 0x00;
    flash.write(8 + 5 + 1, &cleared, 1, 0);

    auto reader = log.firstRecord();
    uint32_t out = 0;
    EXPECT_EQ(reader.read(7, &out, sizeof(out)), FlashLogError::RECORD_CORRUPT);
}

// After a reboot the log has to work out where writing left off. Without that,
// the cursor restarts at 0 and the next record is written over the first one.
TEST(RecordLog, reopening_continues_after_the_last_record) {
    RamFlash<4096, 256> flash;

    uint32_t first = 0x11223344;
    {
        RecordLog log(flash);
        ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
        ASSERT_EQ(log.init(), FlashLogError::OK);
        auto record = log.createRecord();
        ASSERT_EQ(record.field(7, &first, sizeof(first)), FlashLogError::OK);
    }

    RecordLog reopened(flash);              // fresh object, same flash
    ASSERT_EQ(reopened.init(), FlashLogError::OK);

    uint32_t second = 0x55667788;
    {
        auto record = reopened.createRecord();
        ASSERT_EQ(record.field(8, &second, sizeof(second)), FlashLogError::OK);
    }

    auto reader = reopened.firstRecord();   // record 1 must be untouched
    uint32_t out = 0;
    EXPECT_EQ(reader.read(7, &out, sizeof(out)), FlashLogError::OK);
    EXPECT_EQ(out, 0x11223344u);
}

TEST(RecordLog, next_moves_to_the_following_record) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t first = 0x11223344;
    {
        auto record = log.createRecord();
        ASSERT_EQ(record.field(7, &first, sizeof(first)), FlashLogError::OK);
    }
    uint32_t second = 0x55667788;
    {
        auto record = log.createRecord();
        ASSERT_EQ(record.field(8, &second, sizeof(second)), FlashLogError::OK);
    }

    auto reader = log.firstRecord();
    uint32_t out = 0;
    ASSERT_EQ(reader.read(7, &out, sizeof(out)), FlashLogError::OK);
    ASSERT_EQ(out, 0x11223344u);

    ASSERT_EQ(reader.next(), FlashLogError::OK);
    EXPECT_EQ(reader.read(8, &out, sizeof(out)), FlashLogError::OK);
    EXPECT_EQ(out, 0x55667788u);
    EXPECT_NE(reader.read(7, &out, sizeof(out)), FlashLogError::OK);  // record 1 is behind us
}

TEST(RecordLog, next_reports_the_end_of_the_log) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    {
        auto record = log.createRecord();
        ASSERT_EQ(record.field(7, &value, sizeof(value)), FlashLogError::OK);
    }

    auto reader = log.firstRecord();
    EXPECT_EQ(reader.next(), FlashLogError::END_OF_LOG);
}

// A crash leaves a torn record behind, and records written afterwards sit after
// it — so a torn record in the middle must not hide them.
TEST(RecordLog, next_skips_a_torn_record) {
    RamFlash<4096, 256> flash;

    uint32_t first = 0x11223344;
    {
        RecordLog log(flash);
        ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
        ASSERT_EQ(log.init(), FlashLogError::OK);
        auto record = log.createRecord();
        ASSERT_EQ(record.field(7, &first, sizeof(first)), FlashLogError::OK);
    }

    // Power cut: marker and field on flash, close() never runs. The handle is
    // deliberately leaked so no destructor commits it.
    RecordLog crashed(flash);
    ASSERT_EQ(crashed.init(), FlashLogError::OK);
    uint32_t lost = 0x99999999;
    auto* torn = new RecordWriter(crashed.createRecord());
    ASSERT_EQ(torn->field(8, &lost, sizeof(lost)), FlashLogError::OK);

    uint32_t third = 0x55667788;
    RecordLog reopened(flash);
    ASSERT_EQ(reopened.init(), FlashLogError::OK);
    {
        auto record = reopened.createRecord();
        ASSERT_EQ(record.field(9, &third, sizeof(third)), FlashLogError::OK);
    }

    auto reader = reopened.firstRecord();
    uint32_t out = 0;
    ASSERT_EQ(reader.read(7, &out, sizeof(out)), FlashLogError::OK);   // record 1
    ASSERT_EQ(out, 0x11223344u);

    ASSERT_EQ(reader.next(), FlashLogError::OK);                       // skips the torn one
    EXPECT_EQ(reader.read(9, &out, sizeof(out)), FlashLogError::OK);
    EXPECT_EQ(out, 0x55667788u);
}

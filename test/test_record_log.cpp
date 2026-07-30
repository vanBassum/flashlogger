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

// The "never derails on any flash contents" promise, smallest version: a read
// over a stuffed store must return rather than spin. The fill is a bounded count
// rather than "write until it refuses" — in a ring, writes never refuse.
TEST(RecordLog, a_read_over_a_stuffed_store_terminates) {
    RamFlash<1024, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    for (int i = 0; i < 200; i++) {          // 4 sectors x 49 fields = 196 fields
        auto record = log.createRecord();
        ASSERT_EQ(record.field(7, &value, sizeof(value)), FlashLogError::OK);
    }

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

// A corrupt record is reported, but it must not wall off the records behind it:
// next() still moves past it. Only the caller decides whether to stop.
TEST(RecordLog, a_corrupt_record_does_not_stop_iteration) {
    RamFlash<4096, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t first = 0x11223344, second = 0x55667788, third = 0x0A0B0C0D;
    {
        auto record = log.createRecord();
        ASSERT_EQ(record.field(7, &first, sizeof(first)), FlashLogError::OK);
    }
    {
        auto record = log.createRecord();
        ASSERT_EQ(record.field(8, &second, sizeof(second)), FlashLogError::OK);
    }
    {
        auto record = log.createRecord();
        ASSERT_EQ(record.field(9, &third, sizeof(third)), FlashLogError::OK);
    }

    // Rot a byte of record 2's value: field index 3, at 8 (sector header) + 3*5,
    // one byte in for the value.
    uint8_t cleared = 0x00;
    flash.write(8 + 3 * 5 + 1, &cleared, 1, 0);

    auto reader = log.firstRecord();
    uint32_t out = 0;
    ASSERT_EQ(reader.read(7, &out, sizeof(out)), FlashLogError::OK);

    ASSERT_EQ(reader.next(), FlashLogError::OK);
    EXPECT_EQ(reader.read(8, &out, sizeof(out)), FlashLogError::RECORD_CORRUPT);

    ASSERT_EQ(reader.next(), FlashLogError::OK);        // past the corrupt one
    EXPECT_EQ(reader.read(9, &out, sizeof(out)), FlashLogError::OK);
    EXPECT_EQ(out, 0x0A0B0C0Du);
}

// A ring keeps accepting data forever: filling up reclaims the oldest sector
// rather than refusing the write. 4 sectors x 49 fields = 196 fields, and a
// one-field record costs 2 (marker + data), so 200 records forces two laps.
TEST(RecordLog, the_log_keeps_accepting_records_when_it_fills_up) {
    RamFlash<1024, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    for (uint32_t i = 1; i <= 200; i++) {
        auto record = log.createRecord();
        ASSERT_EQ(record.field(7, &i, sizeof(i)), FlashLogError::OK) << "at record " << i;
    }
}

// Reading has to go round the corner too. Once the log has wrapped, iteration
// must start at the OLDEST surviving record and walk the whole ring in age
// order — not start at slot 0 and stop at the append point, which sees only the
// handful of records written since the last wrap. Steps are capped so a walk
// that fails to terminate fails the test instead of hanging it.
TEST(RecordLog, a_wrapped_log_reads_oldest_first) {
    RamFlash<1024, 256> flash;
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    for (uint32_t i = 1; i <= 200; i++) {     // 196 fields of room, so this wraps
        auto record = log.createRecord();
        ASSERT_EQ(record.field(7, &i, sizeof(i)), FlashLogError::OK);
    }

    auto reader = log.firstRecord();
    uint32_t previous = 0, out = 0;
    int seen = 0;
    for (int steps = 0; steps < 500; steps++) {
        if (reader.read(7, &out, sizeof(out)) == FlashLogError::OK) {
            EXPECT_GT(out, previous) << "record " << seen << " is out of age order";
            previous = out;
            seen++;
        }
        if (reader.next() != FlashLogError::OK)
            break;
    }

    EXPECT_GT(seen, 50) << "iteration only reached " << seen << " records";
    EXPECT_EQ(previous, 200u);                // and it ends on the newest
}

// Turning it off and on again after the log has gone round: init() has to find
// the append point wherever it ended up, not assume the first lap.
TEST(RecordLog, reopening_a_wrapped_log_keeps_going) {
    RamFlash<1024, 256> flash;

    {
        RecordLog log(flash);
        ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
        ASSERT_EQ(log.init(), FlashLogError::OK);
        for (uint32_t i = 1; i <= 200; i++) {      // wraps
            auto record = log.createRecord();
            ASSERT_EQ(record.field(7, &i, sizeof(i)), FlashLogError::OK);
        }
    }

    RecordLog reopened(flash);                     // fresh object, same flash
    ASSERT_EQ(reopened.init(), FlashLogError::OK);

    uint32_t after_reboot = 201;
    {
        auto record = reopened.createRecord();
        ASSERT_EQ(record.field(7, &after_reboot, sizeof(after_reboot)), FlashLogError::OK);
    }

    auto reader = reopened.firstRecord();
    uint32_t previous = 0, out = 0;
    int seen = 0;
    for (int steps = 0; steps < 500; steps++) {
        if (reader.read(7, &out, sizeof(out)) == FlashLogError::OK) {
            EXPECT_GT(out, previous) << "out of age order at record " << seen;
            previous = out;
            seen++;
        }
        if (reader.next() != FlashLogError::OK)
            break;
    }

    EXPECT_GT(seen, 50) << "only reached " << seen << " records";
    EXPECT_EQ(previous, 201u);        // the record written after the reboot
}

// Reclaiming the first sector wipes the format header, and it is written back
// immediately after. This test cuts the power in that gap — the erase happened,
// the write did not. The records in the other sectors are all still on the chip,
// so the log must still mount and read them.
TEST(RecordLog, the_log_survives_power_loss_between_erasing_sector_0_and_the_header) {
    RamFlash<1024, 256> flash;

    {
        RecordLog log(flash);
        ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
        ASSERT_EQ(log.init(), FlashLogError::OK);
        for (uint32_t i = 1; i <= 200; i++) {      // wraps, so sector 0 gets reclaimed
            auto record = log.createRecord();
            ASSERT_EQ(record.field(7, &i, sizeof(i)), FlashLogError::OK);
        }
    }

    flash.erase(0, 0);            // power lost right here: header gone, records not

    RecordLog reopened(flash);
    EXPECT_EQ(reopened.init(), FlashLogError::OK);

    auto reader = reopened.firstRecord();
    uint32_t out = 0;
    EXPECT_EQ(reader.read(7, &out, sizeof(out)), FlashLogError::OK);
}

// Power lost part-way through an erase: some of the sector came back to 0xFF, the
// rest still holds old records. That leaves a second hole in the store, so the
// scan for "where do I write next" can no longer just take the first empty field
// it meets — it could pick the hole and start writing into the middle of the log.
TEST(RecordLog, the_log_recovers_from_an_erase_interrupted_by_power_loss) {
    RamFlash<1024, 256> flash;

    {
        RecordLog log(flash);
        ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
        ASSERT_EQ(log.init(), FlashLogError::OK);
        for (uint32_t i = 1; i <= 150; i++) {     // wraps; cursor ends in sector 2
            auto record = log.createRecord();
            ASSERT_EQ(record.field(7, &i, sizeof(i)), FlashLogError::OK);
        }
    }

    // Half of sector 0 erased, half still holding records: an erase that stopped.
    flash.eraseRangeForTest(0, 128);

    RecordLog reopened(flash);
    ASSERT_EQ(reopened.init(), FlashLogError::OK);

    uint32_t after_crash = 151;
    {
        auto record = reopened.createRecord();
        ASSERT_EQ(record.field(7, &after_crash, sizeof(after_crash)), FlashLogError::OK);
    }

    auto reader = reopened.firstRecord();
    uint32_t previous = 0, out = 0;
    int seen = 0;
    for (int steps = 0; steps < 500; steps++) {
        if (reader.read(7, &out, sizeof(out)) == FlashLogError::OK) {
            EXPECT_GT(out, previous) << "out of age order at record " << seen;
            previous = out;
            seen++;
        }
        if (reader.next() != FlashLogError::OK)
            break;
    }

    EXPECT_EQ(previous, 151u) << "newest record missing; reached " << seen << " records";
}

// The ring needs one sector always erased, one being written, and one holding
// older records. With two, "erase the next sector" erases the one just left, which
// destroys a record still being written. Three is the floor.
TEST(RecordLog, format_refuses_a_flash_with_too_few_sectors) {
    RamFlash<512, 256> flash;                     // 2 sectors
    RecordLog log(flash);
    EXPECT_EQ(log.format(1, 4), FlashLogError::ARG_INVALID);
}

TEST(RecordLog, format_accepts_the_smallest_workable_flash) {
    RamFlash<768, 256> flash;                     // 3 sectors
    RecordLog log(flash);
    EXPECT_EQ(log.format(1, 4), FlashLogError::OK);
    EXPECT_EQ(log.init(), FlashLogError::OK);
}

// A refused format must not have wiped anything on its way out.
TEST(RecordLog, a_format_refused_for_too_few_sectors_erases_nothing) {
    RamFlash<512, 256> flash;
    uint8_t marker[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    flash.write(300, marker, 4, 0);

    RecordLog log(flash);
    EXPECT_EQ(log.format(1, 4), FlashLogError::ARG_INVALID);

    uint8_t back[4] = {0, 0, 0, 0};
    flash.read(300, back, 4, 0);
    EXPECT_EQ(back[0], 0xDE);
    EXPECT_EQ(back[3], 0xEF);
}

// 49 fields per sector and 2 fields per record, so 49 records land the cursor
// exactly on a sector boundary — the tightest case for finding where the log ends.
TEST(RecordLog, the_start_is_still_found_when_the_cursor_sits_on_a_sector_edge) {
    RamFlash<1024, 256> flash;

    {
        RecordLog log(flash);
        ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
        ASSERT_EQ(log.init(), FlashLogError::OK);
        for (uint32_t i = 1; i <= 49; i++) {
            auto record = log.createRecord();
            ASSERT_EQ(record.field(7, &i, sizeof(i)), FlashLogError::OK);
        }
    }

    RecordLog reopened(flash);
    ASSERT_EQ(reopened.init(), FlashLogError::OK);

    uint32_t next_one = 50;
    {
        auto record = reopened.createRecord();
        ASSERT_EQ(record.field(7, &next_one, sizeof(next_one)), FlashLogError::OK);
    }

    auto reader = reopened.firstRecord();
    uint32_t previous = 0, out = 0;
    int seen = 0;
    for (int steps = 0; steps < 500; steps++) {
        if (reader.read(7, &out, sizeof(out)) == FlashLogError::OK) {
            EXPECT_GT(out, previous) << "out of age order at record " << seen;
            previous = out;
            seen++;
        }
        if (reader.next() != FlashLogError::OK)
            break;
    }

    EXPECT_EQ(previous, 50u) << "newest missing; reached " << seen;
    EXPECT_EQ(seen, 50)      << "expected all 50 records, nothing reclaimed yet";
}

// A record long enough to come round the ring would reach its own marker — the
// only way left to lose the boundary that tells us where the log starts. It has to
// be refused before it gets there.
TEST(RecordLog, a_record_cannot_grow_long_enough_to_eat_its_own_start) {
    RamFlash<1024, 256> flash;                    // 196 fields, 49 per sector
    RecordLog log(flash);
    ASSERT_EQ(log.format(1, 4), FlashLogError::OK);
    ASSERT_EQ(log.init(), FlashLogError::OK);

    uint32_t value = 0x11223344;
    auto record = log.createRecord();

    FlashLogError err = FlashLogError::OK;
    int written = 0;
    while (err == FlashLogError::OK && written < 1000) {
        err = record.field(7, &value, sizeof(value));
        if (err == FlashLogError::OK)
            written++;
    }

    EXPECT_EQ(err, FlashLogError::RECORD_TOO_LONG);
    EXPECT_LT(written, 196) << "wrote " << written << " fields into a 196-field ring";
}

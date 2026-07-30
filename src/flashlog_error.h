#pragma once

enum class FlashLogError {
    OK,

    FLASH_READ_ERROR,
    FLASH_WRITE_ERROR,

    FORMAT_MISSING,
    FORMAT_CORRUPT,

    STORE_NOT_INITIALIZED,

    ARG_INVALID,
    ARG_OUT_OF_BOUNDS,

    RECORD_ALREADY_OPEN,
    RECORD_TORN,          // never committed: close() didn't run
    RECORD_CORRUPT,       // a real CRC was written and the data no longer matches it
    END_OF_LOG,           // no further record to move to
    RECORD_TOO_LONG,      // would reach round the ring to its own marker
};

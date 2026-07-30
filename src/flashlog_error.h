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
};

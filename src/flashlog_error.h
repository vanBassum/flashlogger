#pragma once

enum class FlashLogError {
    OK,
    READ_ERROR,
    NOT_FORMATTED,
    WRITE_ERROR,
    UNKNOWN_FORMAT,
    INVALID_ARGUMENT,
    NOT_INITIALIZED,
    FLASH_FULL,
};

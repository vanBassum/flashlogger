cmake_minimum_required(VERSION 3.20)
project(FlashLoggerV2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(flashlogger STATIC
    src/field_store.cpp
)
target_include_directories(flashlogger PUBLIC src)

enable_testing()
add_subdirectory(test)

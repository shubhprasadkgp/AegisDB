cmake_minimum_required(VERSION 3.15)

project(PebbleDB)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(
    pebbledb
    src/main.cpp
    src/db.cpp
    src/memtable.cpp
    src/wal.cpp
    src/sstable.cpp
    src/bloom_filter.cpp
    src/recovery.cpp
    src/compaction.cpp
)
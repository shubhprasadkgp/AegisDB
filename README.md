# AegisDB 🛡️

AegisDB is a high-performance, embedded Log-Structured Merge-Tree (LSM) key-value storage engine written from scratch in modern C++17. 

It is designed to demonstrate core database systems engineering concepts, including concurrent background flushing, Write-Ahead Logging (WAL) for crash durability, size-tiered compaction, and $O(1)$ read path optimization using Bloom Filters and Sparse Indexing.

## 🚀 Key Features

* **Thread-Safe Concurrency:** Utilizes a double-buffered MemTable architecture (Active & Immutable) with `std::mutex` and `std::condition_variable`. Prevents write-blocking during disk flushes using asynchronous background threads and intelligent write-stalling (backpressure).
* **ACID Durability (Crash Recovery):** Implements a split Write-Ahead Log (WAL) system (`wal_active.log` and `wal_flushing.log`). On database startup, it automatically discovers, parses, and replays un-flushed operations from the WALs to restore exact in-memory state.
* **$O(1)$ Negative Lookups:** Integrates a custom **3-seed FNV-1a Bloom Filter** that dynamically sizes to 10-bits per key and serializes to disk (`.bf` files). This probabalistic filter intercepts negative queries in-memory, bypassing expensive disk seeks with 100% certainty.
* **Minimal RAM Footprint:** Uses **Binary-Searched Sparse Indexing**. Instead of indexing every key, it maintains block-level offsets (`.idx` files), reducing the RAM overhead of indexing to less than 1% of the total disk size.
* **Size-Tiered Compaction:** Automatically runs background consolidation when the SSTable count reaches a configured threshold (4 tables). This garbage-collects deleted keys (tombstones) and older key versions to control read amplification and reclaim disk space.
* **Cross-Platform Binary I/O:** Uses strict binary-mode file streaming (`std::ios::binary`) to prevent OS-level newline translation drifts, ensuring accurate byte-offset seeks on Windows, Linux, and macOS.

## 🏗️ Architecture

AegisDB follows the classic LSM-Tree design pattern popularized by LevelDB and RocksDB:

1. **Write Path (`put` / `remove`):** 
   Writes are appended to the active WAL and inserted into the active, in-memory `MemTable` (backed by a Red-Black Tree). Deletes are written as tombstone markers.
2. **Background Flush:**
   When the `MemTable` reaches its byte limit, it is swapped to an immutable state. A background `std::thread` flushes it to disk as a Sorted String Table (`.dat`), along with its Sparse Index (`.idx`) and Bloom Filter (`.bf`) files.
3. **Read Path (`get`):**
   Reads search the active `MemTable`, then the immutable `MemTable`, and finally query the SSTables from newest to oldest. SSTable reads are optimized by checking the Bloom Filter in RAM, binary-searching the Sparse Index in RAM, and executing exactly one disk seek to the data block.

## 🛠️ Build Instructions

AegisDB requires a C++17 compliant compiler and CMake.

```bash
# Clone the repository
git clone https://github.com/yourusername/AegisDB.git
cd AegisDB

# Create build directory and compile
mkdir build && cd build
cmake ..
cmake --build .

# Run the test executable
./pebbledb
```

## 💻 Usage Example

AegisDB is designed to be embedded directly into C++ applications. 

```cpp
#include <iostream>
#include "db.h"

int main() {
    // Initialize the database (automatically recovers state from disk)
    PebbleDB db;

    // Write data
    db.put("user_101", "shubh_prasad");
    db.put("user_102", "engineer");

    // Read data
    Entry result;
    if (db.get("user_101", result) && !result.deleted) {
        std::cout << "Found: " << result.value << "\n";
    } else {
        std::cout << "Key not found.\n";
    }

    // Delete data (writes a tombstone)
    db.remove("user_101");

    return 0;
}
```

## 🧠 What I Learned

Building this engine from the ground up provided deep, hands-on experience with:
* Resolving complex multi-threading deadlocks and race conditions.
* Handling OS-level file locking and managing cross-platform binary stream offsets.
* Making concrete systems trade-offs (e.g., trading slight write latency during write-stalls to guarantee memory safety and prevent Out-Of-Memory crashes).
* Implementing probabilistic data structures (Bloom Filters) in a real-world scenario.

---
*Created as an independent systems engineering project by [Shubh Prasad](https://github.com/shubhprasadkgp).*

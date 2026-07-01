#pragma once

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "sstable.h"
#include "memtable.h"
#include "wal.h"

class PebbleDB {
public:
    PebbleDB();
    ~PebbleDB(); // Destructor to join the background thread safely on shutdown

    void put(const std::string& key, const std::string& value);
    bool get(const std::string& key, Entry& result) const;
    void remove(const std::string& key);

private:
    std::unique_ptr<MemTable> active_memtable;
    std::unique_ptr<MemTable> immutable_memtable;

    std::unique_ptr<WAL> active_wal;
    std::unique_ptr<WAL> inactive_wal;

    int next_sstable_id = 1;
    std::vector<SSTable> sstables;

    // Concurrency variables
    mutable std::mutex db_mutex;
    std::condition_variable flush_cv;
    bool is_flushing = false;
    std::thread flush_thread;

    // Flush execution helpers
    void flush();
    void background_flush(std::string filename);
};
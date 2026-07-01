#pragma once

#include <memory>
#include <string>
#include <vector>

#include "sstable.h"
#include "memtable.h"
#include "wal.h"

class PebbleDB {
public:
    PebbleDB();

    WAL wal;

    void put(const std::string& key, const std::string& value);
    bool get(const std::string& key, Entry& result) const;
    void remove(const std::string& key);

private:
    std::unique_ptr<MemTable> active_memtable;
    void flush();
    int next_sstable_id = 1;
    std::vector<SSTable> sstables;
};


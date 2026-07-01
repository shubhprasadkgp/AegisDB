#include <iostream>

#include "db.h"
#include "sstable.h"

PebbleDB::PebbleDB() {
    active_memtable = std::make_unique<MemTable>(100);

    auto operations = wal.recover();
    for (const auto& op : operations) {
        if (op.type == OperationType::PUT)
            active_memtable->put(op.key, op.value);
        else if (op.type == OperationType::DELETE)
            active_memtable->remove(op.key);
    }
}

void PebbleDB::put(const std::string& key, const std::string& value) {
    wal.append({OperationType::PUT, key, value});
    active_memtable->put(key, value);

    if (active_memtable->isFull())
    flush();
}

bool PebbleDB::get(const std::string& key, Entry& result) const {
    // 1. Search the active memtable
    if (active_memtable->get(key, result)) {
        return true;
    }

    // 2. Search SSTables from newest to oldest
    for (auto it = sstables.rbegin(); it != sstables.rend(); ++it) {
        if (it->get(key, result)) {
            return true;
        }
    }

    return false;
}

void PebbleDB::remove(const std::string& key) {
    wal.append({OperationType::DELETE, key, ""});
    active_memtable->remove(key);
}

void PebbleDB::flush() {
    std::string filename = "data/sst_" + std::to_string(next_sstable_id) + ".dat";

    sstables.emplace_back(filename);
    sstables.back().write(*active_memtable);

    next_sstable_id++;
    active_memtable->clear();
    wal.clear();
    std::cout << "Total SSTables: " << sstables.size() << '\n';
    std::cout << filename << '\n';
}
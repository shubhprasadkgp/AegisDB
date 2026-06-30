#include "db.h"

PebbleDB::PebbleDB() {
    active_memtable = std::make_unique<MemTable>();
}

void PebbleDB::put(const std::string& key, const std::string& value) {
    active_memtable->put(key, value);
}

bool PebbleDB::get(const std::string& key, Entry& result) const {
    return active_memtable->get(key, result);
}

void PebbleDB::remove(const std::string& key) {
    active_memtable->remove(key);
}
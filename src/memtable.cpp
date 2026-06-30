#include "memtable.h"

void MemTable::put(const std::string& key, const std::string& value) {
    table[key] = {value, false};
}

bool MemTable::get(const std::string& key, Entry& result) const {
    auto it = table.find(key);

    if (it == table.end()) {
        return false;
    }

    result = it->second;
    return true;
}

void MemTable::remove(const std::string& key) {
    table[key].deleted = true;
}
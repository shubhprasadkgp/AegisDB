#include "memtable.h"

MemTable::MemTable(size_t max_size)
    : current_size(0), max_size(max_size) {}

void MemTable::put(const std::string& key, const std::string& value) {
    table[key] = {value, false};
    current_size += key.size() + value.size();
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

bool MemTable::isFull() const {
    return current_size >= max_size;
}

void MemTable::clear() {
    table.clear();
    current_size = 0;
}

const std::map<std::string, Entry>& MemTable::getTable() const {
    return table;
}
#pragma once

#include <map>
#include <string>
#include <cstddef>

struct Entry {
    std::string value;
    bool deleted;
};

class MemTable {
public:
    MemTable(size_t max_size);

    void put(const std::string& key, const std::string& value);
    bool get(const std::string& key, Entry& result) const;
    void remove(const std::string& key);

    bool isFull() const;
    void clear();

    const std::map<std::string, Entry>& getTable() const;

private:
    std::map<std::string, Entry> table;
    size_t current_size;
    size_t max_size;
};
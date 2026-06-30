#pragma once

#include <map>
#include <string>

struct Entry {
    std::string value;
    bool deleted;
};

class MemTable {
public:
    void put(const std::string& key, const std::string& value);
    bool get(const std::string& key, Entry& result) const;
    void remove(const std::string& key);

private:
    std::map<std::string, Entry> table;
};
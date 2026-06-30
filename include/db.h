#pragma once

#include <memory>
#include <string>

#include "memtable.h"

class PebbleDB {
public:
    PebbleDB();

    void put(const std::string& key, const std::string& value);
    bool get(const std::string& key, Entry& result) const;
    void remove(const std::string& key);

private:
    std::unique_ptr<MemTable> active_memtable;
};
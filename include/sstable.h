#pragma once

#include <vector>
#include <utility>
#include <ios>

#include "memtable.h"


class SSTable {
public:
    SSTable(const std::string& data_filename);

    void write(const MemTable& memtable);
    bool get(const std::string& key, Entry& result) const;



private:
    std::string data_filename;
    std::string index_filename;
    std::vector<std::pair<std::string, std::streampos>> sparse_index;

    void load_index();
};
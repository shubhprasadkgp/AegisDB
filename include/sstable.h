#pragma once

#include "memtable.h"
#include "bloom_filter.h"
#include <vector>
#include <utility>
#include <ios>

class SSTable {
public:
    SSTable(const std::string& data_filename);

    void write(const MemTable& memtable);
    bool get(const std::string& key, Entry& result) const;

    // Public getters for file paths (needed for compaction cleanup)
    std::string getDataFilename() const { return data_filename; }
    std::string getIndexFilename() const { return index_filename; }
    std::string getFilterFilename() const { return filter_filename; }

private:
    std::string data_filename;
    std::string index_filename;
    std::string filter_filename;
    
    std::vector<std::pair<std::string, std::streampos>> sparse_index;
    BloomFilter bloom_filter;

    void load_index();
    void load_filter();
};
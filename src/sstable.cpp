#include "sstable.h"
#include <fstream>
#include <algorithm>
#include <sstream>
#include <iostream>

constexpr int INDEX_INTERVAL = 3;

SSTable::SSTable(const std::string& data_filename)
    : data_filename(data_filename)
{
    index_filename = data_filename;
    filter_filename = data_filename;

    index_filename.replace(
        index_filename.find(".dat"),
        4,
        ".idx"
    );
    
    filter_filename.replace(
        filter_filename.find(".dat"),
        4,
        ".bf"
    );
    
    load_index();
    load_filter();
}

void SSTable::write(const MemTable& memtable) {
    std::ofstream data_file(data_filename, std::ios::binary);
    std::ofstream index_file(index_filename, std::ios::binary);

    int count = 0;
    
    // Create Bloom Filter based on expected number of keys in the MemTable
    BloomFilter filter(memtable.getTable().size());

    for (const auto& pair : memtable.getTable()) {
        if (pair.second.deleted)
            data_file << "DELETE " << pair.first << '\n';
        else
            data_file << pair.first << " " << pair.second.value << '\n';
        
        // Add key to Bloom Filter (including tombstones)
        filter.add(pair.first);

        count++;
        if (count % INDEX_INTERVAL == 0) {
            index_file << pair.first << " " << data_file.tellp() << '\n';
        }
    }

    data_file.close();
    index_file.close();
    
    // Serialize Bloom Filter to disk in binary format
    std::ofstream filter_file(filter_filename, std::ios::binary);
    if (filter_file.is_open()) {
        size_t num_bits = filter.getBitSize();
        filter_file.write(reinterpret_cast<const char*>(&num_bits), sizeof(num_bits));
        
        std::vector<uint8_t> bytes = filter.serialize();
        filter_file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        filter_file.close();
    }
    
    load_index();
    load_filter();
}

void SSTable::load_index() {
    sparse_index.clear();
    std::ifstream index_file(index_filename, std::ios::binary);
    if (!index_file.is_open()) {
        return;
    }

    std::string key;
    std::streamoff offset;
    while (index_file >> key >> offset) {
        sparse_index.emplace_back(key, offset);
    }
    index_file.close();
}

bool SSTable::get(const std::string& key, Entry& result) const {
    // 1. Check Bloom Filter first (avoids opening files/doing seeks if not present)
    if (!bloom_filter.mightContain(key)) {
        std::cout << "  [BloomFilter] '" << key << "' definitely NOT in " << data_filename << " (Bypassed disk scan)\n";
        return false;
    }

    std::cout << "  [BloomFilter] '" << key << "' MIGHT be in " << data_filename << " (Proceeding to disk seek...)\n";

    if (sparse_index.empty()) {
        return false;
    }

        // Binary search for the first index entry that is greater than or equal to the key
    auto it = std::lower_bound(
        sparse_index.begin(),
        sparse_index.end(),
        key,
        [](const std::pair<std::string, std::streampos>& entry, const std::string& k) {
            return entry.first < k;
        }
    );

    // If the iterator points to the first entry, the key is in the very first block (offset 0).
    // Otherwise, it starts from the end of the previous block.
    std::streampos start_offset = 0;
    if (it != sparse_index.begin()) {
        start_offset = std::prev(it)->second;
    }

    std::ifstream data_file(data_filename, std::ios::binary);
    if (!data_file.is_open()) {
        return false;
    }

    data_file.seekg(start_offset);

    std::string line;
    while (std::getline(data_file, line)) {
        std::stringstream ss(line);
        std::string current_key;
        ss >> current_key;

        if (current_key == "DELETE") {
            std::string deleted_key;
            ss >> deleted_key;
            if (deleted_key == key) {
                result.deleted = true;
                result.value = "";
                return true;
            } else if (deleted_key > key) {
                break; // Since SSTable is sorted, we can stop early
            }
        } else {
            if (current_key == key) {
                result.deleted = false;
                ss >> result.value;
                return true;
            } else if (current_key > key) {
                break; // Since SSTable is sorted, we can stop early
            }
        }
    }

    return false;
}

void SSTable::load_filter() {
    std::ifstream filter_file(filter_filename, std::ios::binary);
    if (!filter_file.is_open()) {
        return;
    }

    size_t num_bits = 0;
    filter_file.read(reinterpret_cast<char*>(&num_bits), sizeof(num_bits));

    std::vector<uint8_t> bytes;
    char byte;
    while (filter_file.read(&byte, 1)) {
        bytes.push_back(static_cast<uint8_t>(byte));
    }
    filter_file.close();

    bloom_filter = BloomFilter(num_bits, bytes);
}
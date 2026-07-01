#include "sstable.h"
#include <fstream>
#include <algorithm>
#include <sstream>

constexpr int INDEX_INTERVAL = 3;

SSTable::SSTable(const std::string& data_filename)
    : data_filename(data_filename)
{
    index_filename = data_filename;

    index_filename.replace(
        index_filename.find(".dat"),
        4,
        ".idx"
    );
    load_index();
}

void SSTable::write(const MemTable& memtable) {
    std::ofstream data_file(data_filename);
    std::ofstream index_file(index_filename);

    int count = 0;

    for (const auto& pair : memtable.getTable()) {
        if (pair.second.deleted)
            data_file << "DELETE " << pair.first << '\n';
        else
            data_file << pair.first << " " << pair.second.value << '\n';

        count++;
        if (count % INDEX_INTERVAL == 0) {
            index_file << pair.first << " " << data_file.tellp() << '\n';
        }
    }

    data_file.close();
    index_file.close();
    load_index();
}

void SSTable::load_index() {
    sparse_index.clear();
    std::ifstream index_file(index_filename);
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
    if (sparse_index.empty()) {
        return false;
    }

    // Binary search for the first index entry that is strictly greater than the key
    auto it = std::upper_bound(
        sparse_index.begin(),
        sparse_index.end(),
        key,
        [](const std::string& k, const std::pair<std::string, std::streampos>& entry) {
            return k < entry.first;
        }
    );

    // If upper_bound returns the start, the target key is smaller than the first index entry.
    // In that case, we start scanning from the beginning of the file (offset 0).
    // Otherwise, we start from the previous index entry.
    std::streampos start_offset = 0;
    if (it != sparse_index.begin()) {
        start_offset = std::prev(it)->second;
    }

    std::ifstream data_file(data_filename);
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
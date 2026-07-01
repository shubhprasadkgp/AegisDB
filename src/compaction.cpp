#include "compaction.h"
#include "memtable.h"
#include "sstable.h"
#include <fstream>
#include <sstream>
#include <map>

void compact_sstables(const std::vector<std::string>& input_files, const std::string& output_file) {
    std::map<std::string, Entry> merged_map;

    // 1. Read all data from input SSTables (oldest to newest)
    for (const auto& filepath : input_files) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            continue;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) {
                continue;
            }

            std::stringstream ss(line);
            std::string key;
            ss >> key;

            if (key == "DELETE") {
                std::string deleted_key;
                ss >> deleted_key;
                merged_map[deleted_key] = {"", true};
            } else {
                std::string value;
                ss >> value;
                merged_map[key] = {value, false};
            }
        }
        file.close();
    }

    // 2. Populate a temporary MemTable, discarding tombstones to reclaim space
    // We set a large max size limit since this is a one-time compaction write.
    MemTable temp_memtable(1024 * 1024 * 100); 
    for (const auto& pair : merged_map) {
        if (!pair.second.deleted) {
            temp_memtable.put(pair.first, pair.second.value);
        }
    }

    // 3. Write using SSTable class (which automatically handles sparse index and bloom filters)
    SSTable output_sstable(output_file);
    output_sstable.write(temp_memtable);
}
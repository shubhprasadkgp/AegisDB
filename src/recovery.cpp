#include "recovery.h"
#include <filesystem>
#include <algorithm>
#include <iostream>

std::vector<std::string> discover_sstables(const std::string& directory) {
    std::vector<std::pair<int, std::string>> id_path_pairs;

    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directory(directory);
        return {};
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            
            // Check if filename matches "sst_[id].dat"
            if (filename.rfind("sst_", 0) == 0 && 
                filename.length() > 8 && 
                filename.substr(filename.length() - 4) == ".dat") {
                
                std::string id_str = filename.substr(4, filename.length() - 8);
                
                // Ensure id_str is purely numeric
                if (std::all_of(id_str.begin(), id_str.end(), ::isdigit)) {
                    int id = std::stoi(id_str);
                    id_path_pairs.push_back({id, entry.path().string()});
                }
            }
        }
    }

    // Sort by ID to ensure correct chronological order
    std::sort(id_path_pairs.begin(), id_path_pairs.end());

    std::vector<std::string> sorted_paths;
    for (const auto& pair : id_path_pairs) {
        sorted_paths.push_back(pair.second);
    }

    return sorted_paths;
}
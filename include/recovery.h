#pragma once

#include <string>
#include <vector>

// Returns a list of sorted SSTable data file paths (e.g. {"data/sst_1.dat", "data/sst_2.dat"})
std::vector<std::string> discover_sstables(const std::string& directory);
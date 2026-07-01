#pragma once

#include <vector>
#include <string>

// Merges multiple input SSTables into a single output SSTable, discarding tombstones
void compact_sstables(const std::vector<std::string>& input_files, const std::string& output_file);
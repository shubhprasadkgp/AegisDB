#pragma once

#include <string>
#include <vector>
#include <cstdint>

class BloomFilter {
public:
    // Default constructor (uninitialized / empty filter)
    BloomFilter();

    // Constructor to initialize an empty filter for N expected keys
    BloomFilter(size_t num_keys);
    
    // Constructor to reconstruct a Bloom Filter loaded from disk bytes
    BloomFilter(size_t num_bits, const std::vector<uint8_t>& bytes);

    // Adds a key to the filter
    void add(const std::string& key);

    // Returns false if key is definitely not present, true if it might be present
    bool mightContain(const std::string& key) const;

    // Helpers for serialization
    size_t getBitSize() const;
    std::vector<uint8_t> serialize() const;

private:
    size_t num_bits;
    std::vector<bool> bits;

    // FNV-1a Hash function
    uint32_t fnv1a(const std::string& key, uint32_t seed) const;
};
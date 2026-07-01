#include "bloom_filter.h"
#include <algorithm>

// Three distinct seeds for FNV-1a hash functions
constexpr uint32_t SEEDS[] = {0x811c9dc5, 0xa3a3a3a3, 0x12345678};

BloomFilter::BloomFilter() : num_bits(0) {}

BloomFilter::BloomFilter(size_t num_keys) {
    // 10 bits per key, with a minimum of 8 bits
    num_bits = std::max(static_cast<size_t>(8), num_keys * 10);
    bits.assign(num_bits, false);
}

BloomFilter::BloomFilter(size_t num_bits, const std::vector<uint8_t>& bytes) 
    : num_bits(num_bits) {
    bits.resize(num_bits, false);
    for (size_t i = 0; i < num_bits; ++i) {
        size_t byte_idx = i / 8;
        size_t bit_idx = i % 8;
        if (byte_idx < bytes.size()) {
            bits[i] = (bytes[byte_idx] & (1 << bit_idx)) != 0;
        }
    }
}

uint32_t BloomFilter::fnv1a(const std::string& key, uint32_t seed) const {
    uint32_t hash = seed;
    for (char c : key) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619; // FNV-1a 32-bit prime multiplier
    }
    return hash;
}

void BloomFilter::add(const std::string& key) {
    if (num_bits == 0) return;
    for (uint32_t seed : SEEDS) {
        uint32_t hash = fnv1a(key, seed);
        bits[hash % num_bits] = true;
    }
}

bool BloomFilter::mightContain(const std::string& key) const {
    if (num_bits == 0) return true; // If filter is inactive/empty, assume key might exist (safe fallback)
    for (uint32_t seed : SEEDS) {
        uint32_t hash = fnv1a(key, seed);
        if (!bits[hash % num_bits]) {
            return false; // Definitely not present
        }
    }
    return true; // Might be present
}

size_t BloomFilter::getBitSize() const {
    return num_bits;
}

std::vector<uint8_t> BloomFilter::serialize() const {
    size_t num_bytes = (num_bits + 7) / 8;
    std::vector<uint8_t> bytes(num_bytes, 0);
    for (size_t i = 0; i < num_bits; ++i) {
        if (bits[i]) {
            size_t byte_idx = i / 8;
            size_t bit_idx = i % 8;
            bytes[byte_idx] |= (1 << bit_idx);
        }
    }
    return bytes;
}
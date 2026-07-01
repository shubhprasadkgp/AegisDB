#include <iostream>
#include <filesystem>
#include <algorithm>

#include "db.h"
#include "sstable.h"
#include "recovery.h"
#include "compaction.h"

PebbleDB::PebbleDB() {
    active_memtable = std::make_unique<MemTable>(100);

    // 1. Discover and load existing SSTables from disk
    auto sstable_files = discover_sstables("data");
    int max_id = 0;
    for (const auto& file : sstable_files) {
        sstables.emplace_back(file);
        
        // Parse the ID from path (e.g. "data/sst_3.dat")
        size_t start = file.find("sst_");
        size_t end = file.find(".dat");
        if (start != std::string::npos && end != std::string::npos) {
            int id = std::stoi(file.substr(start + 4, end - (start + 4)));
            if (id > max_id) {
                max_id = id;
            }
        }
    }
    
    // Resume next SSTable ID based on highest loaded index
    next_sstable_id = max_id + 1;
    std::cout << "Loaded " << sstables.size() << " existing SSTables. Next SSTable ID: " << next_sstable_id << "\n";

    // 2. Discover and replay any left-over WAL files on startup
    std::vector<std::string> wal_files;
    if (std::filesystem::exists("data")) {
        for (const auto& entry : std::filesystem::directory_iterator("data")) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                // Match files like "wal_active.log" or "wal_flushing.log"
                if (filename.rfind("wal_", 0) == 0 && 
                    filename.length() > 8 && 
                    filename.substr(filename.length() - 4) == ".log") {
                    wal_files.push_back(entry.path().string());
                }
            }
        }
    }
    
    // Sort chronologically to replay in order
    std::sort(wal_files.begin(), wal_files.end());

    for (const auto& wal_file : wal_files) {
        std::cout << "Replaying left-over WAL file: " << wal_file << "\n";
        {
            WAL temp_wal(wal_file);
            auto operations = temp_wal.recover();
            for (const auto& op : operations) {
                if (op.type == OperationType::PUT)
                    active_memtable->put(op.key, op.value);
                else if (op.type == OperationType::DELETE)
                    active_memtable->remove(op.key);
            }
        } // temp_wal goes out of scope and is destroyed here, releasing the file handle on disk
        
        // Delete replayed WAL so it won't be replayed on subsequent boots
        std::filesystem::remove(wal_file);
    }

    // 3. Initialize a fresh active WAL for new writes
    active_wal = std::make_unique<WAL>("data/wal_active.log");
}

PebbleDB::~PebbleDB() {
    // Wait for the background thread to exit cleanly on database shutdown
    if (flush_thread.joinable()) {
        flush_thread.join();
    }
}

void PebbleDB::put(const std::string& key, const std::string& value) {
    // Join the finished background thread OUTSIDE the lock to avoid deadlocks
    if (flush_thread.joinable()) {
        flush_thread.join();
    }

    std::unique_lock<std::mutex> lock(db_mutex);

    // Stalling / Backpressure:
    while (active_memtable->isFull() && is_flushing) {
        std::cout << "[WriteStall] Active MemTable full. Waiting for background flush to finish...\n";
        flush_cv.wait(lock);
    }

    active_wal->append({OperationType::PUT, key, value});
    active_memtable->put(key, value);

    if (active_memtable->isFull()) {
        flush();
    }
}

bool PebbleDB::get(const std::string& key, Entry& result) const {
    std::lock_guard<std::mutex> lock(db_mutex);

    // 1. Search the active memtable
    if (active_memtable->get(key, result)) {
        return true;
    }

    // 2. Search the immutable memtable (if a background flush is running)
    if (immutable_memtable && immutable_memtable->get(key, result)) {
        return true;
    }

    // 3. Search SSTables from newest to oldest
    for (auto it = sstables.rbegin(); it != sstables.rend(); ++it) {
        if (it->get(key, result)) {
            return true;
        }
    }

    return false;
}

void PebbleDB::remove(const std::string& key) {
    // Join the finished background thread OUTSIDE the lock to avoid deadlocks
    if (flush_thread.joinable()) {
        flush_thread.join();
    }

    std::unique_lock<std::mutex> lock(db_mutex);

    // Stalling / Backpressure
    while (active_memtable->isFull() && is_flushing) {
        std::cout << "[WriteStall] Active MemTable full. Waiting for background flush to finish...\n";
        flush_cv.wait(lock);
    }

    active_wal->append({OperationType::DELETE, key, ""});
    active_memtable->remove(key);

    if (active_memtable->isFull()) {
        flush();
    }
}

void PebbleDB::flush() {
    // 1. Move active MemTable to immutable state
    immutable_memtable = std::move(active_memtable);

    // Save the filepath and release the file handle by deleting the WAL object
    std::string old_wal_path = active_wal->getFilepath();
    active_wal.reset(); // Destructor runs, closing the active WAL file handle

    // 2. Rename active WAL to flushing WAL on disk
    std::filesystem::rename(old_wal_path, "data/wal_flushing.log");

    // 3. Create fresh active MemTable, active WAL, and inactive WAL
    active_memtable = std::make_unique<MemTable>(100);
    active_wal = std::make_unique<WAL>("data/wal_active.log");
    inactive_wal = std::make_unique<WAL>("data/wal_flushing.log");

    // 4. Mark flush status as active and launch the background worker thread
    is_flushing = true;
    std::string sstable_filename = "data/sst_" + std::to_string(next_sstable_id) + ".dat";
    
    flush_thread = std::thread(&PebbleDB::background_flush, this, sstable_filename);
}

void PebbleDB::background_flush(std::string sstable_filename) {
    // Write the immutable memtable to disk (slow I/O runs OUTSIDE of the mutex lock)
    SSTable new_sstable(sstable_filename);
    new_sstable.write(*immutable_memtable);

    // Lock the database mutex briefly to update the global memory state
    {
        std::lock_guard<std::mutex> lock(db_mutex);

        // Add to active SSTables
        sstables.push_back(std::move(new_sstable));
        next_sstable_id++;

        // Delete the flushed WAL log
        std::string wal_path = inactive_wal->getFilepath();
        inactive_wal.reset(); // closes stream
        std::filesystem::remove(wal_path);

        // Clear the immutable MemTable pointer
        immutable_memtable.reset();

        // Release the flush state
        is_flushing = false;

        std::cout << "\n[BackgroundFlush] Completed write to: " << sstable_filename << "\n";
        std::cout << "[BackgroundFlush] Total SSTables: " << sstables.size() << "\n\n";

        // Trigger compaction if we have 4 or more SSTables
        if (sstables.size() >= 4) {
            std::cout << "\n--- Starting Compaction ---\n";
            
            std::vector<std::string> input_files;
            std::vector<std::string> files_to_delete;
            for (const auto& sstable : sstables) {
                input_files.push_back(sstable.getDataFilename());
                files_to_delete.push_back(sstable.getDataFilename());
                files_to_delete.push_back(sstable.getIndexFilename());
                files_to_delete.push_back(sstable.getFilterFilename());
            }

            std::string compacted_filename = "data/sst_" + std::to_string(next_sstable_id) + ".dat";
            
            compact_sstables(input_files, compacted_filename);
            next_sstable_id++;

            for (const auto& path : files_to_delete) {
                std::filesystem::remove(path);
            }

            sstables.clear();
            sstables.emplace_back(compacted_filename);

            std::cout << "Compaction complete. Merged 4 SSTables into: " << compacted_filename << "\n";
            std::cout << "Total SSTables: " << sstables.size() << "\n\n";
        }
    }

    // Wake up any threads stalled on a Write Stall
    flush_cv.notify_all();
}
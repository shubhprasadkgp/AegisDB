#include <iostream>
#include "db.h"

int main() {
    PebbleDB db;

    db.put("k1", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    db.put("k2", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    db.put("k3", "cccccccccccccccccccccccccccccc");
    db.put("k4", "dddddddddddddddddddddddddddddd");

    db.put("k5", "eeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
    db.put("k6", "ffffffffffffffffffffffffffffff");
    db.put("k7", "gggggggggggggggggggggggggggggg");
    db.put("k8", "hhhhhhhhhhhhhhhhhhhhhhhhhhhhhh");

    db.put("k9", "iiiiiiiiiiiiiiiiiiiiiiiiiiiiii");
    db.put("k10", "jjjjjjjjjjjjjjjjjjjjjjjjjjjjj");
    db.put("k11", "kkkkkkkkkkkkkkkkkkkkkkkkkkkkk");
    db.put("k12", "lllllllllllllllllllllllllllll");

    std::cout << "\n--- Verifying SSTable Reads ---\n";
    Entry result;

    // Test 1: Query "k2" (should be in sst_1.dat)
    if (db.get("k2", result)) {
        if (result.deleted) {
            std::cout << "k2 found but marked DELETED\n";
        } else {
            std::cout << "k2 found: " << result.value << "\n";
        }
    } else {
        std::cout << "k2 NOT found\n";
    }

    // Test 2: Query "k6" (should be in sst_2.dat)
    if (db.get("k6", result)) {
        if (result.deleted) {
            std::cout << "k6 found but marked DELETED\n";
        } else {
            std::cout << "k6 found: " << result.value << "\n";
        }
    } else {
        std::cout << "k6 NOT found\n";
    }

    // Test 3: Query "k11" (should be in sst_3.dat)
    if (db.get("k11", result)) {
        if (result.deleted) {
            std::cout << "k11 found but marked DELETED\n";
        } else {
            std::cout << "k11 found: " << result.value << "\n";
        }
    } else {
        std::cout << "k11 NOT found\n";
    }

    // Test 4: Query "k100" (should NOT exist)
    if (db.get("k100", result)) {
        std::cout << "k100 found unexpectedly!\n";
    } else {
        std::cout << "k100 NOT found (Correct)\n";
    }

    // Test 5: Delete "k6", and verify we can't get it anymore
    std::cout << "\n--- Verifying Deletes ---\n";
    db.remove("k6"); // This goes to memtable
    if (db.get("k6", result)) {
        if (result.deleted) {
            std::cout << "k6 is now correctly marked DELETED\n";
        } else {
            std::cout << "k6 found with value: " << result.value << " (Failed)\n";
        }
    } else {
        std::cout << "k6 NOT found\n";
    }

    return 0;
}
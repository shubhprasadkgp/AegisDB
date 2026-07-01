#include <iostream>
#include "db.h"

void run_compaction_session() {
    std::cout << "=== SESSION 1: Writing Data to Trigger Compaction ===\n";
    PebbleDB db;

    // Batch 1 -> Will flush to sst_1.dat
    std::cout << "\nWriting Batch 1...\n";
    db.put("k1", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    db.put("k2", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    db.put("k3", "cccccccccccccccccccccccccccccc");
    db.put("k4", "dddddddddddddddddddddddddddddd"); // triggers flush

    // Batch 2 -> Will flush to sst_2.dat
    std::cout << "\nWriting Batch 2...\n";
    db.put("k5", "eeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
    db.put("k6", "ffffffffffffffffffffffffffffff");
    db.put("k7", "gggggggggggggggggggggggggggggg");
    db.put("k8", "hhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"); // triggers flush

    // Batch 3 -> Will flush to sst_3.dat
    std::cout << "\nWriting Batch 3...\n";
    db.put("k9", "iiiiiiiiiiiiiiiiiiiiiiiiiiiiii");
    db.put("k10", "jjjjjjjjjjjjjjjjjjjjjjjjjjjjj");
    db.put("k11", "kkkkkkkkkkkkkkkkkkkkkkkkkkkkk");
    db.put("k12", "lllllllllllllllllllllllllllll"); // triggers flush

    // Batch 4 -> Will flush to sst_4.dat, which triggers compaction!
    std::cout << "\nWriting Batch 4...\n";
    db.put("k13", "mmmmmmmmmmmmmmmmmmmmmmmmmmmmm");
    db.put("k14", "nnnnnnnnnnnnnnnnnnnnnnnnnnnnn");
    db.put("k15", "ooooooooooooooooooooooooooooo");
    db.put("k16", "ppppppppppppppppppppppppppppp"); // triggers flush & compaction

    std::cout << "\nSession 1 complete. Database closing.\n\n";
}

void run_verification_session() {
    std::cout << "=== SESSION 2: Recovery and Read Verification ===\n";
    PebbleDB db;

    Entry result;

    std::cout << "\nReading Keys:\n";

    // Test reading keys from all original 4 batches (which are now inside sst_5.dat)
    std::vector<std::string> keys_to_test = {"k2", "k7", "k11", "k15", "k100"};
    for (const auto& key : keys_to_test) {
        if (db.get(key, result)) {
            std::cout << "  Key '" << key << "' found: " << result.value << "\n";
        } else {
            std::cout << "  Key '" << key << "' NOT found (Correct for non-existent keys)\n";
        }
    }

    std::cout << "Session 2 complete.\n";
}

int main() {
    run_compaction_session();
    run_verification_session();
    return 0;
}
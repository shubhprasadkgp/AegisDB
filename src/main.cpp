#include <iostream>
#include "db.h"

int main() {
    PebbleDB db;
    Entry result;

    // Test put()
    db.put("name", "Shubh");
    db.put("college", "IITKGP");

    // Test get()
    if (db.get("name", result))
        std::cout << "Name: " << result.value << '\n';

    if (db.get("college", result))
        std::cout << "College: " << result.value << '\n';

    // Test missing key
    if (!db.get("city", result))
        std::cout << "City not found\n";

    // Test remove()
    db.remove("name");

    if (db.get("name", result))
        std::cout << "Deleted: " << result.deleted << '\n';

    // Test reinsert
    db.put("name", "Shubh Prasad");

    if (db.get("name", result))
        std::cout << result.value << " " << result.deleted << '\n';

    return 0;
}
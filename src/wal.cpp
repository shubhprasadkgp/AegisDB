#include <sstream>
#include <vector>

#include "wal.h"

WAL::WAL() {
    log_file.open("data/wal.log", std::ios::app);
}

void WAL::append(const Operation& op) {
    if (op.type == OperationType::PUT)
        log_file << "PUT " << op.key << " " << op.value << '\n';
    else
        log_file << "DELETE " << op.key << '\n';

    log_file.flush();
}

std::vector<Operation> WAL::recover() {
    std::vector<Operation> operations;

    log_file.close();

    std::ifstream input("data/wal.log");
    std::string line;

    while (std::getline(input, line)) {
        std::stringstream ss(line);

        std::string command;
        ss >> command;

        if (command == "PUT") {
            Operation op;
            op.type = OperationType::PUT;
            ss >> op.key >> op.value;
            operations.push_back(op);
        }
        else if (command == "DELETE") {
            Operation op;
            op.type = OperationType::DELETE;
            ss >> op.key;
            operations.push_back(op);
        }
    }

    input.close();

    log_file.open("data/wal.log", std::ios::app);

    return operations;
}

void WAL::clear() {
    log_file.close();

    std::ofstream file("data/wal.log", std::ios::trunc);
    file.close();

    log_file.open("data/wal.log", std::ios::app);
}
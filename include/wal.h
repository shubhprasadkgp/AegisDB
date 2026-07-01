#pragma once

#include <fstream>
#include <string>
#include <vector>

enum class OperationType {
    PUT,
    DELETE
};

struct Operation {
    OperationType type;
    std::string key;
    std::string value;
};

class WAL {
public:
    WAL();

    void append(const Operation& op);

    std::vector<Operation> recover();

    void clear();

private:
    std::ofstream log_file;
};
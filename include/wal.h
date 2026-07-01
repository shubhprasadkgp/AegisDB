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
    WAL(const std::string& filepath);

    void append(const Operation& op);

    std::vector<Operation> recover();

    void clear();

    std::string getFilepath() const { return filepath; }

private:
    std::string filepath;
    std::ofstream log_file;
};
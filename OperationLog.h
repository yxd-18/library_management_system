#pragma once
#include <string>

struct OperationLog {
    long long logId = 0;
    long long operatorId = 0;
    std::string operationType;
    std::string targetTable;
    long long targetId = 0;
    std::string operationContent;
    std::string ipAddress;
    std::string operationTime;
};

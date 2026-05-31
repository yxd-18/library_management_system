#pragma once
#include <string>
#include <vector>
#include "DatabaseManager.h"
#include "OperationLog.h"

class OperationLogDao {
private:
    DatabaseManager& db;

public:
    OperationLogDao(DatabaseManager& database);

    bool addLog(long long operatorId,
        const std::string& operationType,
        const std::string& targetTable,
        long long targetId,
        const std::string& operationContent,
        const std::string& ipAddress = "");

    std::vector<OperationLog> getAllLogs();
};

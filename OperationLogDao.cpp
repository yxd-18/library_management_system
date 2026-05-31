#include "OperationLogDao.h"

#include <cstdlib>
#include <iostream>
#include <libpq-fe.h>

OperationLogDao::OperationLogDao(DatabaseManager& database) : db(database) {
}

bool OperationLogDao::addLog(long long operatorId,
    const std::string& operationType,
    const std::string& targetTable,
    long long targetId,
    const std::string& operationContent,
    const std::string& ipAddress) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    const std::string operatorIdStr = std::to_string(operatorId);
    const std::string targetIdStr = std::to_string(targetId);

    const char* params[6] = {
        operatorIdStr.c_str(),
        operationType.c_str(),
        targetTable.c_str(),
        targetIdStr.c_str(),
        operationContent.c_str(),
        ipAddress.empty() ? nullptr : ipAddress.c_str()
    };

    PGresult* res = PQexecParams(
        conn,
        "INSERT INTO operation_logs (operator_id, operation_type, target_table, target_id, operation_content, ip_address) VALUES ($1, $2, $3, $4, $5, $6)",
        6,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "写入操作日志失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

std::vector<OperationLog> OperationLogDao::getAllLogs() {
    std::vector<OperationLog> logs;
    PGconn* conn = db.getConnection();
    if (!conn) {
        return logs;
    }

    const char* sql =
        "SELECT log_id, operator_id, operation_type, COALESCE(target_table, ''), "
        "COALESCE(target_id, 0), operation_content, COALESCE(ip_address, ''), "
        "COALESCE(operation_time::text, '') "
        "FROM operation_logs ORDER BY operation_time DESC, log_id DESC";

    PGresult* res = PQexec(conn, sql);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "查询操作日志失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return logs;
    }

    const int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        OperationLog log;
        log.logId = std::atoll(PQgetvalue(res, i, 0));
        log.operatorId = std::atoll(PQgetvalue(res, i, 1));
        log.operationType = PQgetvalue(res, i, 2);
        log.targetTable = PQgetvalue(res, i, 3);
        log.targetId = std::atoll(PQgetvalue(res, i, 4));
        log.operationContent = PQgetvalue(res, i, 5);
        log.ipAddress = PQgetvalue(res, i, 6);
        log.operationTime = PQgetvalue(res, i, 7);
        logs.push_back(log);
    }

    PQclear(res);
    return logs;
}

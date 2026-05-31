#include "BackupDao.h"

#include <cstdlib>
#include <iostream>
#include <libpq-fe.h>
#include <sstream>

namespace {
BackupRecord buildBackupRecordFromRow(PGresult* res, int row) {
    BackupRecord record;
    record.backupId = std::atoll(PQgetvalue(res, row, 0));
    record.backupName = PQgetvalue(res, row, 1);
    record.filePath = PQgetvalue(res, row, 2);
    record.backupType = PQgetvalue(res, row, 3);
    record.operatorId = std::atoll(PQgetvalue(res, row, 4));
    record.operatorName = PQgetvalue(res, row, 5);
    record.backupTime = PQgetvalue(res, row, 6);
    record.remark = PQgetvalue(res, row, 7);
    return record;
}
}

BackupDao::BackupDao(DatabaseManager& database, OperationLogDao& operationLogDao)
    : db(database), logDao(operationLogDao) {
}

bool BackupDao::addBackupRecord(const std::string& backupName,
    const std::string& filePath,
    const std::string& backupType,
    long long operatorId,
    const std::string& remark) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    if (backupName.empty() || filePath.empty() || backupType.empty()) {
        std::cerr << "备份名称、文件路径、备份类型不能为空！" << std::endl;
        return false;
    }

    const std::string operatorIdStr = std::to_string(operatorId);
    const char* params[5] = {
        backupName.c_str(),
        filePath.c_str(),
        backupType.c_str(),
        operatorIdStr.c_str(),
        remark.c_str()
    };

    PGresult* res = PQexecParams(
        conn,
        "INSERT INTO backup_records "
        "(backup_name, file_path, backup_type, operator_id, remark) "
        "VALUES ($1, $2, $3::backup_type_enum, $4, $5) "
        "RETURNING backup_id",
        5,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "新增备份记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    const long long backupId = std::atoll(PQgetvalue(res, 0, 0));
    PQclear(res);

    std::ostringstream oss;
    oss << "新增备份记录：名称=" << backupName
        << "，类型=" << backupType
        << "，路径=" << filePath;
    logDao.addLog(operatorId, "新增备份记录", "backup_records", backupId, oss.str());
    return true;
}

std::vector<BackupRecord> BackupDao::getAllBackupRecords() {
    std::vector<BackupRecord> records;
    PGconn* conn = db.getConnection();
    if (!conn) {
        return records;
    }

    PGresult* res = PQexec(
        conn,
        "SELECT br.backup_id, br.backup_name, br.file_path, br.backup_type::text, "
        "br.operator_id, COALESCE(u.username, ''), COALESCE(br.backup_time::text, ''), "
        "COALESCE(br.remark, '') "
        "FROM backup_records br "
        "LEFT JOIN users u ON br.operator_id = u.user_id "
        "ORDER BY br.backup_id DESC"
    );
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "查询备份记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return records;
    }

    const int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        records.push_back(buildBackupRecordFromRow(res, i));
    }
    PQclear(res);
    return records;
}

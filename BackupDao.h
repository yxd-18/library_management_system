#ifndef BACKUPDAO_H
#define BACKUPDAO_H

#include <string>
#include <vector>
#include "BackupRecord.h"
#include "DatabaseManager.h"
#include "OperationLogDao.h"

class BackupDao {
private:
    DatabaseManager& db;
    OperationLogDao& logDao;

public:
    BackupDao(DatabaseManager& database, OperationLogDao& operationLogDao);

    bool addBackupRecord(const std::string& backupName,
        const std::string& filePath,
        const std::string& backupType,
        long long operatorId,
        const std::string& remark = "");
    std::vector<BackupRecord> getAllBackupRecords();
};

#endif

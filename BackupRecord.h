#ifndef BACKUPRECORD_H
#define BACKUPRECORD_H

#include <string>

struct BackupRecord {
    long long backupId = 0;
    std::string backupName;
    std::string filePath;
    std::string backupType;
    long long operatorId = 0;
    std::string operatorName;
    std::string backupTime;
    std::string remark;
};

#endif

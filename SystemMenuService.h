#ifndef SYSTEMMENUSERVICE_H
#define SYSTEMMENUSERVICE_H

#include <optional>
#include <string>
#include <vector>
#include "BackupDao.h"
#include "BackupRecord.h"
#include "OperationLog.h"
#include "OperationLogDao.h"
#include "ServiceResult.h"
#include "UserSettings.h"
#include "UserSettingsDao.h"

DataResult<std::vector<OperationLog>> listOperationLogs(OperationLogDao& logDao);
ActionResult createBackupLog(BackupDao& backupDao,
    const std::string& backupName,
    const std::string& filePath,
    const std::string& backupType,
    long long operatorId,
    const std::string& remark);
DataResult<std::vector<BackupRecord>> listBackupLogs(BackupDao& backupDao);
ActionResult ensureDefaultUserSettings(UserSettingsDao& settingsDao, long long userId);
DataResult<std::optional<UserSettings>> getUserSettings(UserSettingsDao& settingsDao, long long userId);
DataResult<std::vector<UserSettings>> listUserSettings(UserSettingsDao& settingsDao);
ActionResult updateUserSettingsProfile(UserSettingsDao& settingsDao, const UserSettings& settings, long long operatorId);

#endif

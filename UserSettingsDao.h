#ifndef USERSETTINGSDAO_H
#define USERSETTINGSDAO_H

#include <optional>
#include <vector>
#include "DatabaseManager.h"
#include "OperationLogDao.h"
#include "UserSettings.h"

class UserSettingsDao {
private:
    DatabaseManager& db;
    OperationLogDao& logDao;

public:
    UserSettingsDao(DatabaseManager& database, OperationLogDao& operationLogDao);

    bool ensureDefaultSettings(long long userId);
    std::optional<UserSettings> getSettingsByUserId(long long userId);
    std::vector<UserSettings> getAllSettings();
    bool updateSettings(const UserSettings& settings, long long operatorId);
};

#endif

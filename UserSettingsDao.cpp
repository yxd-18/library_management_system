#include "UserSettingsDao.h"

#include <cstdlib>
#include <iostream>
#include <libpq-fe.h>
#include <sstream>

namespace {
bool hasAffectedRows(PGresult* res) {
    const char* tuples = PQcmdTuples(res);
    return tuples != nullptr && tuples[0] != '\0' && std::atoi(tuples) > 0;
}

UserSettings buildSettingsFromRow(PGresult* res, int row) {
    UserSettings settings;
    settings.settingId = std::atoll(PQgetvalue(res, row, 0));
    settings.userId = std::atoll(PQgetvalue(res, row, 1));
    settings.username = PQgetvalue(res, row, 2);
    settings.fontSize = PQgetvalue(res, row, 3);
    settings.theme = PQgetvalue(res, row, 4);
    settings.languagePref = PQgetvalue(res, row, 5);
    settings.pageSize = std::atoi(PQgetvalue(res, row, 6));
    settings.enableNotification = std::string(PQgetvalue(res, row, 7)) == "t";
    settings.updateTime = PQgetvalue(res, row, 8);
    return settings;
}
}

UserSettingsDao::UserSettingsDao(DatabaseManager& database, OperationLogDao& operationLogDao)
    : db(database), logDao(operationLogDao) {
}

bool UserSettingsDao::ensureDefaultSettings(long long userId) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    const std::string userIdStr = std::to_string(userId);
    const char* params[1] = { userIdStr.c_str() };
    PGresult* res = PQexecParams(
        conn,
        "INSERT INTO user_settings (user_id) "
        "SELECT $1 WHERE NOT EXISTS (SELECT 1 FROM user_settings WHERE user_id = $1)",
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "初始化用户设置失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

std::optional<UserSettings> UserSettingsDao::getSettingsByUserId(long long userId) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return std::nullopt;
    }

    if (!ensureDefaultSettings(userId)) {
        return std::nullopt;
    }

    const std::string userIdStr = std::to_string(userId);
    const char* params[1] = { userIdStr.c_str() };
    PGresult* res = PQexecParams(
        conn,
        "SELECT us.setting_id, us.user_id, u.username, us.font_size, us.theme, "
        "us.language_pref, us.page_size, us.enable_notification::text, "
        "COALESCE(us.update_time::text, '') "
        "FROM user_settings us "
        "JOIN users u ON us.user_id = u.user_id "
        "WHERE us.user_id = $1",
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "查询用户设置失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return std::nullopt;
    }
    if (PQntuples(res) == 0) {
        PQclear(res);
        return std::nullopt;
    }

    UserSettings settings = buildSettingsFromRow(res, 0);
    PQclear(res);
    return settings;
}

std::vector<UserSettings> UserSettingsDao::getAllSettings() {
    std::vector<UserSettings> settingsList;
    PGconn* conn = db.getConnection();
    if (!conn) {
        return settingsList;
    }

    PGresult* res = PQexec(
        conn,
        "SELECT us.setting_id, us.user_id, u.username, us.font_size, us.theme, "
        "us.language_pref, us.page_size, us.enable_notification::text, "
        "COALESCE(us.update_time::text, '') "
        "FROM user_settings us "
        "JOIN users u ON us.user_id = u.user_id "
        "ORDER BY us.user_id"
    );
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "查询全部用户设置失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return settingsList;
    }

    const int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        settingsList.push_back(buildSettingsFromRow(res, i));
    }
    PQclear(res);
    return settingsList;
}

bool UserSettingsDao::updateSettings(const UserSettings& settings, long long operatorId) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    if (!ensureDefaultSettings(settings.userId)) {
        return false;
    }

    const std::string pageSizeStr = std::to_string(settings.pageSize);
    const std::string userIdStr = std::to_string(settings.userId);
    const char* params[6] = {
        settings.fontSize.c_str(),
        settings.theme.c_str(),
        settings.languagePref.c_str(),
        pageSizeStr.c_str(),
        settings.enableNotification ? "true" : "false",
        userIdStr.c_str()
    };
    PGresult* res = PQexecParams(
        conn,
        "UPDATE user_settings "
        "SET font_size = $1, theme = $2, language_pref = $3, "
        "page_size = $4::INT, enable_notification = $5::BOOLEAN, "
        "update_time = CURRENT_TIMESTAMP "
        "WHERE user_id = $6",
        6,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(res) != PGRES_COMMAND_OK || !hasAffectedRows(res)) {
        std::cerr << "更新用户设置失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }
    PQclear(res);

    std::ostringstream oss;
    oss << "修改用户设置：用户ID=" << settings.userId
        << "，主题=" << settings.theme
        << "，语言=" << settings.languagePref
        << "，每页条数=" << settings.pageSize;
    logDao.addLog(operatorId, "修改用户设置", "user_settings", settings.userId, oss.str());
    return true;
}

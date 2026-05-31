#include "SystemMenuService.h"

DataResult<std::vector<OperationLog>> listOperationLogs(OperationLogDao& logDao) {
    DataResult<std::vector<OperationLog>> result;
    result.success = true;
    result.data = logDao.getAllLogs();
    result.message = result.data.empty() ? "当前暂无操作日志。" : "查询操作日志成功。";
    return result;
}

ActionResult createBackupLog(BackupDao& backupDao,
    const std::string& backupName,
    const std::string& filePath,
    const std::string& backupType,
    long long operatorId,
    const std::string& remark) {
    ActionResult result;
    result.success = backupDao.addBackupRecord(backupName, filePath, backupType, operatorId, remark);
    result.message = result.success ? "新增备份记录成功！" : "新增备份记录失败！";
    return result;
}

DataResult<std::vector<BackupRecord>> listBackupLogs(BackupDao& backupDao) {
    DataResult<std::vector<BackupRecord>> result;
    result.success = true;
    result.data = backupDao.getAllBackupRecords();
    result.message = result.data.empty() ? "当前暂无备份记录。" : "查询备份记录成功。";
    return result;
}

ActionResult ensureDefaultUserSettings(UserSettingsDao& settingsDao, long long userId) {
    ActionResult result;
    result.success = settingsDao.ensureDefaultSettings(userId);
    result.message = result.success ? "默认设置初始化完成！" : "默认设置初始化失败！";
    return result;
}

DataResult<std::optional<UserSettings>> getUserSettings(UserSettingsDao& settingsDao, long long userId) {
    DataResult<std::optional<UserSettings>> result;
    result.data = settingsDao.getSettingsByUserId(userId);
    result.success = result.data.has_value();
    result.message = result.success ? "查询用户设置成功。" : "未找到当前用户设置！";
    return result;
}

DataResult<std::vector<UserSettings>> listUserSettings(UserSettingsDao& settingsDao) {
    DataResult<std::vector<UserSettings>> result;
    result.success = true;
    result.data = settingsDao.getAllSettings();
    result.message = result.data.empty() ? "当前暂无用户设置数据。" : "查询全部用户设置成功。";
    return result;
}

ActionResult updateUserSettingsProfile(UserSettingsDao& settingsDao, const UserSettings& settings, long long operatorId) {
    ActionResult result;
    result.success = settingsDao.updateSettings(settings, operatorId);
    result.message = result.success ? "修改用户设置成功！" : "修改用户设置失败！";
    return result;
}

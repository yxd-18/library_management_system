#include "SystemMenu.h"

#include <iostream>
#include "MenuCommon.h"
#include "SystemMenuService.h"

namespace {
void printSystemMenu(long long currentOperatorId) {
    std::cout << "\n===== 系统模块 =====\n";
    std::cout << "当前操作人ID: " << currentOperatorId << "\n";
    std::cout << "1. 查看操作日志\n";
    std::cout << "2. 新增备份记录\n";
    std::cout << "3. 查看全部备份记录\n";
    std::cout << "4. 初始化当前用户默认设置\n";
    std::cout << "5. 查看当前用户设置\n";
    std::cout << "6. 查看全部用户设置\n";
    std::cout << "7. 修改当前用户设置\n";
    std::cout << "0. 返回上一级\n";
    std::cout << "请选择：";
}
}

void runSystemMenu(OperationLogDao& logDao,
    BackupDao& backupDao,
    UserSettingsDao& settingsDao,
    long long currentOperatorId) {
    while (true) {
        printSystemMenu(currentOperatorId);
        const int choice = parseInt(promptLine(""), -1);

        if (choice == 0) {
            return;
        }
        if (choice == 1) {
            const auto result = listOperationLogs(logDao);
            std::cout << result.message << std::endl;
            for (const auto& log : result.data) {
                std::cout << log.logId << " | 操作人: " << log.operatorId
                    << " | 类型: " << log.operationType
                    << " | 表: " << log.targetTable
                    << " | 目标ID: " << log.targetId
                    << " | 内容: " << log.operationContent
                    << " | 时间: " << log.operationTime
                    << std::endl;
            }
        }
        else if (choice == 2) {
            const std::string backupName = promptLine("备份名称: ");
            const std::string filePath = promptLine("备份文件路径: ");
            const std::string backupType = promptLine("备份类型(手动备份/自动备份/恢复): ");
            const std::string remark = promptLine("备注(可留空): ");
            const auto result = createBackupLog(backupDao, backupName, filePath, backupType, currentOperatorId, remark);
            std::cout << result.message << std::endl;
        }
        else if (choice == 3) {
            const auto result = listBackupLogs(backupDao);
            std::cout << result.message << std::endl;
            for (const auto& record : result.data) {
                printBackupRecord(record);
            }
        }
        else if (choice == 4) {
            const auto result = ensureDefaultUserSettings(settingsDao, currentOperatorId);
            std::cout << result.message << std::endl;
        }
        else if (choice == 5) {
            const auto result = getUserSettings(settingsDao, currentOperatorId);
            std::cout << result.message << std::endl;
            if (result.success && result.data.has_value()) {
                printSettings(*result.data);
            }
        }
        else if (choice == 6) {
            const auto result = listUserSettings(settingsDao);
            std::cout << result.message << std::endl;
            for (const auto& settings : result.data) {
                printSettings(settings);
            }
        }
        else if (choice == 7) {
            UserSettings settings;
            settings.userId = currentOperatorId;
            settings.fontSize = promptLine("字号(small/medium/large): ");
            settings.theme = promptLine("主题(light/dark): ");
            settings.languagePref = promptLine("语言(如 zh-CN/en-US): ");
            settings.pageSize = promptInt("每页显示条数: ", 10);
            settings.enableNotification = promptLine("是否开启通知(true/false): ") != "false";
            const auto result = updateUserSettingsProfile(settingsDao, settings, currentOperatorId);
            std::cout << result.message << std::endl;
        }
        else {
            std::cout << "无效的菜单编号，请重新输入。" << std::endl;
        }
    }
}

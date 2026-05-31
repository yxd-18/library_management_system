#include "MenuHandlers.h"

#include <optional>
#include <vector>
#include <iostream>
#include "BookMenu.h"
#include "BookMenuService.h"
#include "BorrowMenu.h"
#include "BorrowMenuService.h"
#include "MenuCommon.h"
#include "SystemMenu.h"
#include "SystemMenuService.h"
#include "UserMenu.h"
#include "UserMenuService.h"

namespace {
bool isAdmin(const User& user) {
    return user.roleId == 1 || user.roleName == "admin";
}

void syncCurrentOperatorId(const std::optional<User>& currentUser, long long& currentOperatorId) {
    currentOperatorId = currentUser.has_value() ? currentUser->userId : 0;
}

void printGuestMenu() {
    std::cout << "\n===== 图书管理系统 =====\n";
    std::cout << "当前状态: 未登录\n";
    std::cout << "1. 用户登录\n";
    std::cout << "2. 读者注册\n";
    std::cout << "3. 查询图书目录\n";
    std::cout << "0. 退出系统\n";
    std::cout << "请选择：";
}

void printAdminMenu(const User& user) {
    std::cout << "\n===== 管理员工作台 =====\n";
    std::cout << "当前用户: " << user.realName
        << " (" << user.username << ")"
        << " | 角色: " << user.roleName
        << " | ID: " << user.userId << "\n";
    std::cout << "1. 用户管理\n";
    std::cout << "2. 图书管理\n";
    std::cout << "3. 借阅管理\n";
    std::cout << "4. 系统管理\n";
    std::cout << "5. 退出登录\n";
    std::cout << "0. 退出系统\n";
    std::cout << "请选择：";
}

void printReaderMenu(const User& user) {
    std::cout << "\n===== 读者工作台 =====\n";
    std::cout << "当前用户: " << user.realName
        << " (" << user.username << ")"
        << " | 学号: " << user.studentNo
        << " | ID: " << user.userId << "\n";
    std::cout << "1. 查询图书目录\n";
    std::cout << "2. 查看我的借阅记录\n";
    std::cout << "3. 查看我当前未归还图书\n";
    std::cout << "4. 续借我的图书\n";
    std::cout << "5. 查看我的预约\n";
    std::cout << "6. 预约图书\n";
    std::cout << "7. 取消我的预约\n";
    std::cout << "8. 挂失我的借阅图书\n";
    std::cout << "9. 我的设置\n";
    std::cout << "10. 退出登录\n";
    std::cout << "0. 退出系统\n";
    std::cout << "请选择：";
}

void runCatalogMenu(BookDao& bookDao) {
    while (true) {
        std::cout << "\n===== 图书目录 =====\n";
        std::cout << "1. 查看全部图书\n";
        std::cout << "2. 按关键词搜索图书\n";
        std::cout << "3. 按ID查看图书详情\n";
        std::cout << "0. 返回上一级\n";
        std::cout << "请选择：";

        const int choice = parseInt(promptLine(""), -1);
        if (choice == 0) {
            return;
        }
        if (choice == 1) {
            const auto result = listBooks(bookDao);
            std::cout << result.message << std::endl;
            for (const auto& book : result.data) {
                printBook(book);
            }
        }
        else if (choice == 2) {
            const std::string keyword = promptLine("请输入关键词: ");
            const auto result = searchBooksByKeyword(bookDao, keyword);
            std::cout << result.message << std::endl;
            for (const auto& book : result.data) {
                printBook(book);
            }
        }
        else if (choice == 3) {
            const long long bookId = promptLongLong("图书ID: ");
            const auto result = findBookById(bookDao, bookId);
            std::cout << result.message << std::endl;
            if (result.success && result.data.has_value()) {
                printBook(*result.data);
                std::cout << "出版社: " << result.data->publisher << std::endl;
                std::cout << "出版日期: " << result.data->publishDate << std::endl;
                std::cout << "简介: " << result.data->description << std::endl;
            }
        }
        else {
            std::cout << "无效的菜单编号，请重新输入。" << std::endl;
        }
    }
}

void printReaderBorrowRecords(const std::vector<BorrowRecord>& records, bool activeOnly) {
    bool hasData = false;
    for (const auto& record : records) {
        if (activeOnly && !record.returnTime.empty()) {
            continue;
        }
        printBorrowRecord(record);
        hasData = true;
    }

    if (!hasData) {
        std::cout << (activeOnly ? "当前没有未归还图书。" : "当前没有借阅记录。") << std::endl;
    }
}

void runReaderSettingsMenu(UserSettingsDao& settingsDao, const User& currentUser) {
    while (true) {
        std::cout << "\n===== 我的设置 =====\n";
        std::cout << "1. 查看我的设置\n";
        std::cout << "2. 修改我的设置\n";
        std::cout << "0. 返回上一级\n";
        std::cout << "请选择：";

        const int choice = parseInt(promptLine(""), -1);
        if (choice == 0) {
            return;
        }
        if (choice == 1) {
            const auto result = getUserSettings(settingsDao, currentUser.userId);
            std::cout << result.message << std::endl;
            if (result.success && result.data.has_value()) {
                printSettings(*result.data);
            }
        }
        else if (choice == 2) {
            UserSettings settings;
            settings.userId = currentUser.userId;
            settings.fontSize = promptLine("字号(small/medium/large): ");
            settings.theme = promptLine("主题(light/dark): ");
            settings.languagePref = promptLine("语言(如 zh-CN/en-US): ");
            settings.pageSize = promptInt("每页显示条数: ", 10);
            settings.enableNotification = promptLine("是否开启通知(true/false): ") != "false";

            const auto result = updateUserSettingsProfile(settingsDao, settings, currentUser.userId);
            std::cout << result.message << std::endl;
        }
        else {
            std::cout << "无效的菜单编号，请重新输入。" << std::endl;
        }
    }
}

bool runGuestWorkspace(UserDao& userDao,
    BookDao& bookDao,
    UserSettingsDao& settingsDao,
    std::optional<User>& currentUser,
    long long& currentOperatorId) {
    while (!currentUser.has_value()) {
        printGuestMenu();
        const int choice = parseInt(promptLine(""), -1);

        if (choice == 0) {
            return false;
        }
        if (choice == 1) {
            const std::string username = promptLine("用户名: ");
            const std::string password = promptLine("密码: ");

            const auto result = loginUser(userDao, username, password);
            std::cout << result.message << std::endl;
            if (result.success && result.data.has_value()) {
                currentUser = *result.data;
                syncCurrentOperatorId(currentUser, currentOperatorId);
                ensureDefaultUserSettings(settingsDao, currentOperatorId);
                std::cout << "欢迎，" << currentUser->realName
                    << "。当前角色：" << currentUser->roleName << std::endl;
                return true;
            }
        }
        else if (choice == 2) {
            User user;
            user.username = promptLine("用户名: ");
            user.passwordHash = promptLine("密码: ");
            user.realName = promptLine("姓名: ");
            user.studentNo = promptLine("学号: ");
            user.phone = promptLine("电话: ");
            user.email = promptLine("邮箱: ");
            user.gender = promptLine("性别(男/女/其他): ");
            user.remark = promptLine("备注: ");

            const auto result = registerReaderUser(userDao, user);
            std::cout << result.message << std::endl;
        }
        else if (choice == 3) {
            runCatalogMenu(bookDao);
        }
        else {
            std::cout << "无效的菜单编号，请重新输入。" << std::endl;
        }
    }

    return true;
}

bool runAdminWorkspace(UserDao& userDao,
    BookDao& bookDao,
    CategoryDao& categoryDao,
    BorrowDao& borrowDao,
    OperationLogDao& logDao,
    BackupDao& backupDao,
    UserSettingsDao& settingsDao,
    std::optional<User>& currentUser,
    long long& currentOperatorId) {
    while (currentUser.has_value()) {
        printAdminMenu(*currentUser);
        const int choice = parseInt(promptLine(""), -1);

        if (choice == 0) {
            return false;
        }
        if (choice == 1) {
            runUserMenu(userDao, currentOperatorId);
        }
        else if (choice == 2) {
            runBookMenu(bookDao, categoryDao, currentOperatorId);
        }
        else if (choice == 3) {
            runBorrowMenu(borrowDao, currentOperatorId);
        }
        else if (choice == 4) {
            runSystemMenu(logDao, backupDao, settingsDao, currentOperatorId);
        }
        else if (choice == 5) {
            currentUser.reset();
            syncCurrentOperatorId(currentUser, currentOperatorId);
            std::cout << "已退出登录。" << std::endl;
            return true;
        }
        else {
            std::cout << "无效的菜单编号，请重新输入。" << std::endl;
        }
    }

    return true;
}

bool runReaderWorkspace(BookDao& bookDao,
    BorrowDao& borrowDao,
    UserSettingsDao& settingsDao,
    std::optional<User>& currentUser,
    long long& currentOperatorId) {
    while (currentUser.has_value()) {
        printReaderMenu(*currentUser);
        const int choice = parseInt(promptLine(""), -1);

        if (choice == 0) {
            return false;
        }
        if (choice == 1) {
            runCatalogMenu(bookDao);
        }
        else if (choice == 2) {
            const auto result = listBorrowRecordsByUser(borrowDao, currentUser->userId);
            std::cout << result.message << std::endl;
            printReaderBorrowRecords(result.data, false);
        }
        else if (choice == 3) {
            const auto result = listBorrowRecordsByUser(borrowDao, currentUser->userId);
            std::cout << result.message << std::endl;
            printReaderBorrowRecords(result.data, true);
        }
        else if (choice == 4) {
            const long long recordId = promptLongLong("请输入要续借的借阅记录ID: ");
            const auto recordResult = findBorrowRecordById(borrowDao, recordId);
            if (!recordResult.success || !recordResult.data.has_value()) {
                std::cout << recordResult.message << std::endl;
                continue;
            }
            if (recordResult.data->userId != currentUser->userId) {
                std::cout << "只能续借自己的借阅记录。" << std::endl;
                continue;
            }

            const std::string remark = promptLine("备注(可留空): ");
            const auto result = renewBorrowedBook(borrowDao, recordId, currentOperatorId, remark);
            std::cout << result.message << std::endl;
        }
        else if (choice == 5) {
            const auto result = listReservationRecordsByUser(borrowDao, currentUser->userId);
            std::cout << result.message << std::endl;
            for (const auto& record : result.data) {
                printReservationRecord(record);
            }
        }
        else if (choice == 6) {
            const long long bookId = promptLongLong("请输入要预约的图书ID: ");
            const std::string remark = promptLine("备注(可留空): ");
            const auto result = createReservationRecord(borrowDao, currentUser->userId, bookId, currentOperatorId, remark);
            std::cout << result.message << std::endl;
        }
        else if (choice == 7) {
            const long long reservationId = promptLongLong("请输入要取消的预约ID: ");
            const auto reservationResult = findReservationRecordById(borrowDao, reservationId);
            if (!reservationResult.success || !reservationResult.data.has_value()) {
                std::cout << reservationResult.message << std::endl;
                continue;
            }
            if (reservationResult.data->userId != currentUser->userId) {
                std::cout << "只能取消自己的预约记录。" << std::endl;
                continue;
            }

            const std::string remark = promptLine("备注(可留空): ");
            const auto result = cancelReservationRecord(borrowDao, reservationId, currentOperatorId, remark);
            std::cout << result.message << std::endl;
        }
        else if (choice == 8) {
            const long long recordId = promptLongLong("请输入要挂失的借阅记录ID: ");
            const auto recordResult = findBorrowRecordById(borrowDao, recordId);
            if (!recordResult.success || !recordResult.data.has_value()) {
                std::cout << recordResult.message << std::endl;
                continue;
            }
            if (recordResult.data->userId != currentUser->userId) {
                std::cout << "只能挂失自己的借阅记录。" << std::endl;
                continue;
            }
            if (!recordResult.data->returnTime.empty()) {
                std::cout << "该记录已结束，不能再挂失。" << std::endl;
                continue;
            }

            const std::string remark = promptLine("备注(可留空): ");
            const auto result = reportLostBorrowedBook(borrowDao, recordId, currentOperatorId, remark);
            std::cout << result.message << std::endl;
        }
        else if (choice == 9) {
            runReaderSettingsMenu(settingsDao, *currentUser);
        }
        else if (choice == 10) {
            currentUser.reset();
            syncCurrentOperatorId(currentUser, currentOperatorId);
            std::cout << "已退出登录。" << std::endl;
            return true;
        }
        else {
            std::cout << "无效的菜单编号，请重新输入。" << std::endl;
        }
    }

    return true;
}
}

void runConsoleMenu(UserDao& userDao,
    BookDao& bookDao,
    CategoryDao& categoryDao,
    BorrowDao& borrowDao,
    OperationLogDao& logDao,
    BackupDao& backupDao,
    UserSettingsDao& settingsDao,
    long long& currentOperatorId) {
    std::optional<User> currentUser;
    syncCurrentOperatorId(currentUser, currentOperatorId);

    while (true) {
        if (!currentUser.has_value()) {
            if (!runGuestWorkspace(userDao, bookDao, settingsDao, currentUser, currentOperatorId)) {
                return;
            }
            continue;
        }

        if (isAdmin(*currentUser)) {
            if (!runAdminWorkspace(userDao, bookDao, categoryDao, borrowDao, logDao, backupDao, settingsDao, currentUser, currentOperatorId)) {
                return;
            }
        }
        else {
            if (!runReaderWorkspace(bookDao, borrowDao, settingsDao, currentUser, currentOperatorId)) {
                return;
            }
        }
    }
}

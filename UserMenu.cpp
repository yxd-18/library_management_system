#include "UserMenu.h"

#include <iostream>
#include "MenuCommon.h"
#include "UserMenuService.h"

namespace {
void printUserMenu(long long currentOperatorId) {
    std::cout << "\n===== 用户管理 =====\n";
    std::cout << "当前管理员ID: " << currentOperatorId << "\n";
    std::cout << "1. 查询所有用户\n";
    std::cout << "2. 按用户名查询用户\n";
    std::cout << "3. 新增读者账号\n";
    std::cout << "4. 修改用户信息\n";
    std::cout << "5. 删除用户\n";
    std::cout << "0. 返回上一级\n";
    std::cout << "请选择：";
}
}

void runUserMenu(UserDao& userDao, long long& currentOperatorId) {
    while (true) {
        printUserMenu(currentOperatorId);
        const int choice = parseInt(promptLine(""), -1);

        if (choice == 0) {
            return;
        }
        if (choice == 1) {
            const auto result = listUsers(userDao);
            std::cout << result.message << std::endl;
            for (const auto& user : result.data) {
                std::cout << user.userId << " | "
                    << user.username << " | "
                    << user.realName << " | "
                    << user.roleName << " | "
                    << user.status << std::endl;
            }
        }
        else if (choice == 2) {
            const std::string username = promptLine("请输入用户名: ");
            const auto result = findUserByUsername(userDao, username);
            std::cout << result.message << std::endl;
            if (result.success && result.data.has_value()) {
                std::cout << "ID: " << result.data->userId << std::endl;
                std::cout << "用户名: " << result.data->username << std::endl;
                std::cout << "姓名: " << result.data->realName << std::endl;
                std::cout << "学号: " << result.data->studentNo << std::endl;
                std::cout << "电话: " << result.data->phone << std::endl;
                std::cout << "邮箱: " << result.data->email << std::endl;
                std::cout << "角色: " << result.data->roleName << std::endl;
                std::cout << "状态: " << result.data->status << std::endl;
            }
        }
        else if (choice == 3) {
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
        else if (choice == 4) {
            User user;
            user.userId = promptLongLong("用户ID: ");
            user.realName = promptLine("姓名: ");
            user.studentNo = promptLine("学号: ");
            user.phone = promptLine("电话: ");
            user.email = promptLine("邮箱: ");
            user.gender = promptLine("性别(男/女/其他): ");
            user.status = promptLine("状态(正常/禁用): ");
            user.remark = promptLine("备注: ");

            const auto result = updateExistingUser(userDao, user);
            std::cout << result.message << std::endl;
        }
        else if (choice == 5) {
            const long long userId = promptLongLong("请输入要删除的用户ID: ");
            if (userId == currentOperatorId) {
                std::cout << "不能删除当前已登录的管理员账号。" << std::endl;
                continue;
            }
            const auto result = removeUser(userDao, userId);
            std::cout << result.message << std::endl;
        }
        else {
            std::cout << "无效的菜单编号，请重新输入。" << std::endl;
        }
    }
}

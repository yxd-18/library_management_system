#include "BorrowMenu.h"

#include <iostream>
#include "MenuCommon.h"
#include "BorrowMenuService.h"

namespace {
void printBorrowMenu() {
    std::cout << "\n===== 借阅模块 =====\n";
    std::cout << "1. 借书\n";
    std::cout << "2. 还书\n";
    std::cout << "3. 续借\n";
    std::cout << "4. 办理预约\n";
    std::cout << "5. 取消预约\n";
    std::cout << "6. 图书挂失处理\n";
    std::cout << "7. 查询全部借阅记录\n";
    std::cout << "8. 查询未归还记录\n";
    std::cout << "9. 查询逾期记录\n";
    std::cout << "10. 按用户查询借阅记录\n";
    std::cout << "11. 查询全部预约记录\n";
    std::cout << "12. 按用户查询预约记录\n";
    std::cout << "13. 手动刷新逾期状态\n";
    std::cout << "0. 返回上一级\n";
    std::cout << "请选择：";
}
}

void runBorrowMenu(BorrowDao& borrowDao, long long currentOperatorId) {
    while (true) {
        printBorrowMenu();
        const int choice = parseInt(promptLine(""), -1);

        if (choice == 0) {
            return;
        }
        if (choice == 1) {
            const long long userId = promptLongLong("借阅用户ID: ");
            const long long bookId = promptLongLong("图书ID: ");
            const std::string remark = promptLine("备注(可留空): ");
            const auto result = createBorrowRecord(borrowDao, userId, bookId, currentOperatorId, remark);
            std::cout << result.message << std::endl;
        }
        else if (choice == 2) {
            const long long recordId = promptLongLong("借阅记录ID: ");
            const std::string remark = promptLine("备注(可留空): ");
            const auto result = returnBorrowedBook(borrowDao, recordId, currentOperatorId, remark);
            std::cout << result.message << std::endl;
        }
        else if (choice == 3) {
            const long long recordId = promptLongLong("借阅记录ID: ");
            const std::string remark = promptLine("备注(可留空): ");
            const auto result = renewBorrowedBook(borrowDao, recordId, currentOperatorId, remark);
            std::cout << result.message << std::endl;
        }
        else if (choice == 4) {
            const long long userId = promptLongLong("预约用户ID: ");
            const long long bookId = promptLongLong("图书ID: ");
            const std::string remark = promptLine("备注(可留空): ");
            const auto result = createReservationRecord(borrowDao, userId, bookId, currentOperatorId, remark);
            std::cout << result.message << std::endl;
        }
        else if (choice == 5) {
            const long long reservationId = promptLongLong("预约记录ID: ");
            const std::string remark = promptLine("备注(可留空): ");
            const auto result = cancelReservationRecord(borrowDao, reservationId, currentOperatorId, remark);
            std::cout << result.message << std::endl;
        }
        else if (choice == 6) {
            const long long recordId = promptLongLong("借阅记录ID: ");
            const std::string remark = promptLine("备注(可留空): ");
            const auto result = reportLostBorrowedBook(borrowDao, recordId, currentOperatorId, remark);
            std::cout << result.message << std::endl;
        }
        else if (choice == 7) {
            const auto result = listBorrowRecords(borrowDao);
            std::cout << result.message << std::endl;
            for (const auto& record : result.data) {
                printBorrowRecord(record);
            }
        }
        else if (choice == 8) {
            const auto result = listActiveBorrowRecords(borrowDao);
            std::cout << result.message << std::endl;
            for (const auto& record : result.data) {
                printBorrowRecord(record);
            }
        }
        else if (choice == 9) {
            const auto result = listOverdueBorrowRecords(borrowDao);
            std::cout << result.message << std::endl;
            for (const auto& record : result.data) {
                printBorrowRecord(record);
            }
        }
        else if (choice == 10) {
            const long long userId = promptLongLong("用户ID: ");
            const auto result = listBorrowRecordsByUser(borrowDao, userId);
            std::cout << result.message << std::endl;
            for (const auto& record : result.data) {
                printBorrowRecord(record);
            }
        }
        else if (choice == 11) {
            const auto result = listReservationRecords(borrowDao);
            std::cout << result.message << std::endl;
            for (const auto& record : result.data) {
                printReservationRecord(record);
            }
        }
        else if (choice == 12) {
            const long long userId = promptLongLong("用户ID: ");
            const auto result = listReservationRecordsByUser(borrowDao, userId);
            std::cout << result.message << std::endl;
            for (const auto& record : result.data) {
                printReservationRecord(record);
            }
        }
        else if (choice == 13) {
            const auto result = refreshBorrowOverdueStatus(borrowDao);
            std::cout << result.message << std::endl;
        }
        else {
            std::cout << "无效的菜单编号，请重新输入。" << std::endl;
        }
    }
}

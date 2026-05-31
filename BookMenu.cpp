#include "BookMenu.h"

#include <iostream>
#include "MenuCommon.h"
#include "BookMenuService.h"

namespace {
void printBookMenu() {
    std::cout << "\n===== 图书模块 =====\n";
    std::cout << "1. 新增图书\n";
    std::cout << "2. 查询所有图书\n";
    std::cout << "3. 按ID查询图书\n";
    std::cout << "4. 关键词搜索图书\n";
    std::cout << "5. 修改图书\n";
    std::cout << "6. 删除图书\n";
    std::cout << "7. 新增分类\n";
    std::cout << "8. 查询所有分类\n";
    std::cout << "9. 按ID查询分类\n";
    std::cout << "10. 修改分类\n";
    std::cout << "11. 删除分类\n";
    std::cout << "0. 返回上一级\n";
    std::cout << "请选择：";
}
}

void runBookMenu(BookDao& bookDao, CategoryDao& categoryDao, long long currentOperatorId) {
    while (true) {
        printBookMenu();
        const int choice = parseInt(promptLine(""), -1);

        if (choice == 0) {
            return;
        }
        if (choice == 1) {
            Book book = inputBook(false);
            const auto result = createBook(bookDao, book, currentOperatorId);
            std::cout << result.message << std::endl;
        }
        else if (choice == 2) {
            const auto result = listBooks(bookDao);
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
        else if (choice == 4) {
            const std::string keyword = promptLine("请输入关键词: ");
            const auto result = searchBooksByKeyword(bookDao, keyword);
            std::cout << result.message << std::endl;
            for (const auto& book : result.data) {
                printBook(book);
            }
        }
        else if (choice == 5) {
            Book book = inputBook(true);
            const auto result = updateExistingBook(bookDao, book, currentOperatorId);
            std::cout << result.message << std::endl;
        }
        else if (choice == 6) {
            const long long bookId = promptLongLong("请输入要删除的图书ID: ");
            const auto result = removeBook(bookDao, bookId, currentOperatorId);
            std::cout << result.message << std::endl;
        }
        else if (choice == 7) {
            BookCategory category = inputCategory(false);
            const auto result = createCategory(categoryDao, category, currentOperatorId);
            std::cout << result.message << std::endl;
        }
        else if (choice == 8) {
            const auto result = listCategories(categoryDao);
            std::cout << result.message << std::endl;
            for (const auto& category : result.data) {
                printCategory(category);
            }
        }
        else if (choice == 9) {
            const int categoryId = promptInt("分类ID: ");
            const auto result = findCategoryById(categoryDao, categoryId);
            std::cout << result.message << std::endl;
            if (result.success && result.data.has_value()) {
                printCategory(*result.data);
            }
        }
        else if (choice == 10) {
            BookCategory category = inputCategory(true);
            const auto result = updateExistingCategory(categoryDao, category, currentOperatorId);
            std::cout << result.message << std::endl;
        }
        else if (choice == 11) {
            const int categoryId = promptInt("请输入要删除的分类ID: ");
            const auto result = removeCategory(categoryDao, categoryId, currentOperatorId);
            std::cout << result.message << std::endl;
        }
        else {
            std::cout << "无效的菜单编号，请重新输入。" << std::endl;
        }
    }
}

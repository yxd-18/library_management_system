#pragma once
#include <optional>
#include <vector>
#include "DatabaseManager.h"
#include "BookCategory.h"
#include "OperationLogDao.h"

class CategoryDao {
private:
    DatabaseManager& db;
    OperationLogDao& logDao;

public:
    CategoryDao(DatabaseManager& database, OperationLogDao& operationLogDao);

    bool addCategory(const BookCategory& category, long long operatorId);
    std::vector<BookCategory> getAllCategories();
    std::optional<BookCategory> getCategoryById(int categoryId);
    bool updateCategory(const BookCategory& category, long long operatorId);
    bool deleteCategory(int categoryId, long long operatorId);
};

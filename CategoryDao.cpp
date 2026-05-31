#include "CategoryDao.h"

#include <cstdlib>
#include <iostream>
#include <libpq-fe.h>
#include <sstream>

namespace {
BookCategory buildCategoryFromRow(PGresult* res, int row) {
    BookCategory category;
    category.categoryId = std::stoi(PQgetvalue(res, row, 0));
    category.categoryName = PQgetvalue(res, row, 1);
    category.parentId = std::stoi(PQgetvalue(res, row, 2));
    category.parentName = PQgetvalue(res, row, 3);
    category.categoryDesc = PQgetvalue(res, row, 4);
    category.createTime = PQgetvalue(res, row, 5);
    return category;
}

bool hasAffectedRows(PGresult* res) {
    const char* tuples = PQcmdTuples(res);
    return tuples != nullptr && tuples[0] != '\0' && std::atoi(tuples) > 0;
}
}

CategoryDao::CategoryDao(DatabaseManager& database, OperationLogDao& operationLogDao)
    : db(database), logDao(operationLogDao) {
}

bool CategoryDao::addCategory(const BookCategory& category, long long operatorId) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    if (category.categoryName.empty()) {
        std::cerr << "分类名称不能为空！" << std::endl;
        return false;
    }

    const std::string parentIdStr = category.parentId > 0 ? std::to_string(category.parentId) : "";
    const char* params[3] = {
        category.categoryName.c_str(),
        parentIdStr.c_str(),
        category.categoryDesc.c_str()
    };

    PGresult* res = PQexecParams(
        conn,
        "INSERT INTO book_categories (category_name, parent_id, category_desc) VALUES ($1, NULLIF($2, '')::INT, $3) RETURNING category_id",
        3,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "新增分类失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    const int newCategoryId = std::atoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    std::ostringstream oss;
    oss << "新增分类：名称=" << category.categoryName;
    logDao.addLog(operatorId, "新增分类", "book_categories", newCategoryId, oss.str());
    return true;
}

std::vector<BookCategory> CategoryDao::getAllCategories() {
    std::vector<BookCategory> categories;
    PGconn* conn = db.getConnection();
    if (!conn) {
        return categories;
    }

    const char* sql =
        "SELECT c.category_id, c.category_name, COALESCE(c.parent_id, 0), "
        "COALESCE(p.category_name, ''), COALESCE(c.category_desc, ''), "
        "COALESCE(c.create_time::text, '') "
        "FROM book_categories c "
        "LEFT JOIN book_categories p ON c.parent_id = p.category_id "
        "ORDER BY c.category_id";

    PGresult* res = PQexec(conn, sql);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "查询分类列表失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return categories;
    }

    const int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        categories.push_back(buildCategoryFromRow(res, i));
    }

    PQclear(res);
    return categories;
}

std::optional<BookCategory> CategoryDao::getCategoryById(int categoryId) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return std::nullopt;
    }

    const std::string categoryIdStr = std::to_string(categoryId);
    const char* params[1] = { categoryIdStr.c_str() };
    const char* sql =
        "SELECT c.category_id, c.category_name, COALESCE(c.parent_id, 0), "
        "COALESCE(p.category_name, ''), COALESCE(c.category_desc, ''), "
        "COALESCE(c.create_time::text, '') "
        "FROM book_categories c "
        "LEFT JOIN book_categories p ON c.parent_id = p.category_id "
        "WHERE c.category_id = $1";

    PGresult* res = PQexecParams(conn, sql, 1, nullptr, params, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "按 ID 查询分类失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return std::nullopt;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        return std::nullopt;
    }

    BookCategory category = buildCategoryFromRow(res, 0);
    PQclear(res);
    return category;
}

bool CategoryDao::updateCategory(const BookCategory& category, long long operatorId) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    if (category.categoryId <= 0) {
        std::cerr << "分类 ID 不合法！" << std::endl;
        return false;
    }
    if (category.categoryName.empty()) {
        std::cerr << "分类名称不能为空！" << std::endl;
        return false;
    }
    if (category.parentId == category.categoryId && category.parentId != 0) {
        std::cerr << "父分类不能是自己！" << std::endl;
        return false;
    }

    const std::string categoryIdStr = std::to_string(category.categoryId);
    const std::string parentIdStr = category.parentId > 0 ? std::to_string(category.parentId) : "";
    const char* params[4] = {
        category.categoryName.c_str(),
        parentIdStr.c_str(),
        category.categoryDesc.c_str(),
        categoryIdStr.c_str()
    };

    PGresult* res = PQexecParams(
        conn,
        "UPDATE book_categories SET category_name = $1, parent_id = NULLIF($2, '')::INT, category_desc = $3 WHERE category_id = $4",
        4,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "修改分类失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    if (!hasAffectedRows(res)) {
        std::cerr << "未找到要修改的分类！" << std::endl;
        PQclear(res);
        return false;
    }
    PQclear(res);

    std::ostringstream oss;
    oss << "修改分类：ID=" << category.categoryId << "，名称=" << category.categoryName;
    logDao.addLog(operatorId, "修改分类", "book_categories", category.categoryId, oss.str());
    return true;
}

bool CategoryDao::deleteCategory(int categoryId, long long operatorId) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    const std::string categoryIdStr = std::to_string(categoryId);
    const char* params[1] = { categoryIdStr.c_str() };

    PGresult* childRes = PQexecParams(
        conn,
        "SELECT COUNT(*) FROM book_categories WHERE parent_id = $1",
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(childRes) != PGRES_TUPLES_OK) {
        std::cerr << "检查子分类失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(childRes);
        return false;
    }

    const int childCount = std::atoi(PQgetvalue(childRes, 0, 0));
    PQclear(childRes);
    if (childCount > 0) {
        std::cerr << "该分类下还有子分类，不能删除！" << std::endl;
        return false;
    }

    PGresult* bookRes = PQexecParams(
        conn,
        "SELECT COUNT(*) FROM books WHERE category_id = $1",
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(bookRes) != PGRES_TUPLES_OK) {
        std::cerr << "检查分类下图书失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(bookRes);
        return false;
    }

    const int bookCount = std::atoi(PQgetvalue(bookRes, 0, 0));
    PQclear(bookRes);
    if (bookCount > 0) {
        std::cerr << "该分类下仍有图书，不能删除！" << std::endl;
        return false;
    }

    std::string categoryName;
    PGresult* nameRes = PQexecParams(
        conn,
        "SELECT category_name FROM book_categories WHERE category_id = $1",
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(nameRes) == PGRES_TUPLES_OK && PQntuples(nameRes) > 0) {
        categoryName = PQgetvalue(nameRes, 0, 0);
    }
    PQclear(nameRes);

    PGresult* res = PQexecParams(
        conn,
        "DELETE FROM book_categories WHERE category_id = $1",
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "删除分类失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    if (!hasAffectedRows(res)) {
        std::cerr << "未找到要删除的分类！" << std::endl;
        PQclear(res);
        return false;
    }
    PQclear(res);

    std::ostringstream oss;
    oss << "删除分类：ID=" << categoryId << "，名称=" << categoryName;
    logDao.addLog(operatorId, "删除分类", "book_categories", categoryId, oss.str());
    return true;
}

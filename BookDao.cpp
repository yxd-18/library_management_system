#include "BookDao.h"

#include <cstdlib>
#include <iostream>
#include <libpq-fe.h>
#include <sstream>

namespace {
const char* kBookSelectClause =
    "SELECT b.book_id, b.isbn, b.book_title, b.author, COALESCE(b.publisher, ''), "
    "COALESCE(b.publish_date::text, ''), COALESCE(b.category_id, 0), "
    "COALESCE(c.category_name, ''), COALESCE(b.price, 0)::text, "
    "COALESCE(b.total_stock, 0), COALESCE(b.available_stock, 0), COALESCE(b.lost_count, 0), "
    "COALESCE((SELECT COUNT(*) FROM reservation_records rr "
    "          WHERE rr.book_id = b.book_id AND rr.reservation_status = '排队中'), 0), "
    "COALESCE((SELECT COUNT(*) FROM reservation_records rr "
    "          WHERE rr.book_id = b.book_id AND rr.reservation_status = '可取书'), 0), "
    "COALESCE(b.location, ''), b.book_status::text, COALESCE(b.description, ''), "
    "COALESCE(b.create_time::text, ''), COALESCE(b.update_time::text, '') "
    "FROM books b "
    "LEFT JOIN book_categories c ON b.category_id = c.category_id ";

Book buildBookFromRow(PGresult* res, int row) {
    Book book;
    book.bookId = std::stoll(PQgetvalue(res, row, 0));
    book.isbn = PQgetvalue(res, row, 1);
    book.bookTitle = PQgetvalue(res, row, 2);
    book.author = PQgetvalue(res, row, 3);
    book.publisher = PQgetvalue(res, row, 4);
    book.publishDate = PQgetvalue(res, row, 5);
    book.categoryId = std::stoi(PQgetvalue(res, row, 6));
    book.categoryName = PQgetvalue(res, row, 7);
    book.price = std::stod(PQgetvalue(res, row, 8));
    book.totalStock = std::stoi(PQgetvalue(res, row, 9));
    book.availableStock = std::stoi(PQgetvalue(res, row, 10));
    book.lostCount = std::stoi(PQgetvalue(res, row, 11));
    book.pendingReservationCount = std::stoi(PQgetvalue(res, row, 12));
    book.readyReservationCount = std::stoi(PQgetvalue(res, row, 13));
    book.location = PQgetvalue(res, row, 14);
    book.bookStatus = PQgetvalue(res, row, 15);
    book.description = PQgetvalue(res, row, 16);
    book.createTime = PQgetvalue(res, row, 17);
    book.updateTime = PQgetvalue(res, row, 18);
    return book;
}

bool hasAffectedRows(PGresult* res) {
    const char* tuples = PQcmdTuples(res);
    return tuples != nullptr && tuples[0] != '\0' && std::atoi(tuples) > 0;
}

bool isDuplicateIsbn(PGconn* conn, const std::string& isbn, long long excludeBookId = 0) {
    const std::string excludeIdStr = std::to_string(excludeBookId);
    const char* sql =
        "SELECT COUNT(*) FROM books WHERE isbn = $1 AND ($2::BIGINT = 0 OR book_id <> $2::BIGINT)";
    const char* params[2] = {
        isbn.c_str(),
        excludeIdStr.c_str()
    };

    PGresult* res = PQexecParams(conn, sql, 2, nullptr, params, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "校验 ISBN 失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return true;
    }

    const int count = std::atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return count > 0;
}
}

BookDao::BookDao(DatabaseManager& database, OperationLogDao& operationLogDao)
    : db(database), logDao(operationLogDao) {
}

std::string BookDao::calculateBookStatus(int totalStock, int availableStock) {
    if (totalStock <= 0 || availableStock <= 0) {
        return "无库存";
    }
    if (availableStock < totalStock) {
        return "部分借出";
    }
    return "可借";
}

bool BookDao::addBook(const Book& book, long long operatorId) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    if (book.isbn.empty() || book.bookTitle.empty() || book.author.empty()) {
        std::cerr << "ISBN、书名、作者不能为空！" << std::endl;
        return false;
    }
    if (book.availableStock > book.totalStock || book.totalStock < 0 || book.availableStock < 0) {
        std::cerr << "库存数据不合法！" << std::endl;
        return false;
    }
    if (isDuplicateIsbn(conn, book.isbn)) {
        std::cerr << "ISBN 已存在，不能重复添加！" << std::endl;
        return false;
    }

    const std::string status = calculateBookStatus(book.totalStock, book.availableStock);
    const std::string categoryIdStr = book.categoryId > 0 ? std::to_string(book.categoryId) : "";
    const std::string priceStr = std::to_string(book.price);
    const std::string totalStockStr = std::to_string(book.totalStock);
    const std::string availableStockStr = std::to_string(book.availableStock);

    const char* params[12] = {
        book.isbn.c_str(),
        book.bookTitle.c_str(),
        book.author.c_str(),
        book.publisher.c_str(),
        book.publishDate.c_str(),
        categoryIdStr.c_str(),
        priceStr.c_str(),
        totalStockStr.c_str(),
        availableStockStr.c_str(),
        book.location.c_str(),
        status.c_str(),
        book.description.c_str()
    };

    const char* sql =
        "INSERT INTO books "
        "(isbn, book_title, author, publisher, publish_date, category_id, price, total_stock, available_stock, location, book_status, description) "
        "VALUES ($1, $2, $3, $4, NULLIF($5, '')::DATE, NULLIF($6, '')::INT, $7::NUMERIC, $8::INT, $9::INT, $10, $11::book_status_enum, $12) "
        "RETURNING book_id";

    PGresult* res = PQexecParams(conn, sql, 12, nullptr, params, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "新增图书失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    const long long newBookId = std::atoll(PQgetvalue(res, 0, 0));
    PQclear(res);

    std::ostringstream oss;
    oss << "新增图书：书名=" << book.bookTitle
        << "，ISBN=" << book.isbn
        << "，作者=" << book.author;
    logDao.addLog(operatorId, "新增图书", "books", newBookId, oss.str());

    return true;
}

std::vector<Book> BookDao::getAllBooks() {
    std::vector<Book> books;
    PGconn* conn = db.getConnection();
    if (!conn) {
        return books;
    }

    std::string sql = std::string(kBookSelectClause) + "ORDER BY b.book_id";

    PGresult* res = PQexec(conn, sql.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "查询图书列表失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return books;
    }

    const int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        books.push_back(buildBookFromRow(res, i));
    }

    PQclear(res);
    return books;
}

std::optional<Book> BookDao::getBookById(long long bookId) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return std::nullopt;
    }

    const std::string bookIdStr = std::to_string(bookId);
    const char* params[1] = { bookIdStr.c_str() };
    std::string sql = std::string(kBookSelectClause) + "WHERE b.book_id = $1";

    PGresult* res = PQexecParams(conn, sql.c_str(), 1, nullptr, params, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "按 ID 查询图书失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return std::nullopt;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        return std::nullopt;
    }

    Book book = buildBookFromRow(res, 0);
    PQclear(res);
    return book;
}

std::vector<Book> BookDao::searchBooks(const std::string& keyword) {
    std::vector<Book> books;
    PGconn* conn = db.getConnection();
    if (!conn) {
        return books;
    }

    const std::string likeKeyword = "%" + keyword + "%";
    const char* params[1] = { likeKeyword.c_str() };
    std::string sql = std::string(kBookSelectClause) +
        "WHERE b.book_title ILIKE $1 "
        "   OR b.author ILIKE $1 "
        "   OR b.isbn ILIKE $1 "
        "   OR COALESCE(c.category_name, '') ILIKE $1 "
        "ORDER BY b.book_id";

    PGresult* res = PQexecParams(conn, sql.c_str(), 1, nullptr, params, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "搜索图书失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return books;
    }

    const int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        books.push_back(buildBookFromRow(res, i));
    }

    PQclear(res);
    return books;
}

bool BookDao::updateBook(const Book& book, long long operatorId) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    if (book.bookId <= 0) {
        std::cerr << "图书 ID 不合法！" << std::endl;
        return false;
    }
    if (book.isbn.empty() || book.bookTitle.empty() || book.author.empty()) {
        std::cerr << "ISBN、书名、作者不能为空！" << std::endl;
        return false;
    }
    if (book.availableStock > book.totalStock || book.totalStock < 0 || book.availableStock < 0) {
        std::cerr << "库存数据不合法！" << std::endl;
        return false;
    }
    if (isDuplicateIsbn(conn, book.isbn, book.bookId)) {
        std::cerr << "ISBN 已被其他图书占用！" << std::endl;
        return false;
    }

    const std::string status = calculateBookStatus(book.totalStock, book.availableStock);
    const std::string bookIdStr = std::to_string(book.bookId);
    const std::string categoryIdStr = book.categoryId > 0 ? std::to_string(book.categoryId) : "";
    const std::string priceStr = std::to_string(book.price);
    const std::string totalStockStr = std::to_string(book.totalStock);
    const std::string availableStockStr = std::to_string(book.availableStock);

    const char* params[13] = {
        book.isbn.c_str(),
        book.bookTitle.c_str(),
        book.author.c_str(),
        book.publisher.c_str(),
        book.publishDate.c_str(),
        categoryIdStr.c_str(),
        priceStr.c_str(),
        totalStockStr.c_str(),
        availableStockStr.c_str(),
        book.location.c_str(),
        status.c_str(),
        book.description.c_str(),
        bookIdStr.c_str()
    };

    const char* sql =
        "UPDATE books SET "
        "isbn = $1, book_title = $2, author = $3, publisher = $4, "
        "publish_date = NULLIF($5, '')::DATE, category_id = NULLIF($6, '')::INT, "
        "price = $7::NUMERIC, total_stock = $8::INT, available_stock = $9::INT, "
        "location = $10, book_status = $11::book_status_enum, description = $12, "
        "update_time = CURRENT_TIMESTAMP "
        "WHERE book_id = $13";

    PGresult* res = PQexecParams(conn, sql, 13, nullptr, params, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "修改图书失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    if (!hasAffectedRows(res)) {
        std::cerr << "未找到要修改的图书！" << std::endl;
        PQclear(res);
        return false;
    }
    PQclear(res);

    std::ostringstream oss;
    oss << "修改图书：ID=" << book.bookId
        << "，书名=" << book.bookTitle
        << "，ISBN=" << book.isbn;
    logDao.addLog(operatorId, "修改图书", "books", book.bookId, oss.str());

    return true;
}

bool BookDao::deleteBook(long long bookId, long long operatorId) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    const std::string bookIdStr = std::to_string(bookId);
    const char* params[1] = { bookIdStr.c_str() };

    PGresult* checkRes = PQexecParams(
        conn,
        "SELECT COUNT(*) FROM borrow_records WHERE book_id = $1 AND return_time IS NULL",
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(checkRes) != PGRES_TUPLES_OK) {
        std::cerr << "检查借阅记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(checkRes);
        return false;
    }

    const int borrowCount = std::atoi(PQgetvalue(checkRes, 0, 0));
    PQclear(checkRes);
    if (borrowCount > 0) {
        std::cerr << "该图书仍有未归还记录，不能删除！" << std::endl;
        return false;
    }

    std::string bookTitle;
    PGresult* titleRes = PQexecParams(
        conn,
        "SELECT book_title FROM books WHERE book_id = $1",
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(titleRes) == PGRES_TUPLES_OK && PQntuples(titleRes) > 0) {
        bookTitle = PQgetvalue(titleRes, 0, 0);
    }
    PQclear(titleRes);

    PGresult* res = PQexecParams(
        conn,
        "DELETE FROM books WHERE book_id = $1",
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "删除图书失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    if (!hasAffectedRows(res)) {
        std::cerr << "未找到要删除的图书！" << std::endl;
        PQclear(res);
        return false;
    }
    PQclear(res);

    std::ostringstream oss;
    oss << "删除图书：ID=" << bookId << "，书名=" << bookTitle;
    logDao.addLog(operatorId, "删除图书", "books", bookId, oss.str());

    return true;
}

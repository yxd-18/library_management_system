#pragma once
#include <optional>
#include <string>
#include <vector>
#include "DatabaseManager.h"
#include "Book.h"
#include "OperationLogDao.h"

class BookDao {
private:
    DatabaseManager& db;
    OperationLogDao& logDao;

    std::string calculateBookStatus(int totalStock, int availableStock);

public:
    BookDao(DatabaseManager& database, OperationLogDao& operationLogDao);

    bool addBook(const Book& book, long long operatorId);
    std::vector<Book> getAllBooks();
    std::optional<Book> getBookById(long long bookId);
    std::vector<Book> searchBooks(const std::string& keyword);
    bool updateBook(const Book& book, long long operatorId);
    bool deleteBook(long long bookId, long long operatorId);
};

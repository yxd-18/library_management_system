#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Book.h"
#include "BookDao.h"
#include "BorrowDao.h"
#include "BorrowRecord.h"
#include "DBUtil.h"
#include "OperationLogDao.h"
#include "ReservationRecord.h"
#include "User.h"
#include "UserDao.h"

class LibraryRepository {
public:
    LibraryRepository();
    ~LibraryRepository();

    bool initialize(std::string& errorMessage);
    std::optional<User> login(const std::string& username, const std::string& password, std::string& errorMessage);

    std::vector<User> getAllUsers() const;
    std::vector<Book> getAllBooks() const;
    std::vector<Book> searchBooks(const std::string& keyword) const;
    std::optional<Book> getBookById(long long bookId) const;
    bool addBook(const Book& book, long long operatorId, std::string& errorMessage) const;
    bool updateBook(const Book& book, long long operatorId, std::string& errorMessage) const;
    bool deleteBook(long long bookId, long long operatorId, std::string& errorMessage) const;
    std::vector<BorrowRecord> getActiveBorrowRecords() const;
    std::vector<BorrowRecord> getOverdueBorrowRecords() const;
    std::vector<BorrowRecord> getBorrowRecordsByUser(long long userId) const;
    bool borrowBook(long long userId, long long bookId, long long operatorId, std::string& errorMessage) const;
    bool returnBook(long long recordId, long long operatorId, std::string& errorMessage) const;
    bool renewBook(long long recordId, long long operatorId, std::string& errorMessage) const;
    bool reserveBook(long long userId, long long bookId, long long operatorId, const std::string& remark, std::string& errorMessage) const;
    bool cancelReservation(long long reservationId, long long operatorId, std::string& errorMessage) const;
    bool reportLost(long long recordId, long long operatorId, std::string& errorMessage) const;
    std::vector<ReservationRecord> getAllReservations() const;
    std::vector<ReservationRecord> getReservationsByUser(long long userId) const;

private:
    DBUtil db_;
    std::unique_ptr<OperationLogDao> logDao_;
    std::unique_ptr<UserDao> userDao_;
    std::unique_ptr<BookDao> bookDao_;
    std::unique_ptr<BorrowDao> borrowDao_;
};

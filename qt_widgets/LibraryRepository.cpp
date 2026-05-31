#include "LibraryRepository.h"

#include <cstdlib>
#include <filesystem>

#include <QCoreApplication>
#include <QDir>

#include "QtDbSupport.h"

LibraryRepository::LibraryRepository() = default;

LibraryRepository::~LibraryRepository() = default;

bool LibraryRepository::initialize(std::string& errorMessage) {
    if (db_.isConnected()) {
        return true;
    }

    QtDBConfig config;
    std::vector<std::string> candidatePaths;

    const QString appDir = QCoreApplication::applicationDirPath();
    QDir probeDir(appDir);
    for (int i = 0; i < 6; ++i) {
        candidatePaths.push_back(probeDir.filePath("db_config.txt").toStdString());
        if (!probeDir.cdUp()) {
            break;
        }
    }
    candidatePaths.push_back(QDir::current().filePath("db_config.txt").toStdString());

    loadQtDbConfig(candidatePaths, config);
    // config.password = "zyp200551";

    if (config.password.empty()) {
        if (const char* envPassword = std::getenv("PGPASSWORD"); envPassword != nullptr) {
            config.password = envPassword;
        }
    }

    if (config.password.empty()) {
        errorMessage = "未找到数据库密码。请在 db_config.txt 或环境变量 PGPASSWORD 中配置密码。";
        return false;
    }

    if (!db_.connect(config.host, config.port, config.dbname, config.user, config.password)) {
        errorMessage = db_.getLastError();
        return false;
    }

    if (!applyQtLibraryEnhancements(db_.getConnection(), errorMessage)) {
        db_.disconnect();
        return false;
    }

    logDao_ = std::make_unique<OperationLogDao>(db_);
    userDao_ = std::make_unique<UserDao>(db_.getConnection());
    bookDao_ = std::make_unique<BookDao>(db_, *logDao_);
    borrowDao_ = std::make_unique<BorrowDao>(db_, *logDao_);

    return true;
}

std::optional<User> LibraryRepository::login(const std::string& username, const std::string& password, std::string& errorMessage) {
    errorMessage.clear();
    if (!userDao_) {
        errorMessage = "数据库尚未初始化。";
        return std::nullopt;
    }

    auto user = userDao_->login(username, password);
    if (!user.has_value()) {
        errorMessage = "用户名或密码错误，或账号已被禁用。";
    }
    return user;
}

std::vector<User> LibraryRepository::getAllUsers() const {
    return userDao_ ? userDao_->getAllUsers() : std::vector<User>{};
}

std::vector<Book> LibraryRepository::getAllBooks() const {
    return bookDao_ ? bookDao_->getAllBooks() : std::vector<Book>{};
}

std::vector<Book> LibraryRepository::searchBooks(const std::string& keyword) const {
    return bookDao_ ? bookDao_->searchBooks(keyword) : std::vector<Book>{};
}

std::optional<Book> LibraryRepository::getBookById(long long bookId) const {
    return bookDao_ ? bookDao_->getBookById(bookId) : std::nullopt;
}

bool LibraryRepository::addBook(const Book& book, long long operatorId, std::string& errorMessage) const {
    errorMessage.clear();
    if (!bookDao_) {
        errorMessage = "数据库尚未初始化。";
        return false;
    }
    if (!bookDao_->addBook(book, operatorId)) {
        errorMessage = "新增图书失败，请检查输入和数据库状态。";
        return false;
    }
    return true;
}

bool LibraryRepository::updateBook(const Book& book, long long operatorId, std::string& errorMessage) const {
    errorMessage.clear();
    if (!bookDao_) {
        errorMessage = "数据库尚未初始化。";
        return false;
    }
    if (!bookDao_->updateBook(book, operatorId)) {
        errorMessage = "更新图书失败，请检查输入和数据库状态。";
        return false;
    }
    return true;
}

bool LibraryRepository::deleteBook(long long bookId, long long operatorId, std::string& errorMessage) const {
    errorMessage.clear();
    if (!bookDao_) {
        errorMessage = "数据库尚未初始化。";
        return false;
    }
    if (!bookDao_->deleteBook(bookId, operatorId)) {
        errorMessage = "删除图书失败，请检查是否存在未归还借阅。";
        return false;
    }
    return true;
}

std::vector<BorrowRecord> LibraryRepository::getActiveBorrowRecords() const {
    return borrowDao_ ? borrowDao_->getActiveRecords() : std::vector<BorrowRecord>{};
}

std::vector<BorrowRecord> LibraryRepository::getOverdueBorrowRecords() const {
    return borrowDao_ ? borrowDao_->getOverdueRecords() : std::vector<BorrowRecord>{};
}

std::vector<BorrowRecord> LibraryRepository::getBorrowRecordsByUser(long long userId) const {
    return borrowDao_ ? borrowDao_->getRecordsByUser(userId) : std::vector<BorrowRecord>{};
}

bool LibraryRepository::borrowBook(long long userId, long long bookId, long long operatorId, std::string& errorMessage) const {
    errorMessage.clear();
    if (!borrowDao_) {
        errorMessage = "数据库尚未初始化。";
        return false;
    }
    if (!borrowDao_->borrowBook(userId, bookId, operatorId, "前端借书")) {
        errorMessage = "借书失败，请检查图书库存和用户状态。";
        return false;
    }
    return true;
}

bool LibraryRepository::returnBook(long long recordId, long long operatorId, std::string& errorMessage) const {
    errorMessage.clear();
    if (!borrowDao_) {
        errorMessage = "数据库尚未初始化。";
        return false;
    }
    if (!borrowDao_->returnBook(recordId, operatorId, "前端还书")) {
        errorMessage = "还书失败，请检查借阅记录。";
        return false;
    }
    return true;
}

bool LibraryRepository::renewBook(long long recordId, long long operatorId, std::string& errorMessage) const {
    errorMessage.clear();
    if (!borrowDao_) {
        errorMessage = "数据库尚未初始化。";
        return false;
    }
    if (!borrowDao_->renewBook(recordId, operatorId, "前端续借")) {
        errorMessage = "续借失败，可能已达到最大续借次数。";
        return false;
    }
    return true;
}

bool LibraryRepository::reserveBook(long long userId, long long bookId, long long operatorId, const std::string& remark, std::string& errorMessage) const {
    errorMessage.clear();
    if (!borrowDao_) {
        errorMessage = "数据库尚未初始化。";
        return false;
    }
    if (!borrowDao_->reserveBook(userId, bookId, operatorId, remark)) {
        errorMessage = "预约失败，请检查图书状态或用户资格。";
        return false;
    }
    return true;
}

bool LibraryRepository::cancelReservation(long long reservationId, long long operatorId, std::string& errorMessage) const {
    errorMessage.clear();
    if (!borrowDao_) {
        errorMessage = "数据库尚未初始化。";
        return false;
    }
    if (!borrowDao_->cancelReservation(reservationId, operatorId, "前端取消预约")) {
        errorMessage = "取消预约失败，请检查预约记录。";
        return false;
    }
    return true;
}

bool LibraryRepository::reportLost(long long recordId, long long operatorId, std::string& errorMessage) const {
    errorMessage.clear();
    if (!borrowDao_) {
        errorMessage = "数据库尚未初始化。";
        return false;
    }
    if (!borrowDao_->reportLost(recordId, operatorId, "前端挂失")) {
        errorMessage = "挂失失败，请检查借阅记录。";
        return false;
    }
    return true;
}

std::vector<ReservationRecord> LibraryRepository::getAllReservations() const {
    return borrowDao_ ? borrowDao_->getAllReservations() : std::vector<ReservationRecord>{};
}

std::vector<ReservationRecord> LibraryRepository::getReservationsByUser(long long userId) const {
    return borrowDao_ ? borrowDao_->getReservationsByUser(userId) : std::vector<ReservationRecord>{};
}

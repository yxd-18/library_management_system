#include "MenuCommon.h"

#include <iostream>

std::string trimMenuText(const std::string& value) {
    const std::string whitespace = " \t\r\n";
    const size_t start = value.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }

    const size_t end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

std::string promptLine(const std::string& label) {
    std::cout << label;
    std::string value;
    std::getline(std::cin, value);
    return trimMenuText(value);
}

int parseInt(const std::string& text, int defaultValue) {
    if (text.empty()) {
        return defaultValue;
    }
    try {
        return std::stoi(text);
    }
    catch (...) {
        return defaultValue;
    }
}

long long parseLongLong(const std::string& text, long long defaultValue) {
    if (text.empty()) {
        return defaultValue;
    }
    try {
        return std::stoll(text);
    }
    catch (...) {
        return defaultValue;
    }
}

double parseDouble(const std::string& text, double defaultValue) {
    if (text.empty()) {
        return defaultValue;
    }
    try {
        return std::stod(text);
    }
    catch (...) {
        return defaultValue;
    }
}

int promptInt(const std::string& label, int defaultValue) {
    return parseInt(promptLine(label), defaultValue);
}

long long promptLongLong(const std::string& label, long long defaultValue) {
    return parseLongLong(promptLine(label), defaultValue);
}

double promptDouble(const std::string& label, double defaultValue) {
    return parseDouble(promptLine(label), defaultValue);
}

Book inputBook(bool includeId) {
    Book book;
    if (includeId) {
        book.bookId = promptLongLong("图书ID: ");
    }
    book.isbn = promptLine("ISBN: ");
    book.bookTitle = promptLine("书名: ");
    book.author = promptLine("作者: ");
    book.publisher = promptLine("出版社: ");
    book.publishDate = promptLine("出版日期(yyyy-mm-dd，可留空): ");
    book.categoryId = promptInt("分类ID(无则填0): ");
    book.price = promptDouble("价格: ");
    book.totalStock = promptInt("总库存: ");
    book.availableStock = promptInt("可借库存: ");
    book.location = promptLine("馆藏位置: ");
    book.description = promptLine("图书简介: ");
    return book;
}

BookCategory inputCategory(bool includeId) {
    BookCategory category;
    if (includeId) {
        category.categoryId = promptInt("分类ID: ");
    }
    category.categoryName = promptLine("分类名称: ");
    category.parentId = promptInt("父分类ID(无则填0): ");
    category.categoryDesc = promptLine("分类说明: ");
    return category;
}

void printBook(const Book& book) {
    std::cout << "ID: " << book.bookId
        << " | 书名: " << book.bookTitle
        << " | ISBN: " << book.isbn
        << " | 作者: " << book.author
        << " | 分类: " << book.categoryName
        << " | 总库存: " << book.totalStock
        << " | 可借: " << book.availableStock
        << " | 挂失: " << book.lostCount
        << " | 预约: " << book.pendingReservationCount << "排队/" << book.readyReservationCount << "到书"
        << " | 状态: " << book.bookStatus
        << " | 位置: " << book.location
        << std::endl;
}

void printCategory(const BookCategory& category) {
    std::cout << "ID: " << category.categoryId
        << " | 名称: " << category.categoryName
        << " | 父分类: " << category.parentName
        << " | 说明: " << category.categoryDesc
        << std::endl;
}

void printBorrowRecord(const BorrowRecord& record) {
    std::cout << "记录ID: " << record.recordId
        << " | 用户: " << record.username
        << " | 图书: " << record.bookTitle
        << " | 借阅时间: " << record.borrowTime
        << " | 应还时间: " << record.dueTime
        << " | 状态: " << record.recordStatus
        << " | 续借: " << record.renewCount << "/" << record.maxRenewCount
        << " | 逾期天数: " << record.overdueDays
        << " | 罚金: " << record.fineAmount
        << std::endl;
}

void printReservationRecord(const ReservationRecord& record) {
    std::cout << "预约ID: " << record.reservationId
        << " | 用户: " << record.username
        << " | 图书: " << record.bookTitle
        << " | 预约时间: " << record.reservationTime
        << " | 到书时间: " << record.readyTime
        << " | 取书截止: " << record.pickupDeadline
        << " | 状态: " << record.reservationStatus
        << std::endl;
}

void printBackupRecord(const BackupRecord& record) {
    std::cout << "备份ID: " << record.backupId
        << " | 名称: " << record.backupName
        << " | 类型: " << record.backupType
        << " | 操作人: " << record.operatorName
        << " | 时间: " << record.backupTime
        << " | 路径: " << record.filePath
        << std::endl;
}

void printSettings(const UserSettings& settings) {
    std::cout << "用户ID: " << settings.userId
        << " | 用户名: " << settings.username
        << " | 字号: " << settings.fontSize
        << " | 主题: " << settings.theme
        << " | 语言: " << settings.languagePref
        << " | 每页条数: " << settings.pageSize
        << " | 通知: " << (settings.enableNotification ? "开" : "关")
        << std::endl;
}

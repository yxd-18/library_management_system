#ifndef BOOK_H
#define BOOK_H

#include <string>

struct Book {
    long long bookId = 0;
    std::string isbn;
    std::string bookTitle;
    std::string author;
    std::string publisher;
    std::string publishDate;       // yyyy-mm-dd
    int categoryId = 0;
    std::string categoryName;
    double price = 0.0;
    int totalStock = 0;
    int availableStock = 0;
    int lostCount = 0;
    int pendingReservationCount = 0;
    int readyReservationCount = 0;
    std::string location;
    std::string bookStatus;        // 可借 / 部分借出 / 无库存 / 下架 / 损坏
    std::string description;
    std::string createTime;
    std::string updateTime;
};

#endif


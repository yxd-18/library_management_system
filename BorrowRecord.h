#ifndef BORROWRECORD_H
#define BORROWRECORD_H

#include <string>

struct BorrowRecord {
    long long recordId = 0;
    long long userId = 0;
    std::string username;
    std::string realName;
    long long bookId = 0;
    std::string bookTitle;
    std::string borrowTime;
    std::string dueTime;
    std::string returnTime;
    int renewCount = 0;
    int maxRenewCount = 1;
    std::string recordStatus;
    int overdueDays = 0;
    double fineAmount = 0.0;
    long long operatorId = 0;
    std::string operatorName;
    std::string remark;
};

#endif

#ifndef RESERVATIONRECORD_H
#define RESERVATIONRECORD_H

#include <string>

struct ReservationRecord {
    long long reservationId = 0;
    long long userId = 0;
    std::string username;
    std::string realName;
    long long bookId = 0;
    std::string bookTitle;
    std::string reservationTime;
    std::string readyTime;
    std::string pickupDeadline;
    std::string fulfillTime;
    std::string cancelTime;
    std::string reservationStatus;
    long long operatorId = 0;
    std::string operatorName;
    std::string remark;
};

#endif

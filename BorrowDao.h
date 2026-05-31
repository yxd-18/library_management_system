#ifndef BORROWDAO_H
#define BORROWDAO_H

#include <optional>
#include <string>
#include <vector>
#include "BorrowRecord.h"
#include "DatabaseManager.h"
#include "OperationLogDao.h"
#include "ReservationRecord.h"

class BorrowDao {
private:
    DatabaseManager& db;
    OperationLogDao& logDao;

public:
    BorrowDao(DatabaseManager& database, OperationLogDao& operationLogDao);

    bool refreshOverdueRecords();
    bool borrowBook(long long userId, long long bookId, long long operatorId, const std::string& remark = "");
    bool returnBook(long long recordId, long long operatorId, const std::string& remark = "");
    bool renewBook(long long recordId, long long operatorId, const std::string& remark = "");
    bool reserveBook(long long userId, long long bookId, long long operatorId, const std::string& remark = "");
    bool cancelReservation(long long reservationId, long long operatorId, const std::string& remark = "");
    bool reportLost(long long recordId, long long operatorId, const std::string& remark = "");
    std::vector<BorrowRecord> getAllRecords();
    std::vector<BorrowRecord> getActiveRecords();
    std::vector<BorrowRecord> getOverdueRecords();
    std::vector<BorrowRecord> getRecordsByUser(long long userId);
    std::optional<BorrowRecord> getRecordById(long long recordId);
    std::vector<ReservationRecord> getAllReservations();
    std::vector<ReservationRecord> getReservationsByUser(long long userId);
    std::optional<ReservationRecord> getReservationById(long long reservationId);
};

#endif

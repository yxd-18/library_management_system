#ifndef BORROWMENUSERVICE_H
#define BORROWMENUSERVICE_H

#include <optional>
#include <string>
#include <vector>
#include "BorrowDao.h"
#include "BorrowRecord.h"
#include "ReservationRecord.h"
#include "ServiceResult.h"

ActionResult createBorrowRecord(BorrowDao& borrowDao, long long userId, long long bookId, long long operatorId, const std::string& remark);
ActionResult returnBorrowedBook(BorrowDao& borrowDao, long long recordId, long long operatorId, const std::string& remark);
ActionResult renewBorrowedBook(BorrowDao& borrowDao, long long recordId, long long operatorId, const std::string& remark);
ActionResult createReservationRecord(BorrowDao& borrowDao, long long userId, long long bookId, long long operatorId, const std::string& remark);
ActionResult cancelReservationRecord(BorrowDao& borrowDao, long long reservationId, long long operatorId, const std::string& remark);
ActionResult reportLostBorrowedBook(BorrowDao& borrowDao, long long recordId, long long operatorId, const std::string& remark);

DataResult<std::vector<BorrowRecord>> listBorrowRecords(BorrowDao& borrowDao);
DataResult<std::vector<BorrowRecord>> listActiveBorrowRecords(BorrowDao& borrowDao);
DataResult<std::vector<BorrowRecord>> listOverdueBorrowRecords(BorrowDao& borrowDao);
DataResult<std::vector<BorrowRecord>> listBorrowRecordsByUser(BorrowDao& borrowDao, long long userId);
DataResult<std::optional<BorrowRecord>> findBorrowRecordById(BorrowDao& borrowDao, long long recordId);
DataResult<std::vector<ReservationRecord>> listReservationRecords(BorrowDao& borrowDao);
DataResult<std::vector<ReservationRecord>> listReservationRecordsByUser(BorrowDao& borrowDao, long long userId);
DataResult<std::optional<ReservationRecord>> findReservationRecordById(BorrowDao& borrowDao, long long reservationId);

ActionResult refreshBorrowOverdueStatus(BorrowDao& borrowDao);

#endif

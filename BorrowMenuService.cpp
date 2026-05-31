#include "BorrowMenuService.h"

ActionResult createBorrowRecord(BorrowDao& borrowDao, long long userId, long long bookId, long long operatorId, const std::string& remark) {
    ActionResult result;
    result.success = borrowDao.borrowBook(userId, bookId, operatorId, remark);
    result.message = result.success ? "借书成功！" : "借书失败！";
    return result;
}

ActionResult returnBorrowedBook(BorrowDao& borrowDao, long long recordId, long long operatorId, const std::string& remark) {
    ActionResult result;
    result.success = borrowDao.returnBook(recordId, operatorId, remark);
    result.message = result.success ? "还书成功！" : "还书失败！";
    return result;
}

ActionResult renewBorrowedBook(BorrowDao& borrowDao, long long recordId, long long operatorId, const std::string& remark) {
    ActionResult result;
    result.success = borrowDao.renewBook(recordId, operatorId, remark);
    result.message = result.success ? "续借成功！" : "续借失败！";
    return result;
}

ActionResult createReservationRecord(BorrowDao& borrowDao, long long userId, long long bookId, long long operatorId, const std::string& remark) {
    ActionResult result;
    result.success = borrowDao.reserveBook(userId, bookId, operatorId, remark);
    result.message = result.success ? "预约成功！" : "预约失败！";
    return result;
}

ActionResult cancelReservationRecord(BorrowDao& borrowDao, long long reservationId, long long operatorId, const std::string& remark) {
    ActionResult result;
    result.success = borrowDao.cancelReservation(reservationId, operatorId, remark);
    result.message = result.success ? "取消预约成功！" : "取消预约失败！";
    return result;
}

ActionResult reportLostBorrowedBook(BorrowDao& borrowDao, long long recordId, long long operatorId, const std::string& remark) {
    ActionResult result;
    result.success = borrowDao.reportLost(recordId, operatorId, remark);
    result.message = result.success ? "挂失处理成功！" : "挂失处理失败！";
    return result;
}

DataResult<std::vector<BorrowRecord>> listBorrowRecords(BorrowDao& borrowDao) {
    DataResult<std::vector<BorrowRecord>> result;
    result.success = true;
    result.data = borrowDao.getAllRecords();
    result.message = result.data.empty() ? "当前暂无借阅记录。" : "查询借阅记录成功。";
    return result;
}

DataResult<std::vector<BorrowRecord>> listActiveBorrowRecords(BorrowDao& borrowDao) {
    DataResult<std::vector<BorrowRecord>> result;
    result.success = true;
    result.data = borrowDao.getActiveRecords();
    result.message = result.data.empty() ? "当前暂无未归还记录。" : "查询未归还记录成功。";
    return result;
}

DataResult<std::vector<BorrowRecord>> listOverdueBorrowRecords(BorrowDao& borrowDao) {
    DataResult<std::vector<BorrowRecord>> result;
    result.success = true;
    result.data = borrowDao.getOverdueRecords();
    result.message = result.data.empty() ? "当前暂无逾期记录。" : "查询逾期记录成功。";
    return result;
}

DataResult<std::vector<BorrowRecord>> listBorrowRecordsByUser(BorrowDao& borrowDao, long long userId) {
    DataResult<std::vector<BorrowRecord>> result;
    result.success = true;
    result.data = borrowDao.getRecordsByUser(userId);
    result.message = result.data.empty() ? "该用户暂无借阅记录。" : "查询用户借阅记录成功。";
    return result;
}

DataResult<std::optional<BorrowRecord>> findBorrowRecordById(BorrowDao& borrowDao, long long recordId) {
    DataResult<std::optional<BorrowRecord>> result;
    result.data = borrowDao.getRecordById(recordId);
    result.success = result.data.has_value();
    result.message = result.success ? "查询借阅记录成功。" : "未找到该借阅记录！";
    return result;
}

DataResult<std::vector<ReservationRecord>> listReservationRecords(BorrowDao& borrowDao) {
    DataResult<std::vector<ReservationRecord>> result;
    result.success = true;
    result.data = borrowDao.getAllReservations();
    result.message = result.data.empty() ? "当前暂无预约记录。" : "查询预约记录成功。";
    return result;
}

DataResult<std::vector<ReservationRecord>> listReservationRecordsByUser(BorrowDao& borrowDao, long long userId) {
    DataResult<std::vector<ReservationRecord>> result;
    result.success = true;
    result.data = borrowDao.getReservationsByUser(userId);
    result.message = result.data.empty() ? "该用户暂无预约记录。" : "查询用户预约记录成功。";
    return result;
}

DataResult<std::optional<ReservationRecord>> findReservationRecordById(BorrowDao& borrowDao, long long reservationId) {
    DataResult<std::optional<ReservationRecord>> result;
    result.data = borrowDao.getReservationById(reservationId);
    result.success = result.data.has_value();
    result.message = result.success ? "查询预约记录成功。" : "未找到该预约记录！";
    return result;
}

ActionResult refreshBorrowOverdueStatus(BorrowDao& borrowDao) {
    ActionResult result;
    result.success = borrowDao.refreshOverdueRecords();
    result.message = result.success ? "逾期状态刷新完成！" : "逾期状态刷新失败！";
    return result;
}

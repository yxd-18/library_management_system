#include "BorrowDao.h"

#include <cstdlib>
#include <iostream>
#include <libpq-fe.h>
#include <sstream>

namespace {
constexpr int kMaxActiveBorrowCount = 5;
constexpr int kMaxActiveReservationCount = 3;

bool execSimple(PGconn* conn, const char* sql) {
    PGresult* res = PQexec(conn, sql);
    const bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    if (!ok) {
        std::cerr << "SQL 执行失败: " << PQerrorMessage(conn) << std::endl;
    }
    PQclear(res);
    return ok;
}

bool hasAffectedRows(PGresult* res) {
    const char* tuples = PQcmdTuples(res);
    return tuples != nullptr && tuples[0] != '\0' && std::atoi(tuples) > 0;
}

double parseAmount(const char* text) {
    if (text == nullptr || text[0] == '\0') {
        return 0.0;
    }
    return std::atof(text);
}

int queryCount(PGconn* conn, const char* sql, int paramCount, const char* const* params, const char* errorMessage) {
    PGresult* res = PQexecParams(conn, sql, paramCount, nullptr, params, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << errorMessage << ": " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return -1;
    }

    const int count = std::atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return count;
}

BorrowRecord buildBorrowRecordFromRow(PGresult* res, int row) {
    BorrowRecord record;
    record.recordId = std::atoll(PQgetvalue(res, row, 0));
    record.userId = std::atoll(PQgetvalue(res, row, 1));
    record.username = PQgetvalue(res, row, 2);
    record.realName = PQgetvalue(res, row, 3);
    record.bookId = std::atoll(PQgetvalue(res, row, 4));
    record.bookTitle = PQgetvalue(res, row, 5);
    record.borrowTime = PQgetvalue(res, row, 6);
    record.dueTime = PQgetvalue(res, row, 7);
    record.returnTime = PQgetvalue(res, row, 8);
    record.renewCount = std::atoi(PQgetvalue(res, row, 9));
    record.maxRenewCount = std::atoi(PQgetvalue(res, row, 10));
    record.recordStatus = PQgetvalue(res, row, 11);
    record.overdueDays = std::atoi(PQgetvalue(res, row, 12));
    record.fineAmount = parseAmount(PQgetvalue(res, row, 13));
    record.operatorId = std::atoll(PQgetvalue(res, row, 14));
    record.operatorName = PQgetvalue(res, row, 15);
    record.remark = PQgetvalue(res, row, 16);
    return record;
}

ReservationRecord buildReservationRecordFromRow(PGresult* res, int row) {
    ReservationRecord record;
    record.reservationId = std::atoll(PQgetvalue(res, row, 0));
    record.userId = std::atoll(PQgetvalue(res, row, 1));
    record.username = PQgetvalue(res, row, 2);
    record.realName = PQgetvalue(res, row, 3);
    record.bookId = std::atoll(PQgetvalue(res, row, 4));
    record.bookTitle = PQgetvalue(res, row, 5);
    record.reservationTime = PQgetvalue(res, row, 6);
    record.readyTime = PQgetvalue(res, row, 7);
    record.pickupDeadline = PQgetvalue(res, row, 8);
    record.fulfillTime = PQgetvalue(res, row, 9);
    record.cancelTime = PQgetvalue(res, row, 10);
    record.reservationStatus = PQgetvalue(res, row, 11);
    record.operatorId = std::atoll(PQgetvalue(res, row, 12));
    record.operatorName = PQgetvalue(res, row, 13);
    record.remark = PQgetvalue(res, row, 14);
    return record;
}

const char* kBorrowRecordSelect =
    "SELECT br.record_id, br.user_id, u.username, u.real_name, "
    "br.book_id, b.book_title, "
    "COALESCE(br.borrow_time::text, ''), COALESCE(br.due_time::text, ''), "
    "COALESCE(br.return_time::text, ''), br.renew_count, br.max_renew_count, "
    "br.record_status::text, br.overdue_days, COALESCE(br.fine_amount, 0)::text, "
    "COALESCE(br.operator_id, 0), COALESCE(op.username, ''), COALESCE(br.remark, '') "
    "FROM borrow_records br "
    "JOIN users u ON br.user_id = u.user_id "
    "JOIN books b ON br.book_id = b.book_id "
    "LEFT JOIN users op ON br.operator_id = op.user_id ";

const char* kReservationRecordSelect =
    "SELECT rr.reservation_id, rr.user_id, u.username, u.real_name, "
    "rr.book_id, b.book_title, "
    "COALESCE(rr.reservation_time::text, ''), COALESCE(rr.ready_time::text, ''), "
    "COALESCE(rr.pickup_deadline::text, ''), COALESCE(rr.fulfill_time::text, ''), "
    "COALESCE(rr.cancel_time::text, ''), rr.reservation_status::text, "
    "COALESCE(rr.operator_id, 0), COALESCE(op.username, ''), COALESCE(rr.remark, '') "
    "FROM reservation_records rr "
    "JOIN users u ON rr.user_id = u.user_id "
    "JOIN books b ON rr.book_id = b.book_id "
    "LEFT JOIN users op ON rr.operator_id = op.user_id ";

bool markNextReservationReady(PGconn* conn, long long bookId, long long operatorId) {
    const std::string bookIdStr = std::to_string(bookId);
    const std::string operatorIdStr = std::to_string(operatorId);
    const char* countParams[1] = { bookIdStr.c_str() };

    PGresult* countRes = PQexecParams(
        conn,
        "SELECT COALESCE(b.available_stock, 0), "
        "       COALESCE((SELECT COUNT(*) FROM reservation_records rr "
        "                 WHERE rr.book_id = b.book_id AND rr.reservation_status = '可取书'), 0) "
        "FROM books b WHERE b.book_id = $1",
        1,
        nullptr,
        countParams,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(countRes) != PGRES_TUPLES_OK) {
        std::cerr << "检查预约放行数量失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(countRes);
        return false;
    }
    if (PQntuples(countRes) == 0) {
        PQclear(countRes);
        return true;
    }

    const int availableStock = std::atoi(PQgetvalue(countRes, 0, 0));
    const int readyCount = std::atoi(PQgetvalue(countRes, 0, 1));
    PQclear(countRes);

    if (availableStock <= readyCount) {
        return true;
    }

    const char* params[2] = { bookIdStr.c_str(), operatorIdStr.c_str() };

    PGresult* res = PQexecParams(
        conn,
        "WITH next_reservation AS ("
        "    SELECT reservation_id "
        "    FROM reservation_records "
        "    WHERE book_id = $1 AND reservation_status = '排队中' "
        "    ORDER BY reservation_time, reservation_id "
        "    LIMIT 1"
        ") "
        "UPDATE reservation_records rr "
        "SET reservation_status = '可取书', "
        "    ready_time = CURRENT_TIMESTAMP, "
        "    pickup_deadline = CURRENT_TIMESTAMP + INTERVAL '3 day', "
        "    operator_id = $2 "
        "FROM next_reservation nr "
        "WHERE rr.reservation_id = nr.reservation_id",
        2,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "更新预约到书状态失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

bool completeReservationForBorrow(PGconn* conn, long long userId, long long bookId, long long operatorId) {
    const std::string userIdStr = std::to_string(userId);
    const std::string bookIdStr = std::to_string(bookId);
    const std::string operatorIdStr = std::to_string(operatorId);
    const char* params[3] = { userIdStr.c_str(), bookIdStr.c_str(), operatorIdStr.c_str() };

    PGresult* res = PQexecParams(
        conn,
        "WITH current_reservation AS ("
        "    SELECT reservation_id "
        "    FROM reservation_records "
        "    WHERE user_id = $1 AND book_id = $2 "
        "      AND reservation_status IN ('排队中', '可取书') "
        "    ORDER BY reservation_id "
        "    LIMIT 1"
        ") "
        "UPDATE reservation_records rr "
        "SET reservation_status = '已完成', "
        "    ready_time = COALESCE(rr.ready_time, CURRENT_TIMESTAMP), "
        "    pickup_deadline = COALESCE(rr.pickup_deadline, CURRENT_TIMESTAMP + INTERVAL '3 day'), "
        "    fulfill_time = CURRENT_TIMESTAMP, "
        "    operator_id = $3 "
        "FROM current_reservation cr "
        "WHERE rr.reservation_id = cr.reservation_id",
        3,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "更新预约完成状态失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}
}

BorrowDao::BorrowDao(DatabaseManager& database, OperationLogDao& operationLogDao)
    : db(database), logDao(operationLogDao) {
}

bool BorrowDao::refreshOverdueRecords() {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    PGresult* res = PQexec(
        conn,
        "UPDATE borrow_records "
        "SET record_status = CASE "
        "        WHEN due_time < CURRENT_TIMESTAMP THEN '已逾期'::borrow_status_enum "
        "        ELSE '借阅中'::borrow_status_enum "
        "    END, "
        "    overdue_days = CASE "
        "        WHEN due_time < CURRENT_TIMESTAMP THEN GREATEST(0, CURRENT_DATE - due_time::date) "
        "        ELSE 0 "
        "    END, "
        "    fine_amount = CASE "
        "        WHEN due_time < CURRENT_TIMESTAMP THEN ROUND((GREATEST(0, CURRENT_DATE - due_time::date) * 0.50)::numeric, 2) "
        "        ELSE 0 "
        "    END "
        "WHERE return_time IS NULL "
        "  AND record_status <> '已挂失'::borrow_status_enum"
    );

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "刷新逾期记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

bool BorrowDao::borrowBook(long long userId, long long bookId, long long operatorId, const std::string& remark) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    refreshOverdueRecords();

    const std::string userIdStr = std::to_string(userId);
    const std::string bookIdStr = std::to_string(bookId);
    const std::string operatorIdStr = std::to_string(operatorId);

    const char* userParams[1] = { userIdStr.c_str() };
    const int validUserCount = queryCount(
        conn,
        "SELECT COUNT(*) FROM users WHERE user_id = $1 AND status = '正常'",
        1,
        userParams,
        "校验借阅用户失败"
    );
    if (validUserCount <= 0) {
        std::cerr << "用户不存在或已被禁用，不能借书！" << std::endl;
        return false;
    }

    const int activeBorrowCount = queryCount(
        conn,
        "SELECT COUNT(*) FROM borrow_records "
        "WHERE user_id = $1 AND return_time IS NULL "
        "  AND record_status IN ('借阅中', '已逾期')",
        1,
        userParams,
        "检查借阅上限失败"
    );
    if (activeBorrowCount < 0) {
        return false;
    }
    if (activeBorrowCount >= kMaxActiveBorrowCount) {
        std::cerr << "该用户已达到最大借阅上限(" << kMaxActiveBorrowCount << "本)！" << std::endl;
        return false;
    }

    const int overdueCount = queryCount(
        conn,
        "SELECT COUNT(*) FROM borrow_records "
        "WHERE user_id = $1 AND return_time IS NULL AND record_status = '已逾期'",
        1,
        userParams,
        "检查逾期记录失败"
    );
    if (overdueCount < 0) {
        return false;
    }
    if (overdueCount > 0) {
        std::cerr << "该用户存在逾期未还记录，不能继续借书！" << std::endl;
        return false;
    }

    const char* duplicateParams[2] = { userIdStr.c_str(), bookIdStr.c_str() };
    const int duplicateCount = queryCount(
        conn,
        "SELECT COUNT(*) FROM borrow_records "
        "WHERE user_id = $1 AND book_id = $2 AND return_time IS NULL",
        2,
        duplicateParams,
        "检查重复借阅失败"
    );
    if (duplicateCount < 0) {
        return false;
    }
    if (duplicateCount > 0) {
        std::cerr << "同一个用户对同一本书存在未归还记录，不能重复借阅！" << std::endl;
        return false;
    }

    PGresult* readyRes = PQexecParams(
        conn,
        "SELECT "
        "    COALESCE((SELECT COUNT(*) FROM reservation_records "
        "              WHERE book_id = $1 AND reservation_status = '可取书'), 0), "
        "    COALESCE((SELECT COUNT(*) FROM reservation_records "
        "              WHERE book_id = $1 AND user_id = $2 AND reservation_status = '可取书'), 0)",
        2,
        nullptr,
        duplicateParams,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(readyRes) != PGRES_TUPLES_OK) {
        std::cerr << "检查到书预约失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(readyRes);
        return false;
    }
    const int readyReservationCount = PQntuples(readyRes) > 0 ? std::atoi(PQgetvalue(readyRes, 0, 0)) : 0;
    const int currentUserReadyCount = PQntuples(readyRes) > 0 ? std::atoi(PQgetvalue(readyRes, 0, 1)) : 0;
    PQclear(readyRes);

    const char* bookParams[1] = { bookIdStr.c_str() };
    PGresult* bookRes = PQexecParams(
        conn,
        "SELECT book_title, available_stock, book_status::text "
        "FROM books WHERE book_id = $1",
        1,
        nullptr,
        bookParams,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(bookRes) != PGRES_TUPLES_OK) {
        std::cerr << "校验图书信息失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(bookRes);
        return false;
    }
    if (PQntuples(bookRes) == 0) {
        std::cerr << "未找到要借阅的图书！" << std::endl;
        PQclear(bookRes);
        return false;
    }

    const std::string bookTitle = PQgetvalue(bookRes, 0, 0);
    const int availableStock = std::atoi(PQgetvalue(bookRes, 0, 1));
    const std::string bookStatus = PQgetvalue(bookRes, 0, 2);
    PQclear(bookRes);

    if (availableStock <= 0 || bookStatus == "无库存" || bookStatus == "下架" || bookStatus == "损坏") {
        std::cerr << "该图书当前不可借阅！如确有需要，请先预约。" << std::endl;
        return false;
    }
    if (readyReservationCount > 0 && availableStock <= readyReservationCount && currentUserReadyCount == 0) {
        std::cerr << "该图书当前可借库存已被预约用户占用，暂不能外借！" << std::endl;
        return false;
    }

    if (!execSimple(conn, "BEGIN")) {
        return false;
    }

    const char* insertParams[4] = {
        userIdStr.c_str(),
        bookIdStr.c_str(),
        operatorIdStr.c_str(),
        remark.c_str()
    };
    PGresult* insertRes = PQexecParams(
        conn,
        "INSERT INTO borrow_records "
        "(user_id, book_id, borrow_time, due_time, record_status, overdue_days, fine_amount, operator_id, remark) "
        "VALUES ($1, $2, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP + INTERVAL '30 day', '借阅中', 0, 0, $3, $4) "
        "RETURNING record_id",
        4,
        nullptr,
        insertParams,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(insertRes) != PGRES_TUPLES_OK) {
        std::cerr << "新增借阅记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(insertRes);
        execSimple(conn, "ROLLBACK");
        return false;
    }

    const long long newRecordId = std::atoll(PQgetvalue(insertRes, 0, 0));
    PQclear(insertRes);

    PGresult* stockRes = PQexecParams(
        conn,
        "UPDATE books "
        "SET available_stock = available_stock - 1, "
        "    book_status = CASE "
        "        WHEN book_status IN ('下架', '损坏') THEN book_status "
        "        WHEN total_stock <= 0 OR available_stock - 1 <= 0 THEN '无库存'::book_status_enum "
        "        WHEN available_stock - 1 < total_stock THEN '部分借出'::book_status_enum "
        "        ELSE '可借'::book_status_enum "
        "    END, "
        "    update_time = CURRENT_TIMESTAMP "
        "WHERE book_id = $1 AND available_stock > 0",
        1,
        nullptr,
        bookParams,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(stockRes) != PGRES_COMMAND_OK || !hasAffectedRows(stockRes)) {
        std::cerr << "更新图书库存失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(stockRes);
        execSimple(conn, "ROLLBACK");
        return false;
    }
    PQclear(stockRes);

    if (!completeReservationForBorrow(conn, userId, bookId, operatorId)) {
        execSimple(conn, "ROLLBACK");
        return false;
    }

    if (!execSimple(conn, "COMMIT")) {
        execSimple(conn, "ROLLBACK");
        return false;
    }

    std::ostringstream oss;
    oss << "借书：记录ID=" << newRecordId
        << "，用户ID=" << userId
        << "，图书ID=" << bookId
        << "，书名=" << bookTitle;
    logDao.addLog(operatorId, "借书", "borrow_records", newRecordId, oss.str());
    return true;
}

bool BorrowDao::returnBook(long long recordId, long long operatorId, const std::string& remark) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    refreshOverdueRecords();

    const std::string recordIdStr = std::to_string(recordId);
    const std::string operatorIdStr = std::to_string(operatorId);
    const char* recordParams[1] = { recordIdStr.c_str() };

    PGresult* recordRes = PQexecParams(
        conn,
        "SELECT br.book_id, b.book_title "
        "FROM borrow_records br "
        "JOIN books b ON br.book_id = b.book_id "
        "WHERE br.record_id = $1 AND br.return_time IS NULL",
        1,
        nullptr,
        recordParams,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(recordRes) != PGRES_TUPLES_OK) {
        std::cerr << "查询借阅记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(recordRes);
        return false;
    }
    if (PQntuples(recordRes) == 0) {
        std::cerr << "未找到可归还的借阅记录！" << std::endl;
        PQclear(recordRes);
        return false;
    }

    const std::string bookIdStr = PQgetvalue(recordRes, 0, 0);
    const std::string bookTitle = PQgetvalue(recordRes, 0, 1);
    PQclear(recordRes);

    if (!execSimple(conn, "BEGIN")) {
        return false;
    }

    const char* updateParams[3] = {
        operatorIdStr.c_str(),
        remark.c_str(),
        recordIdStr.c_str()
    };
    PGresult* updateRes = PQexecParams(
        conn,
        "UPDATE borrow_records "
        "SET return_time = CURRENT_TIMESTAMP, "
        "    record_status = '已归还', "
        "    overdue_days = GREATEST(0, CURRENT_DATE - due_time::date), "
        "    fine_amount = ROUND((GREATEST(0, CURRENT_DATE - due_time::date) * 0.50)::numeric, 2), "
        "    operator_id = $1, "
        "    remark = CASE WHEN $2 = '' THEN remark ELSE $2 END "
        "WHERE record_id = $3 AND return_time IS NULL",
        3,
        nullptr,
        updateParams,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(updateRes) != PGRES_COMMAND_OK || !hasAffectedRows(updateRes)) {
        std::cerr << "归还图书失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(updateRes);
        execSimple(conn, "ROLLBACK");
        return false;
    }
    PQclear(updateRes);

    const char* bookParams[1] = { bookIdStr.c_str() };
    PGresult* stockRes = PQexecParams(
        conn,
        "UPDATE books "
        "SET available_stock = available_stock + 1, "
        "    book_status = CASE "
        "        WHEN book_status IN ('下架', '损坏') THEN book_status "
        "        WHEN total_stock <= 0 OR available_stock + 1 <= 0 THEN '无库存'::book_status_enum "
        "        WHEN available_stock + 1 < total_stock THEN '部分借出'::book_status_enum "
        "        ELSE '可借'::book_status_enum "
        "    END, "
        "    update_time = CURRENT_TIMESTAMP "
        "WHERE book_id = $1",
        1,
        nullptr,
        bookParams,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(stockRes) != PGRES_COMMAND_OK || !hasAffectedRows(stockRes)) {
        std::cerr << "恢复图书库存失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(stockRes);
        execSimple(conn, "ROLLBACK");
        return false;
    }
    PQclear(stockRes);

    if (!markNextReservationReady(conn, std::atoll(bookIdStr.c_str()), operatorId)) {
        execSimple(conn, "ROLLBACK");
        return false;
    }

    if (!execSimple(conn, "COMMIT")) {
        execSimple(conn, "ROLLBACK");
        return false;
    }

    std::ostringstream oss;
    oss << "还书：记录ID=" << recordId
        << "，图书ID=" << bookIdStr
        << "，书名=" << bookTitle;
    logDao.addLog(operatorId, "还书", "borrow_records", recordId, oss.str());
    return true;
}

bool BorrowDao::renewBook(long long recordId, long long operatorId, const std::string& remark) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    refreshOverdueRecords();

    const std::string recordIdStr = std::to_string(recordId);
    const std::string operatorIdStr = std::to_string(operatorId);
    const char* params[1] = { recordIdStr.c_str() };

    PGresult* checkRes = PQexecParams(
        conn,
        "SELECT br.renew_count, br.max_renew_count, br.record_status::text, b.book_title "
        "FROM borrow_records br "
        "JOIN books b ON br.book_id = b.book_id "
        "WHERE br.record_id = $1 AND br.return_time IS NULL",
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(checkRes) != PGRES_TUPLES_OK) {
        std::cerr << "查询续借记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(checkRes);
        return false;
    }
    if (PQntuples(checkRes) == 0) {
        std::cerr << "未找到可续借的记录！" << std::endl;
        PQclear(checkRes);
        return false;
    }

    const int renewCount = std::atoi(PQgetvalue(checkRes, 0, 0));
    const int maxRenewCount = std::atoi(PQgetvalue(checkRes, 0, 1));
    const std::string recordStatus = PQgetvalue(checkRes, 0, 2);
    const std::string bookTitle = PQgetvalue(checkRes, 0, 3);
    PQclear(checkRes);

    if (recordStatus == "已逾期") {
        std::cerr << "逾期记录不能续借！" << std::endl;
        return false;
    }
    if (recordStatus == "已挂失") {
        std::cerr << "已挂失记录不能续借！" << std::endl;
        return false;
    }
    if (renewCount >= maxRenewCount) {
        std::cerr << "该记录已达到最大续借次数！" << std::endl;
        return false;
    }

    const char* renewParams[3] = {
        operatorIdStr.c_str(),
        remark.c_str(),
        recordIdStr.c_str()
    };
    PGresult* renewRes = PQexecParams(
        conn,
        "UPDATE borrow_records "
        "SET due_time = due_time + INTERVAL '30 day', "
        "    renew_count = renew_count + 1, "
        "    overdue_days = 0, "
        "    fine_amount = 0, "
        "    record_status = '借阅中', "
        "    operator_id = $1, "
        "    remark = CASE WHEN $2 = '' THEN remark ELSE $2 END "
        "WHERE record_id = $3 AND return_time IS NULL",
        3,
        nullptr,
        renewParams,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(renewRes) != PGRES_COMMAND_OK || !hasAffectedRows(renewRes)) {
        std::cerr << "续借失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(renewRes);
        return false;
    }
    PQclear(renewRes);

    std::ostringstream oss;
    oss << "续借：记录ID=" << recordId << "，书名=" << bookTitle;
    logDao.addLog(operatorId, "续借", "borrow_records", recordId, oss.str());
    return true;
}

bool BorrowDao::reserveBook(long long userId, long long bookId, long long operatorId, const std::string& remark) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    const std::string userIdStr = std::to_string(userId);
    const std::string bookIdStr = std::to_string(bookId);
    const std::string operatorIdStr = std::to_string(operatorId);

    const char* userParams[1] = { userIdStr.c_str() };
    const int validUserCount = queryCount(
        conn,
        "SELECT COUNT(*) FROM users WHERE user_id = $1 AND status = '正常'",
        1,
        userParams,
        "校验预约用户失败"
    );
    if (validUserCount <= 0) {
        std::cerr << "用户不存在或已被禁用，不能预约！" << std::endl;
        return false;
    }

    const int activeReservationCount = queryCount(
        conn,
        "SELECT COUNT(*) FROM reservation_records "
        "WHERE user_id = $1 AND reservation_status IN ('排队中', '可取书')",
        1,
        userParams,
        "检查预约上限失败"
    );
    if (activeReservationCount < 0) {
        return false;
    }
    if (activeReservationCount >= kMaxActiveReservationCount) {
        std::cerr << "该用户已达到最大预约上限(" << kMaxActiveReservationCount << "本)！" << std::endl;
        return false;
    }

    const char* duplicateParams[2] = { userIdStr.c_str(), bookIdStr.c_str() };
    const int activeBorrowCount = queryCount(
        conn,
        "SELECT COUNT(*) FROM borrow_records "
        "WHERE user_id = $1 AND book_id = $2 AND return_time IS NULL",
        2,
        duplicateParams,
        "检查当前借阅失败"
    );
    if (activeBorrowCount < 0) {
        return false;
    }
    if (activeBorrowCount > 0) {
        std::cerr << "该用户已借阅此书，无需重复预约！" << std::endl;
        return false;
    }

    const int duplicateReservationCount = queryCount(
        conn,
        "SELECT COUNT(*) FROM reservation_records "
        "WHERE user_id = $1 AND book_id = $2 AND reservation_status IN ('排队中', '可取书')",
        2,
        duplicateParams,
        "检查重复预约失败"
    );
    if (duplicateReservationCount < 0) {
        return false;
    }
    if (duplicateReservationCount > 0) {
        std::cerr << "该用户已经预约过这本书！" << std::endl;
        return false;
    }

    const char* bookParams[1] = { bookIdStr.c_str() };
    PGresult* bookRes = PQexecParams(
        conn,
        "SELECT book_title, available_stock, book_status::text, "
        "       COALESCE((SELECT COUNT(*) FROM reservation_records rr "
        "                 WHERE rr.book_id = books.book_id AND rr.reservation_status = '可取书'), 0) "
        "FROM books WHERE book_id = $1",
        1,
        nullptr,
        bookParams,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(bookRes) != PGRES_TUPLES_OK) {
        std::cerr << "校验预约图书失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(bookRes);
        return false;
    }
    if (PQntuples(bookRes) == 0) {
        std::cerr << "未找到要预约的图书！" << std::endl;
        PQclear(bookRes);
        return false;
    }

    const std::string bookTitle = PQgetvalue(bookRes, 0, 0);
    const int availableStock = std::atoi(PQgetvalue(bookRes, 0, 1));
    const std::string bookStatus = PQgetvalue(bookRes, 0, 2);
    const int readyReservationCount = std::atoi(PQgetvalue(bookRes, 0, 3));
    PQclear(bookRes);

    if (bookStatus == "下架" || bookStatus == "损坏") {
        std::cerr << "该图书当前不可预约！" << std::endl;
        return false;
    }
    if (availableStock > readyReservationCount) {
        std::cerr << "该图书当前仍有库存，建议直接借阅而不是预约！" << std::endl;
        return false;
    }

    const char* insertParams[4] = {
        userIdStr.c_str(),
        bookIdStr.c_str(),
        operatorIdStr.c_str(),
        remark.c_str()
    };
    PGresult* insertRes = PQexecParams(
        conn,
        "INSERT INTO reservation_records "
        "(user_id, book_id, reservation_time, reservation_status, operator_id, remark) "
        "VALUES ($1, $2, CURRENT_TIMESTAMP, '排队中', $3, $4) "
        "RETURNING reservation_id",
        4,
        nullptr,
        insertParams,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(insertRes) != PGRES_TUPLES_OK) {
        std::cerr << "新增预约记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(insertRes);
        return false;
    }

    const long long reservationId = std::atoll(PQgetvalue(insertRes, 0, 0));
    PQclear(insertRes);

    std::ostringstream oss;
    oss << "预约图书：预约ID=" << reservationId
        << "，用户ID=" << userId
        << "，图书ID=" << bookId
        << "，书名=" << bookTitle;
    logDao.addLog(operatorId, "预约图书", "reservation_records", reservationId, oss.str());
    return true;
}

bool BorrowDao::cancelReservation(long long reservationId, long long operatorId, const std::string& remark) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    const std::string reservationIdStr = std::to_string(reservationId);
    const std::string operatorIdStr = std::to_string(operatorId);
    const char* params[1] = { reservationIdStr.c_str() };

    PGresult* checkRes = PQexecParams(
        conn,
        "SELECT rr.book_id, b.book_title, rr.reservation_status::text "
        "FROM reservation_records rr "
        "JOIN books b ON rr.book_id = b.book_id "
        "WHERE rr.reservation_id = $1",
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(checkRes) != PGRES_TUPLES_OK) {
        std::cerr << "查询预约记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(checkRes);
        return false;
    }
    if (PQntuples(checkRes) == 0) {
        std::cerr << "未找到预约记录！" << std::endl;
        PQclear(checkRes);
        return false;
    }

    const long long bookId = std::atoll(PQgetvalue(checkRes, 0, 0));
    const std::string bookTitle = PQgetvalue(checkRes, 0, 1);
    const std::string reservationStatus = PQgetvalue(checkRes, 0, 2);
    PQclear(checkRes);

    if (reservationStatus != "排队中" && reservationStatus != "可取书") {
        std::cerr << "当前预约状态不能取消！" << std::endl;
        return false;
    }

    if (!execSimple(conn, "BEGIN")) {
        return false;
    }

    const char* updateParams[3] = {
        operatorIdStr.c_str(),
        remark.c_str(),
        reservationIdStr.c_str()
    };
    PGresult* updateRes = PQexecParams(
        conn,
        "UPDATE reservation_records "
        "SET reservation_status = '已取消', "
        "    cancel_time = CURRENT_TIMESTAMP, "
        "    operator_id = $1, "
        "    remark = CASE WHEN $2 = '' THEN remark ELSE $2 END "
        "WHERE reservation_id = $3 "
        "  AND reservation_status IN ('排队中', '可取书')",
        3,
        nullptr,
        updateParams,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(updateRes) != PGRES_COMMAND_OK || !hasAffectedRows(updateRes)) {
        std::cerr << "取消预约失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(updateRes);
        execSimple(conn, "ROLLBACK");
        return false;
    }
    PQclear(updateRes);

    if (reservationStatus == "可取书" && !markNextReservationReady(conn, bookId, operatorId)) {
        execSimple(conn, "ROLLBACK");
        return false;
    }

    if (!execSimple(conn, "COMMIT")) {
        execSimple(conn, "ROLLBACK");
        return false;
    }

    std::ostringstream oss;
    oss << "取消预约：预约ID=" << reservationId << "，书名=" << bookTitle;
    logDao.addLog(operatorId, "取消预约", "reservation_records", reservationId, oss.str());
    return true;
}

bool BorrowDao::reportLost(long long recordId, long long operatorId, const std::string& remark) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return false;
    }

    refreshOverdueRecords();

    const std::string recordIdStr = std::to_string(recordId);
    const std::string operatorIdStr = std::to_string(operatorId);
    const char* params[1] = { recordIdStr.c_str() };

    PGresult* recordRes = PQexecParams(
        conn,
        "SELECT br.book_id, b.book_title, COALESCE(b.price, 0)::text "
        "FROM borrow_records br "
        "JOIN books b ON br.book_id = b.book_id "
        "WHERE br.record_id = $1 AND br.return_time IS NULL",
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(recordRes) != PGRES_TUPLES_OK) {
        std::cerr << "查询挂失记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(recordRes);
        return false;
    }
    if (PQntuples(recordRes) == 0) {
        std::cerr << "未找到可挂失的借阅记录！" << std::endl;
        PQclear(recordRes);
        return false;
    }

    const std::string bookIdStr = PQgetvalue(recordRes, 0, 0);
    const std::string bookTitle = PQgetvalue(recordRes, 0, 1);
    const std::string priceStr = PQgetvalue(recordRes, 0, 2);
    const double bookPrice = parseAmount(priceStr.c_str());
    PQclear(recordRes);

    if (!execSimple(conn, "BEGIN")) {
        return false;
    }

    const char* updateParams[4] = {
        operatorIdStr.c_str(),
        remark.c_str(),
        priceStr.c_str(),
        recordIdStr.c_str()
    };
    PGresult* updateRes = PQexecParams(
        conn,
        "UPDATE borrow_records "
        "SET return_time = CURRENT_TIMESTAMP, "
        "    record_status = '已挂失', "
        "    overdue_days = GREATEST(0, CURRENT_DATE - due_time::date), "
        "    fine_amount = ROUND(($3::numeric + GREATEST(0, CURRENT_DATE - due_time::date) * 0.50)::numeric, 2), "
        "    operator_id = $1, "
        "    remark = CASE WHEN $2 = '' THEN remark ELSE $2 END "
        "WHERE record_id = $4 AND return_time IS NULL",
        4,
        nullptr,
        updateParams,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(updateRes) != PGRES_COMMAND_OK || !hasAffectedRows(updateRes)) {
        std::cerr << "处理挂失失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(updateRes);
        execSimple(conn, "ROLLBACK");
        return false;
    }
    PQclear(updateRes);

    const char* bookParams[1] = { bookIdStr.c_str() };
    PGresult* stockRes = PQexecParams(
        conn,
        "UPDATE books "
        "SET total_stock = GREATEST(total_stock - 1, 0), "
        "    lost_count = lost_count + 1, "
        "    book_status = CASE "
        "        WHEN book_status IN ('下架', '损坏') THEN book_status "
        "        WHEN total_stock - 1 <= 0 OR available_stock <= 0 THEN '无库存'::book_status_enum "
        "        WHEN available_stock < total_stock - 1 THEN '部分借出'::book_status_enum "
        "        ELSE '可借'::book_status_enum "
        "    END, "
        "    update_time = CURRENT_TIMESTAMP "
        "WHERE book_id = $1",
        1,
        nullptr,
        bookParams,
        nullptr,
        nullptr,
        0
    );
    if (PQresultStatus(stockRes) != PGRES_COMMAND_OK || !hasAffectedRows(stockRes)) {
        std::cerr << "更新图书挂失库存失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(stockRes);
        execSimple(conn, "ROLLBACK");
        return false;
    }
    PQclear(stockRes);

    if (!execSimple(conn, "COMMIT")) {
        execSimple(conn, "ROLLBACK");
        return false;
    }

    std::ostringstream oss;
    oss << "图书挂失：记录ID=" << recordId
        << "，书名=" << bookTitle
        << "，赔偿金额=" << bookPrice;
    logDao.addLog(operatorId, "图书挂失", "borrow_records", recordId, oss.str());
    return true;
}

std::vector<BorrowRecord> BorrowDao::getAllRecords() {
    std::vector<BorrowRecord> records;
    PGconn* conn = db.getConnection();
    if (!conn) {
        return records;
    }

    refreshOverdueRecords();

    std::string sql = std::string(kBorrowRecordSelect) + "ORDER BY br.record_id DESC";
    PGresult* res = PQexec(conn, sql.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "查询借阅记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return records;
    }

    const int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        records.push_back(buildBorrowRecordFromRow(res, i));
    }
    PQclear(res);
    return records;
}

std::vector<BorrowRecord> BorrowDao::getActiveRecords() {
    std::vector<BorrowRecord> records;
    PGconn* conn = db.getConnection();
    if (!conn) {
        return records;
    }

    refreshOverdueRecords();

    std::string sql = std::string(kBorrowRecordSelect) +
        "WHERE br.return_time IS NULL ORDER BY br.record_id DESC";
    PGresult* res = PQexec(conn, sql.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "查询未归还记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return records;
    }

    const int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        records.push_back(buildBorrowRecordFromRow(res, i));
    }
    PQclear(res);
    return records;
}

std::vector<BorrowRecord> BorrowDao::getOverdueRecords() {
    std::vector<BorrowRecord> records;
    PGconn* conn = db.getConnection();
    if (!conn) {
        return records;
    }

    refreshOverdueRecords();

    std::string sql = std::string(kBorrowRecordSelect) +
        "WHERE br.return_time IS NULL AND br.record_status = '已逾期' ORDER BY br.record_id DESC";
    PGresult* res = PQexec(conn, sql.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "查询逾期记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return records;
    }

    const int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        records.push_back(buildBorrowRecordFromRow(res, i));
    }
    PQclear(res);
    return records;
}

std::vector<BorrowRecord> BorrowDao::getRecordsByUser(long long userId) {
    std::vector<BorrowRecord> records;
    PGconn* conn = db.getConnection();
    if (!conn) {
        return records;
    }

    refreshOverdueRecords();

    const std::string userIdStr = std::to_string(userId);
    const char* params[1] = { userIdStr.c_str() };
    std::string sql = std::string(kBorrowRecordSelect) +
        "WHERE br.user_id = $1 ORDER BY br.record_id DESC";
    PGresult* res = PQexecParams(conn, sql.c_str(), 1, nullptr, params, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "按用户查询借阅记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return records;
    }

    const int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        records.push_back(buildBorrowRecordFromRow(res, i));
    }
    PQclear(res);
    return records;
}

std::optional<BorrowRecord> BorrowDao::getRecordById(long long recordId) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return std::nullopt;
    }

    refreshOverdueRecords();

    const std::string recordIdStr = std::to_string(recordId);
    const char* params[1] = { recordIdStr.c_str() };
    std::string sql = std::string(kBorrowRecordSelect) +
        "WHERE br.record_id = $1";
    PGresult* res = PQexecParams(conn, sql.c_str(), 1, nullptr, params, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "按 ID 查询借阅记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return std::nullopt;
    }
    if (PQntuples(res) == 0) {
        PQclear(res);
        return std::nullopt;
    }

    BorrowRecord record = buildBorrowRecordFromRow(res, 0);
    PQclear(res);
    return record;
}

std::vector<ReservationRecord> BorrowDao::getAllReservations() {
    std::vector<ReservationRecord> records;
    PGconn* conn = db.getConnection();
    if (!conn) {
        return records;
    }

    std::string sql = std::string(kReservationRecordSelect) + "ORDER BY rr.reservation_id DESC";
    PGresult* res = PQexec(conn, sql.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "查询预约记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return records;
    }

    const int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        records.push_back(buildReservationRecordFromRow(res, i));
    }
    PQclear(res);
    return records;
}

std::vector<ReservationRecord> BorrowDao::getReservationsByUser(long long userId) {
    std::vector<ReservationRecord> records;
    PGconn* conn = db.getConnection();
    if (!conn) {
        return records;
    }

    const std::string userIdStr = std::to_string(userId);
    const char* params[1] = { userIdStr.c_str() };
    std::string sql = std::string(kReservationRecordSelect) +
        "WHERE rr.user_id = $1 ORDER BY rr.reservation_id DESC";
    PGresult* res = PQexecParams(conn, sql.c_str(), 1, nullptr, params, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "按用户查询预约记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return records;
    }

    const int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        records.push_back(buildReservationRecordFromRow(res, i));
    }
    PQclear(res);
    return records;
}

std::optional<ReservationRecord> BorrowDao::getReservationById(long long reservationId) {
    PGconn* conn = db.getConnection();
    if (!conn) {
        return std::nullopt;
    }

    const std::string reservationIdStr = std::to_string(reservationId);
    const char* params[1] = { reservationIdStr.c_str() };
    std::string sql = std::string(kReservationRecordSelect) +
        "WHERE rr.reservation_id = $1";
    PGresult* res = PQexecParams(conn, sql.c_str(), 1, nullptr, params, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "按 ID 查询预约记录失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return std::nullopt;
    }
    if (PQntuples(res) == 0) {
        PQclear(res);
        return std::nullopt;
    }

    ReservationRecord record = buildReservationRecordFromRow(res, 0);
    PQclear(res);
    return record;
}

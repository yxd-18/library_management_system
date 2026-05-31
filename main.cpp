#include <clocale>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <windows.h>

#include "DBUtil.h"
#include "MenuHandlers.h"
#include "UserDao.h"

namespace {
struct DBConfig {
    std::string host = "localhost";
    std::string port = "5432";
    std::string dbname = "library_management_system";
    std::string user = "postgres";
    std::string password;
};

std::string trim(const std::string& value) {
    const std::string whitespace = " \t\r\n";
    const size_t start = value.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }

    const size_t end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

bool loadConfigFile(const std::string& path, DBConfig& config) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::unordered_map<std::string, std::string> values;
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        values[trim(line.substr(0, pos))] = trim(line.substr(pos + 1));
    }

    if (values.count("host")) {
        config.host = values["host"];
    }
    if (values.count("port")) {
        config.port = values["port"];
    }
    if (values.count("dbname")) {
        config.dbname = values["dbname"];
    }
    if (values.count("user")) {
        config.user = values["user"];
    }
    if (values.count("password")) {
        config.password = values["password"];
    }

    return true;
}

bool executeSchemaUpgrade(PGconn* conn, const char* sql, const char* label) {
    PGresult* res = PQexec(conn, sql);
    const auto status = PQresultStatus(res);
    const bool ok = status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK;
    if (!ok) {
        std::cerr << label << "失败: " << PQerrorMessage(conn) << std::endl;
    }
    PQclear(res);
    return ok;
}

bool applyLibraryEnhancements(PGconn* conn) {
    return executeSchemaUpgrade(
               conn,
               "ALTER TABLE books "
               "ADD COLUMN IF NOT EXISTS lost_count INT NOT NULL DEFAULT 0",
               "扩展 books 表"
           ) &&
        executeSchemaUpgrade(
               conn,
               "ALTER TABLE borrow_records "
               "ADD COLUMN IF NOT EXISTS fine_amount NUMERIC(10, 2) NOT NULL DEFAULT 0.00",
               "扩展 borrow_records 表"
           ) &&
        executeSchemaUpgrade(
               conn,
               "DO $$ "
               "BEGIN "
               "    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'borrow_status_enum') THEN "
               "        CREATE TYPE borrow_status_enum AS ENUM ('借阅中', '已归还', '已逾期', '已挂失'); "
               "    ELSIF NOT EXISTS ("
               "           SELECT 1 "
               "           FROM pg_enum e "
               "           JOIN pg_type t ON e.enumtypid = t.oid "
               "           WHERE t.typname = 'borrow_status_enum' AND e.enumlabel = '已挂失'"
               "       ) THEN "
               "        ALTER TYPE borrow_status_enum ADD VALUE '已挂失'; "
               "    END IF; "
               "END $$",
               "扩展借阅状态枚举"
           ) &&
        executeSchemaUpgrade(
               conn,
               "DO $$ "
               "BEGIN "
               "    IF EXISTS ("
               "           SELECT 1 "
               "           FROM information_schema.columns "
               "           WHERE table_schema = 'public' "
               "             AND table_name = 'borrow_records' "
               "             AND column_name = 'record_status' "
               "             AND udt_name <> 'borrow_status_enum'"
               "       ) THEN "
               "        ALTER TABLE borrow_records "
               "        ALTER COLUMN record_status DROP DEFAULT, "
               "        ALTER COLUMN record_status TYPE borrow_status_enum USING record_status::text::borrow_status_enum, "
               "        ALTER COLUMN record_status SET DEFAULT '借阅中'; "
               "    END IF; "
               "END $$",
               "修正借阅状态字段类型"
           ) &&
        executeSchemaUpgrade(
               conn,
               "DO $$ "
               "BEGIN "
               "    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'reservation_status_enum') THEN "
               "        CREATE TYPE reservation_status_enum AS ENUM ('排队中', '可取书', '已完成', '已取消'); "
               "    END IF; "
               "END $$",
               "创建预约状态枚举"
           ) &&
        executeSchemaUpgrade(
               conn,
               "CREATE TABLE IF NOT EXISTS reservation_records ("
               "    reservation_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY, "
               "    user_id BIGINT NOT NULL REFERENCES users(user_id), "
               "    book_id BIGINT NOT NULL REFERENCES books(book_id), "
               "    reservation_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
               "    ready_time TIMESTAMP, "
               "    pickup_deadline TIMESTAMP, "
               "    fulfill_time TIMESTAMP, "
               "    cancel_time TIMESTAMP, "
               "    reservation_status reservation_status_enum NOT NULL DEFAULT '排队中', "
               "    operator_id BIGINT REFERENCES users(user_id), "
               "    remark VARCHAR(255)"
               ")",
               "创建预约记录表"
           ) &&
        executeSchemaUpgrade(
               conn,
               "CREATE INDEX IF NOT EXISTS idx_reservation_user_id ON reservation_records(user_id)",
               "创建预约用户索引"
           ) &&
        executeSchemaUpgrade(
               conn,
               "CREATE INDEX IF NOT EXISTS idx_reservation_book_status "
               "ON reservation_records(book_id, reservation_status)",
               "创建预约图书状态索引"
           ) &&
        executeSchemaUpgrade(
               conn,
               "UPDATE borrow_records "
               "SET fine_amount = ROUND((CASE "
               "    WHEN return_time IS NULL AND due_time < CURRENT_TIMESTAMP THEN GREATEST(0, CURRENT_DATE - due_time::date) * 0.50 "
               "    WHEN return_time IS NOT NULL THEN GREATEST(0, return_time::date - due_time::date) * 0.50 "
               "    ELSE 0 "
               "END)::numeric, 2) "
               "WHERE COALESCE(fine_amount, 0) = 0",
               "回填历史罚金"
           );
}

void initializeConsoleUtf8() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::setlocale(LC_ALL, ".UTF-8");
}
}

int main() {
    initializeConsoleUtf8();

    DBUtil db;
    DBConfig config;
    char* envPassword = nullptr;
    size_t envPasswordLength = 0;

    loadConfigFile("db_config.txt", config);

    if (config.password.empty() &&
        _dupenv_s(&envPassword, &envPasswordLength, "PGPASSWORD") == 0 &&
        envPassword != nullptr) {
        config.password = envPassword;
        free(envPassword);
    }

    if (config.password.empty()) {
        std::cout << "请输入 PostgreSQL 数据库密码: ";
        std::getline(std::cin, config.password);
    }

    if (!db.connect(config.host, config.port, config.dbname, config.user, config.password)) {
        return 1;
    }

    if (!applyLibraryEnhancements(db.getConnection())) {
        db.disconnect();
        return 1;
    }

    UserDao userDao(db.getConnection());
    OperationLogDao logDao(db);
    BookDao bookDao(db, logDao);
    CategoryDao categoryDao(db, logDao);
    BorrowDao borrowDao(db, logDao);
    BackupDao backupDao(db, logDao);
    UserSettingsDao settingsDao(db, logDao);

    borrowDao.refreshOverdueRecords();

    long long currentOperatorId = 0;
    runConsoleMenu(userDao, bookDao, categoryDao, borrowDao, logDao, backupDao, settingsDao, currentOperatorId);

    db.disconnect();
    return 0;
}

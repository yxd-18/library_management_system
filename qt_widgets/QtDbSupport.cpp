#include "QtDbSupport.h"

#include <cstdlib>
#include <fstream>
#include <libpq-fe.h>
#include <unordered_map>

std::string trimQtDbText(const std::string& value) {
    const std::string whitespace = " \t\r\n";
    const size_t start = value.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }

    const size_t end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

bool loadQtDbConfig(const std::vector<std::string>& candidatePaths, QtDBConfig& config) {
    for (const auto& path : candidatePaths) {
        std::ifstream file(path);
        if (!file.is_open()) {
            continue;
        }

        std::unordered_map<std::string, std::string> values;
        std::string line;
        while (std::getline(file, line)) {
            line = trimQtDbText(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }

            const size_t pos = line.find('=');
            if (pos == std::string::npos) {
                continue;
            }

            values[trimQtDbText(line.substr(0, pos))] = trimQtDbText(line.substr(pos + 1));
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

    return false;
}

namespace {
bool execUpgrade(PGconn* conn, const char* sql, std::string& errorMessage) {
    PGresult* res = PQexec(conn, sql);
    const auto status = PQresultStatus(res);
    const bool ok = status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK;
    if (!ok) {
        errorMessage = PQerrorMessage(conn);
    }
    PQclear(res);
    return ok;
}
}

bool applyQtLibraryEnhancements(PGconn* conn, std::string& errorMessage) {
    errorMessage.clear();

    return execUpgrade(
               conn,
               "ALTER TABLE books "
               "ADD COLUMN IF NOT EXISTS lost_count INT NOT NULL DEFAULT 0",
               errorMessage
           ) &&
        execUpgrade(
               conn,
               "ALTER TABLE borrow_records "
               "ADD COLUMN IF NOT EXISTS fine_amount NUMERIC(10, 2) NOT NULL DEFAULT 0.00",
               errorMessage
           ) &&
        execUpgrade(
               conn,
               "DO $$ "
               "BEGIN "
               "    IF EXISTS (SELECT 1 FROM pg_type WHERE typname = 'borrow_status_enum') "
               "       AND NOT EXISTS ("
               "           SELECT 1 "
               "           FROM pg_enum e "
               "           JOIN pg_type t ON e.enumtypid = t.oid "
               "           WHERE t.typname = 'borrow_status_enum' AND e.enumlabel = '已挂失'"
               "       ) THEN "
               "        ALTER TYPE borrow_status_enum ADD VALUE '已挂失'; "
               "    END IF; "
               "END $$",
               errorMessage
           ) &&
        execUpgrade(
               conn,
               "DO $$ "
               "BEGIN "
               "    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'reservation_status_enum') THEN "
               "        CREATE TYPE reservation_status_enum AS ENUM ('排队中', '可取书', '已完成', '已取消'); "
               "    END IF; "
               "END $$",
               errorMessage
           ) &&
        execUpgrade(
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
               errorMessage
           ) &&
        execUpgrade(
               conn,
               "CREATE INDEX IF NOT EXISTS idx_reservation_user_id ON reservation_records(user_id)",
               errorMessage
           ) &&
        execUpgrade(
               conn,
               "CREATE INDEX IF NOT EXISTS idx_reservation_book_status "
               "ON reservation_records(book_id, reservation_status)",
               errorMessage
           ) &&
        execUpgrade(
               conn,
               "UPDATE borrow_records "
               "SET fine_amount = ROUND((CASE "
               "    WHEN return_time IS NULL AND due_time < CURRENT_TIMESTAMP THEN GREATEST(0, CURRENT_DATE - due_time::date) * 0.50 "
               "    WHEN return_time IS NOT NULL THEN GREATEST(0, return_time::date - due_time::date) * 0.50 "
               "    ELSE 0 "
               "END)::numeric, 2) "
               "WHERE COALESCE(fine_amount, 0) = 0",
               errorMessage
           );
}

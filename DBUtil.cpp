#include "DBUtil.h"
#include <iostream>

DBUtil::DBUtil() : conn(nullptr) {}

DBUtil::~DBUtil() {
    disconnect();
}

bool DBUtil::connect(const std::string& host,
    const std::string& port,
    const std::string& dbname,
    const std::string& user,
    const std::string& password) {
    std::string connInfo =
        "host=" + host +
        " port=" + port +
        " dbname=" + dbname +
        " user=" + user +
        " password=" + password;

    conn = PQconnectdb(connInfo.c_str());

    if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "数据库连接失败: " << PQerrorMessage(conn) << std::endl;
        return false;
    }

    std::cout << "数据库连接成功！" << std::endl;
    return true;
}

void DBUtil::disconnect() {
    if (conn != nullptr) {
        PQfinish(conn);
        conn = nullptr;
    }
}

bool DBUtil::isConnected() const {
    return conn != nullptr && PQstatus(conn) == CONNECTION_OK;
}

PGconn* DBUtil::getConnection() const {
    return conn;
}

std::string DBUtil::getLastError() const {
    if (conn != nullptr) {
        return PQerrorMessage(conn);
    }
    return "数据库未连接";
}

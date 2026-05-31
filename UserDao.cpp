#include "UserDao.h"
#include <iostream>

UserDao::UserDao(PGconn* connection) : conn(connection) {}

std::string UserDao::simpleHash(const std::string& password) {
    // 当前先做演示，直接返回原值
    // 后面你们可以替换成真正的哈希
    return password;
}

std::optional<User> UserDao::login(const std::string& username, const std::string& password) {
    const char* sql =
        "SELECT u.user_id, u.username, u.password_hash, u.real_name, "
        "COALESCE(u.student_no, ''), COALESCE(u.phone, ''), COALESCE(u.email, ''), "
        "u.gender::text, u.status::text, u.role_id, r.role_name "
        "FROM users u "
        "JOIN roles r ON u.role_id = r.role_id "
        "WHERE u.username = $1 AND u.status = '正常'";

    const char* params[1] = { username.c_str() };

    PGresult* res = PQexecParams(conn, sql, 1, nullptr, params, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "登录查询失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return std::nullopt;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        return std::nullopt;
    }

    std::string dbPassword = PQgetvalue(res, 0, 2);
    if (dbPassword != simpleHash(password)) {
        PQclear(res);
        return std::nullopt;
    }

    User user;
    user.userId = std::stoll(PQgetvalue(res, 0, 0));
    user.username = PQgetvalue(res, 0, 1);
    user.passwordHash = PQgetvalue(res, 0, 2);
    user.realName = PQgetvalue(res, 0, 3);
    user.studentNo = PQgetvalue(res, 0, 4);
    user.phone = PQgetvalue(res, 0, 5);
    user.email = PQgetvalue(res, 0, 6);
    user.gender = PQgetvalue(res, 0, 7);
    user.status = PQgetvalue(res, 0, 8);
    user.roleId = std::stoi(PQgetvalue(res, 0, 9));
    user.roleName = PQgetvalue(res, 0, 10);
    PQclear(res);

    const char* updateSql =
        "UPDATE users SET last_login_time = CURRENT_TIMESTAMP WHERE user_id = $1";
    std::string idStr = std::to_string(user.userId);
    const char* updateParams[1] = { idStr.c_str() };
    PGresult* updateRes = PQexecParams(conn, updateSql, 1, nullptr, updateParams, nullptr, nullptr, 0);
    PQclear(updateRes);

    return user;
}

bool UserDao::registerReader(const User& user) {
    if (user.username.empty() || user.passwordHash.empty() || user.realName.empty() || user.studentNo.empty()) {
        std::cerr << "用户名、密码、姓名、学号不能为空！" << std::endl;
        return false;
    }

    const char* checkSql =
        "SELECT 1 FROM users WHERE username = $1 OR student_no = $2";
    const char* checkParams[2] = { user.username.c_str(), user.studentNo.c_str() };

    PGresult* checkRes = PQexecParams(conn, checkSql, 2, nullptr, checkParams, nullptr, nullptr, 0);

    if (PQresultStatus(checkRes) != PGRES_TUPLES_OK) {
        std::cerr << "注册前校验失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(checkRes);
        return false;
    }

    if (PQntuples(checkRes) > 0) {
        std::cerr << "用户名或学号已存在！" << std::endl;
        PQclear(checkRes);
        return false;
    }
    PQclear(checkRes);

    std::string hashedPassword = simpleHash(user.passwordHash);

    const char* insertSql =
        "INSERT INTO users "
        "(username, password_hash, real_name, student_no, phone, email, gender, status, role_id, remark) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7::gender_enum, '正常', 2, $8)";

    const char* params[8] = {
        user.username.c_str(),
        hashedPassword.c_str(),
        user.realName.c_str(),
        user.studentNo.c_str(),
        user.phone.c_str(),
        user.email.c_str(),
        user.gender.empty() ? "其他" : user.gender.c_str(),
        user.remark.c_str()
    };

    PGresult* res = PQexecParams(conn, insertSql, 8, nullptr, params, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "注册失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

std::vector<User> UserDao::getAllUsers() {
    std::vector<User> users;

    const char* sql =
        "SELECT u.user_id, u.username, u.real_name, COALESCE(u.student_no, ''), "
        "COALESCE(u.phone, ''), COALESCE(u.email, ''), u.gender::text, "
        "u.status::text, u.role_id, r.role_name, COALESCE(u.remark, '') "
        "FROM users u "
        "JOIN roles r ON u.role_id = r.role_id "
        "ORDER BY u.user_id";

    PGresult* res = PQexec(conn, sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "查询所有用户失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return users;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        User user;
        user.userId = std::stoll(PQgetvalue(res, i, 0));
        user.username = PQgetvalue(res, i, 1);
        user.realName = PQgetvalue(res, i, 2);
        user.studentNo = PQgetvalue(res, i, 3);
        user.phone = PQgetvalue(res, i, 4);
        user.email = PQgetvalue(res, i, 5);
        user.gender = PQgetvalue(res, i, 6);
        user.status = PQgetvalue(res, i, 7);
        user.roleId = std::stoi(PQgetvalue(res, i, 8));
        user.roleName = PQgetvalue(res, i, 9);
        user.remark = PQgetvalue(res, i, 10);
        users.push_back(user);
    }

    PQclear(res);
    return users;
}

std::optional<User> UserDao::getUserByUsername(const std::string& username) {
    const char* sql =
        "SELECT u.user_id, u.username, u.password_hash, u.real_name, "
        "COALESCE(u.student_no, ''), COALESCE(u.phone, ''), COALESCE(u.email, ''), "
        "u.gender::text, u.status::text, u.role_id, r.role_name, COALESCE(u.remark, '') "
        "FROM users u "
        "JOIN roles r ON u.role_id = r.role_id "
        "WHERE u.username = $1";

    const char* params[1] = { username.c_str() };

    PGresult* res = PQexecParams(conn, sql, 1, nullptr, params, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "按用户名查询失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return std::nullopt;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        return std::nullopt;
    }

    User user;
    user.userId = std::stoll(PQgetvalue(res, 0, 0));
    user.username = PQgetvalue(res, 0, 1);
    user.passwordHash = PQgetvalue(res, 0, 2);
    user.realName = PQgetvalue(res, 0, 3);
    user.studentNo = PQgetvalue(res, 0, 4);
    user.phone = PQgetvalue(res, 0, 5);
    user.email = PQgetvalue(res, 0, 6);
    user.gender = PQgetvalue(res, 0, 7);
    user.status = PQgetvalue(res, 0, 8);
    user.roleId = std::stoi(PQgetvalue(res, 0, 9));
    user.roleName = PQgetvalue(res, 0, 10);
    user.remark = PQgetvalue(res, 0, 11);

    PQclear(res);
    return user;
}

bool UserDao::updateUser(const User& user) {
    const char* sql =
        "UPDATE users "
        "SET real_name = $1, student_no = $2, phone = $3, email = $4, "
        "gender = $5::gender_enum, status = $6::user_status_enum, remark = $7 "
        "WHERE user_id = $8";

    std::string idStr = std::to_string(user.userId);

    const char* params[8] = {
        user.realName.c_str(),
        user.studentNo.c_str(),
        user.phone.c_str(),
        user.email.c_str(),
        user.gender.c_str(),
        user.status.c_str(),
        user.remark.c_str(),
        idStr.c_str()
    };

    PGresult* res = PQexecParams(conn, sql, 8, nullptr, params, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "更新用户失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

bool UserDao::deleteUser(long long userId) {
    const char* checkSql =
        "SELECT 1 FROM borrow_records WHERE user_id = $1 AND return_time IS NULL";

    std::string idStr = std::to_string(userId);
    const char* checkParams[1] = { idStr.c_str() };

    PGresult* checkRes = PQexecParams(conn, checkSql, 1, nullptr, checkParams, nullptr, nullptr, 0);

    if (PQresultStatus(checkRes) != PGRES_TUPLES_OK) {
        std::cerr << "删除前检查失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(checkRes);
        return false;
    }

    if (PQntuples(checkRes) > 0) {
        std::cerr << "该用户存在未归还图书，不能删除！" << std::endl;
        PQclear(checkRes);
        return false;
    }
    PQclear(checkRes);

    const char* sql = "DELETE FROM users WHERE user_id = $1";
    const char* params[1] = { idStr.c_str() };

    PGresult* res = PQexecParams(conn, sql, 1, nullptr, params, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "删除用户失败: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

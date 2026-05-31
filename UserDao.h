#ifndef USERDAO_H
#define USERDAO_H

#include "User.h"
#include <libpq-fe.h>
#include <optional>
#include <string>
#include <vector>

class UserDao {
private:
    PGconn* conn;

public:
    explicit UserDao(PGconn* connection);

    std::optional<User> login(const std::string& username, const std::string& password);
    bool registerReader(const User& user);
    std::vector<User> getAllUsers();
    std::optional<User> getUserByUsername(const std::string& username);
    bool updateUser(const User& user);
    bool deleteUser(long long userId);

private:
    std::string simpleHash(const std::string& password);
};

#endif


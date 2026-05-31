#ifndef USERMENUSERVICE_H
#define USERMENUSERVICE_H

#include <optional>
#include <string>
#include <vector>
#include "ServiceResult.h"
#include "User.h"
#include "UserDao.h"

DataResult<std::optional<User>> loginUser(UserDao& userDao, const std::string& username, const std::string& password);
ActionResult registerReaderUser(UserDao& userDao, const User& user);
DataResult<std::vector<User>> listUsers(UserDao& userDao);
DataResult<std::optional<User>> findUserByUsername(UserDao& userDao, const std::string& username);
ActionResult updateExistingUser(UserDao& userDao, const User& user);
ActionResult removeUser(UserDao& userDao, long long userId);

#endif

#ifndef USER_H
#define USER_H

#include <string>

struct User {
    long long userId = 0;
    std::string username;
    std::string passwordHash;
    std::string realName;
    std::string studentNo;
    std::string phone;
    std::string email;
    std::string gender;
    std::string status;
    int roleId = 0;
    std::string roleName;
    std::string registerTime;
    std::string lastLoginTime;
    std::string remark;
};

#endif


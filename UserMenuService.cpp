#include "UserMenuService.h"

DataResult<std::optional<User>> loginUser(UserDao& userDao, const std::string& username, const std::string& password) {
    DataResult<std::optional<User>> result;
    result.data = userDao.login(username, password);
    result.success = result.data.has_value();
    result.message = result.success ? "登录成功！" : "登录失败，用户名或密码错误！";
    return result;
}

ActionResult registerReaderUser(UserDao& userDao, const User& user) {
    ActionResult result;
    result.success = userDao.registerReader(user);
    result.message = result.success ? "读者注册成功！" : "读者注册失败！";
    return result;
}

DataResult<std::vector<User>> listUsers(UserDao& userDao) {
    DataResult<std::vector<User>> result;
    result.success = true;
    result.data = userDao.getAllUsers();
    result.message = result.data.empty() ? "当前暂无用户数据。" : "查询用户列表成功。";
    return result;
}

DataResult<std::optional<User>> findUserByUsername(UserDao& userDao, const std::string& username) {
    DataResult<std::optional<User>> result;
    result.data = userDao.getUserByUsername(username);
    result.success = result.data.has_value();
    result.message = result.success ? "查询用户成功。" : "未找到该用户！";
    return result;
}

ActionResult updateExistingUser(UserDao& userDao, const User& user) {
    ActionResult result;
    result.success = userDao.updateUser(user);
    result.message = result.success ? "用户信息更新成功！" : "用户信息更新失败！";
    return result;
}

ActionResult removeUser(UserDao& userDao, long long userId) {
    ActionResult result;
    result.success = userDao.deleteUser(userId);
    result.message = result.success ? "删除用户成功！" : "删除用户失败！";
    return result;
}

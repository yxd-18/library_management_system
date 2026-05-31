#ifndef USERSETTINGS_H
#define USERSETTINGS_H

#include <string>

struct UserSettings {
    long long settingId = 0;
    long long userId = 0;
    std::string username;
    std::string fontSize = "medium";
    std::string theme = "light";
    std::string languagePref = "zh-CN";
    int pageSize = 10;
    bool enableNotification = true;
    std::string updateTime;
};

#endif

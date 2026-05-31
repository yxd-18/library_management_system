#ifndef DBUTIL_H
#define DBUTIL_H

#include <string>
#include <libpq-fe.h>

class DBUtil {
private:
    PGconn* conn;

public:
    DBUtil();
    ~DBUtil();

    bool connect(const std::string& host,
        const std::string& port,
        const std::string& dbname,
        const std::string& user,
        const std::string& password);

    void disconnect();
    bool isConnected() const;
    PGconn* getConnection() const;
    std::string getLastError() const;
};

#endif


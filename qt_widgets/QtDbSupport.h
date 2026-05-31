#pragma once

#include <libpq-fe.h>
#include <string>
#include <vector>

struct QtDBConfig {
    std::string host = "localhost";
    std::string port = "5432";
    std::string dbname = "library_management_system";
    std::string user = "postgres";
    std::string password;
};

std::string trimQtDbText(const std::string& value);
bool loadQtDbConfig(const std::vector<std::string>& candidatePaths, QtDBConfig& config);
bool applyQtLibraryEnhancements(PGconn* conn, std::string& errorMessage);

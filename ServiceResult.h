#ifndef SERVICERESULT_H
#define SERVICERESULT_H

#include <string>

struct ActionResult {
    bool success = false;
    std::string message;
};

template <typename T>
struct DataResult {
    bool success = false;
    std::string message;
    T data{};
};

#endif

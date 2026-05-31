#ifndef BOOKCATEGORY_H
#define BOOKCATEGORY_H

#include <string>

struct BookCategory {
    int categoryId = 0;
    std::string categoryName;
    int parentId = 0;              // 0 表示无父分类
    std::string parentName;
    std::string categoryDesc;
    std::string createTime;
};

#endif


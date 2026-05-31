#ifndef BOOKMENU_H
#define BOOKMENU_H

#include "BookDao.h"
#include "CategoryDao.h"

void runBookMenu(BookDao& bookDao, CategoryDao& categoryDao, long long currentOperatorId);

#endif

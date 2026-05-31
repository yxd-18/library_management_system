#ifndef MENUHANDLERS_H
#define MENUHANDLERS_H

#include "BackupDao.h"
#include "BookDao.h"
#include "BorrowDao.h"
#include "CategoryDao.h"
#include "OperationLogDao.h"
#include "UserDao.h"
#include "UserSettingsDao.h"

void runConsoleMenu(UserDao& userDao,
    BookDao& bookDao,
    CategoryDao& categoryDao,
    BorrowDao& borrowDao,
    OperationLogDao& logDao,
    BackupDao& backupDao,
    UserSettingsDao& settingsDao,
    long long& currentOperatorId);

#endif

#ifndef SYSTEMMENU_H
#define SYSTEMMENU_H

#include "BackupDao.h"
#include "OperationLogDao.h"
#include "UserSettingsDao.h"

void runSystemMenu(OperationLogDao& logDao,
    BackupDao& backupDao,
    UserSettingsDao& settingsDao,
    long long currentOperatorId);

#endif

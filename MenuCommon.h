#ifndef MENUCOMMON_H
#define MENUCOMMON_H

#include <string>
#include "BackupRecord.h"
#include "Book.h"
#include "BookCategory.h"
#include "BorrowRecord.h"
#include "ReservationRecord.h"
#include "UserSettings.h"

std::string trimMenuText(const std::string& value);
std::string promptLine(const std::string& label);
int parseInt(const std::string& text, int defaultValue = 0);
long long parseLongLong(const std::string& text, long long defaultValue = 0);
double parseDouble(const std::string& text, double defaultValue = 0.0);
int promptInt(const std::string& label, int defaultValue = 0);
long long promptLongLong(const std::string& label, long long defaultValue = 0);
double promptDouble(const std::string& label, double defaultValue = 0.0);

Book inputBook(bool includeId);
BookCategory inputCategory(bool includeId);

void printBook(const Book& book);
void printCategory(const BookCategory& category);
void printBorrowRecord(const BorrowRecord& record);
void printReservationRecord(const ReservationRecord& record);
void printBackupRecord(const BackupRecord& record);
void printSettings(const UserSettings& settings);

#endif

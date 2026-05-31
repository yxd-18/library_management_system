#pragma once

#include <optional>

#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTabWidget>
#include <QString>
#include <QTimer>

#include "LibraryRepository.h"

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void buildUi();
    QWidget* createLoginPage();
    QWidget* createAdminPage();
    QWidget* createReaderPage();

    void applyTheme();
    void initializeRepository();
    void handleLogin();
    void logout();
    void showAdminDashboard();
    void showReaderDashboard();
    void refreshAdminDashboard();
    void refreshReaderDashboard();

    void switchAdminSubPage(int index);
    void updateCurrentAdminTableView();
    void onAdminSearchChanged(const QString& text);
    void onAdminPageChanged(int delta);

    void updateCurrentReaderTableView();
    void onReaderSearchChanged(const QString& text);
    void onReaderPageChanged(int delta);
    void showTableDetails(QTableWidget* table, int row);
    bool confirmAction(const QString& title, const QString& text);

    void setLoginStatus(const QString& message, bool isError);
    QWidget* createMetricCard(const QString& title, QLabel*& valueLabel);

    static bool isAdmin(const User& user);
    static QString toText(const std::string& value);
    static QString moneyText(double value);

    LibraryRepository repository_;
    std::optional<User> currentUser_;

    QStackedWidget* pages_ = nullptr;

    QStackedWidget* adminContentStack_ = nullptr;
    QLineEdit* adminSearchEdit_ = nullptr;
    QLabel* adminPageLabel_ = nullptr;
    QPushButton* adminPrevPageButton_ = nullptr;
    QPushButton* adminNextPageButton_ = nullptr;
    int adminPageIndex_[4] = {0, 0, 0, 0};
    QString adminSearchText_[4];

    QLineEdit* readerSearchEdit_ = nullptr;
    QLabel* readerPageLabel_ = nullptr;
    QPushButton* readerPrevPageButton_ = nullptr;
    QPushButton* readerNextPageButton_ = nullptr;
    int readerPageIndex_[3] = {0, 0, 0};
    QString readerSearchText_[3];
    QTabWidget* readerTabs_ = nullptr;

    QLineEdit* usernameEdit_ = nullptr;
    QLineEdit* passwordEdit_ = nullptr;
    QLabel* loginStatusLabel_ = nullptr;

    QLabel* adminGreetingLabel_ = nullptr;
    QLabel* adminUsersMetric_ = nullptr;
    QLabel* adminBooksMetric_ = nullptr;
    QLabel* adminActiveBorrowMetric_ = nullptr;
    QLabel* adminOverdueMetric_ = nullptr;
    QLabel* adminReservationMetric_ = nullptr;
    QTableWidget* adminUsersTable_ = nullptr;
    QTableWidget* adminBooksTable_ = nullptr;
    QTableWidget* adminBorrowsTable_ = nullptr;
    QTableWidget* adminReservationsTable_ = nullptr;

    QLabel* readerGreetingLabel_ = nullptr;
    QLabel* readerBorrowMetric_ = nullptr;
    QLabel* readerFineMetric_ = nullptr;
    QLabel* readerReservationMetric_ = nullptr;
    QTableWidget* readerBorrowTable_ = nullptr;
    QTableWidget* readerReservationTable_ = nullptr;
    QTableWidget* readerCatalogTable_ = nullptr;

    QTimer* adminRefreshTimer_ = nullptr;
};

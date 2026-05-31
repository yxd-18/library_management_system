#include "MainWindow.h"

#include <algorithm>
#include <map>

#include <QApplication>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStringList>
#include <QSplitter>
#include <QTabWidget>
#include <QButtonGroup>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), adminRefreshTimer_(nullptr) {
    buildUi();
    applyTheme();
    initializeRepository();
}

void MainWindow::buildUi() {
    setWindowTitle("图书管理系统 · Qt Widgets");
    resize(1460, 920);

    pages_ = new QStackedWidget(this);
    pages_->addWidget(createLoginPage());
    pages_->addWidget(createAdminPage());
    pages_->addWidget(createReaderPage());
    setCentralWidget(pages_);
}

QWidget* MainWindow::createLoginPage() {
    auto* page = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(page);
    rootLayout->setContentsMargins(36, 36, 36, 36);
    rootLayout->setSpacing(28);

    auto* heroFrame = new QFrame(page);
    heroFrame->setObjectName("heroPanel");
    auto* heroLayout = new QVBoxLayout(heroFrame);
    heroLayout->setContentsMargins(32, 32, 32, 32);
    heroLayout->setSpacing(18);

    auto* titleLabel = new QLabel("Library Console to Desktop", heroFrame);
    titleLabel->setObjectName("heroTitle");
    auto* subtitleLabel = new QLabel("这一版先把登录、管理员工作台和读者工作台搭起来，后面再逐步把借阅、图书维护和统计页面接完整。", heroFrame);
    subtitleLabel->setWordWrap(true);
    subtitleLabel->setObjectName("heroSubtitle");

    auto* pointsLabel = new QLabel(
        "第一阶段界面目标\n"
        "• 统一登录入口\n"
        "• 管理员与读者分角色工作台\n"
        "• 图书目录、借阅、预约的桌面化展示",
        heroFrame
    );
    pointsLabel->setObjectName("heroPoints");

    heroLayout->addWidget(titleLabel);
    heroLayout->addWidget(subtitleLabel);
    heroLayout->addStretch();
    heroLayout->addWidget(pointsLabel);

    auto* formFrame = new QFrame(page);
    formFrame->setObjectName("loginPanel");
    auto* formLayout = new QVBoxLayout(formFrame);
    formLayout->setContentsMargins(28, 28, 28, 28);
    formLayout->setSpacing(18);

    auto* formTitle = new QLabel("登录系统", formFrame);
    formTitle->setObjectName("panelTitle");

    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFormAlignment(Qt::AlignTop);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(16);

    usernameEdit_ = new QLineEdit(formFrame);
    usernameEdit_->setPlaceholderText("例如：admin01 或 reader01");
    passwordEdit_ = new QLineEdit(formFrame);
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setPlaceholderText("请输入密码");

    form->addRow("用户名", usernameEdit_);
    form->addRow("密码", passwordEdit_);

    auto* loginButton = new QPushButton("进入系统", formFrame);
    auto* sampleLabel = new QLabel("示例账号：admin01 / admin123，reader01 / reader123", formFrame);
    sampleLabel->setObjectName("hintLabel");
    loginStatusLabel_ = new QLabel(formFrame);
    loginStatusLabel_->setWordWrap(true);
    loginStatusLabel_->hide();

    connect(loginButton, &QPushButton::clicked, this, [this]() { handleLogin(); });
    connect(passwordEdit_, &QLineEdit::returnPressed, this, [this]() { handleLogin(); });

    formLayout->addWidget(formTitle);
    formLayout->addLayout(form);
    formLayout->addWidget(loginButton);
    formLayout->addWidget(sampleLabel);
    formLayout->addWidget(loginStatusLabel_);
    formLayout->addStretch();

    rootLayout->addWidget(heroFrame, 3);
    rootLayout->addWidget(formFrame, 2);
    return page;
}

QWidget* MainWindow::createAdminPage() {
    auto* page = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(page);
    rootLayout->setContentsMargins(24, 24, 24, 24);
    rootLayout->setSpacing(20);

    auto* navFrame = new QFrame(page);
    navFrame->setObjectName("navPanel");
    navFrame->setFixedWidth(220);
    auto* navLayout = new QVBoxLayout(navFrame);
    navLayout->setContentsMargins(16, 16, 16, 16);
    navLayout->setSpacing(14);
    auto* navTitle = new QLabel("管理员导航", navFrame);
    navTitle->setStyleSheet("font-size:18px; font-weight:700;");
    auto* usersButton = new QPushButton("用户管理", navFrame);
    auto* booksButton = new QPushButton("图书管理", navFrame);
    auto* borrowsButton = new QPushButton("借阅管理", navFrame);
    auto* reservationsButton = new QPushButton("预约管理", navFrame);

    usersButton->setCheckable(true);
    booksButton->setCheckable(true);
    borrowsButton->setCheckable(true);
    reservationsButton->setCheckable(true);
    usersButton->setChecked(true);

    auto* navGroup = new QButtonGroup(this);
    navGroup->setExclusive(true);
    navGroup->addButton(usersButton, 0);
    navGroup->addButton(booksButton, 1);
    navGroup->addButton(borrowsButton, 2);
    navGroup->addButton(reservationsButton, 3);

    navLayout->addWidget(navTitle);
    navLayout->addWidget(usersButton);
    navLayout->addWidget(booksButton);
    navLayout->addWidget(borrowsButton);
    navLayout->addWidget(reservationsButton);
    navLayout->addStretch();

    auto* contentFrame = new QWidget(page);
    auto* contentLayout = new QVBoxLayout(contentFrame);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(18);

    auto* headerLayout = new QHBoxLayout();
    adminGreetingLabel_ = new QLabel("管理员工作台", contentFrame);
    adminGreetingLabel_->setObjectName("pageTitle");
    auto* refreshButton = new QPushButton("刷新数据", contentFrame);
    auto* logoutButton = new QPushButton("退出登录", contentFrame);
    connect(refreshButton, &QPushButton::clicked, this, [this]() { refreshAdminDashboard(); });
    connect(logoutButton, &QPushButton::clicked, this, [this]() { logout(); });

    headerLayout->addWidget(adminGreetingLabel_);
    headerLayout->addStretch();
    headerLayout->addWidget(refreshButton);
    headerLayout->addWidget(logoutButton);

    auto* metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(14);
    metricsLayout->addWidget(createMetricCard("用户总数", adminUsersMetric_));
    metricsLayout->addWidget(createMetricCard("馆藏图书", adminBooksMetric_));
    metricsLayout->addWidget(createMetricCard("未归还借阅", adminActiveBorrowMetric_));
    metricsLayout->addWidget(createMetricCard("逾期记录", adminOverdueMetric_));
    metricsLayout->addWidget(createMetricCard("预约记录", adminReservationMetric_));

    auto* controlLayout = new QHBoxLayout();
    adminSearchEdit_ = new QLineEdit(contentFrame);
    adminSearchEdit_->setPlaceholderText("搜索当前列表");
    adminPrevPageButton_ = new QPushButton("上一页", contentFrame);
    adminNextPageButton_ = new QPushButton("下一页", contentFrame);
    adminPageLabel_ = new QLabel("第 1 / 1 页", contentFrame);
    adminPageLabel_->setAlignment(Qt::AlignCenter);

    controlLayout->addWidget(adminSearchEdit_, 1);
    controlLayout->addWidget(adminPrevPageButton_);
    controlLayout->addWidget(adminPageLabel_);
    controlLayout->addWidget(adminNextPageButton_);

    adminUsersTable_ = new QTableWidget(page);
    adminUsersTable_->setColumnCount(6);
    adminUsersTable_->setHorizontalHeaderLabels({"ID", "用户名", "姓名", "角色", "状态", "需要缴纳的罚金"});

    adminBooksTable_ = new QTableWidget(page);
    adminBooksTable_->setColumnCount(8);
    adminBooksTable_->setHorizontalHeaderLabels({"ID", "书名", "作者", "分类", "总库存", "可借", "预约", "状态"});

    adminBorrowsTable_ = new QTableWidget(page);
    adminBorrowsTable_->setColumnCount(8);
    adminBorrowsTable_->setHorizontalHeaderLabels({"记录ID", "用户", "图书", "借阅时间", "应还时间", "状态", "罚金", "操作"});

    adminReservationsTable_ = new QTableWidget(page);
    adminReservationsTable_->setColumnCount(7);
    adminReservationsTable_->setHorizontalHeaderLabels({"预约ID", "用户", "图书", "预约时间", "取书截止", "状态", "操作"});

    const QList<QTableWidget*> tables = {adminUsersTable_, adminBooksTable_, adminBorrowsTable_, adminReservationsTable_};
    for (auto* table : tables) {
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        table->setAlternatingRowColors(true);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        connect(table, &QTableWidget::cellDoubleClicked, this, [this, table](int row, int) { showTableDetails(table, row); });
    }

    adminContentStack_ = new QStackedWidget(contentFrame);
    auto* usersPage = new QWidget(contentFrame);
    auto* booksPage = new QWidget(contentFrame);
    auto* borrowsPage = new QWidget(contentFrame);
    auto* reservationsPage = new QWidget(contentFrame);

    auto* usersLayout = new QVBoxLayout(usersPage);
    usersLayout->setContentsMargins(0, 0, 0, 0);
    usersLayout->addWidget(adminUsersTable_);

    auto* booksLayout = new QVBoxLayout(booksPage);
    booksLayout->setContentsMargins(0, 0, 0, 0);
    booksLayout->addWidget(adminBooksTable_);

    auto* borrowsLayout = new QVBoxLayout(borrowsPage);
    borrowsLayout->setContentsMargins(0, 0, 0, 0);
    borrowsLayout->addWidget(adminBorrowsTable_);

    auto* reservationsLayout = new QVBoxLayout(reservationsPage);
    reservationsLayout->setContentsMargins(0, 0, 0, 0);
    reservationsLayout->addWidget(adminReservationsTable_);

    adminContentStack_->addWidget(usersPage);
    adminContentStack_->addWidget(booksPage);
    adminContentStack_->addWidget(borrowsPage);
    adminContentStack_->addWidget(reservationsPage);

    contentLayout->addLayout(headerLayout);
    contentLayout->addLayout(metricsLayout);
    contentLayout->addLayout(controlLayout);
    contentLayout->addWidget(adminContentStack_, 1);

    rootLayout->addWidget(navFrame);
    rootLayout->addWidget(contentFrame, 1);

    connect(navGroup, &QButtonGroup::idClicked, this, [this](int index) {
        switchAdminSubPage(index);
    });
    connect(adminSearchEdit_, &QLineEdit::textChanged, this, &MainWindow::onAdminSearchChanged);
    connect(adminPrevPageButton_, &QPushButton::clicked, this, [this]() { onAdminPageChanged(-1); });
    connect(adminNextPageButton_, &QPushButton::clicked, this, [this]() { onAdminPageChanged(1); });

    switchAdminSubPage(0);
    return page;
}

QWidget* MainWindow::createReaderPage() {
    auto* page = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(page);
    rootLayout->setContentsMargins(24, 24, 24, 24);
    rootLayout->setSpacing(20);

    auto* headerLayout = new QHBoxLayout();
    readerGreetingLabel_ = new QLabel("读者工作台", page);
    readerGreetingLabel_->setObjectName("pageTitle");
    auto* refreshButton = new QPushButton("刷新数据", page);
    auto* logoutButton = new QPushButton("退出登录", page);
    connect(refreshButton, &QPushButton::clicked, this, [this]() { refreshReaderDashboard(); });
    connect(logoutButton, &QPushButton::clicked, this, [this]() { logout(); });

    headerLayout->addWidget(readerGreetingLabel_);
    headerLayout->addStretch();
    headerLayout->addWidget(refreshButton);
    headerLayout->addWidget(logoutButton);

    auto* metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(14);
    metricsLayout->addWidget(createMetricCard("当前借阅", readerBorrowMetric_));
    metricsLayout->addWidget(createMetricCard("当前罚金", readerFineMetric_));
    metricsLayout->addWidget(createMetricCard("我的订阅", readerReservationMetric_));

    auto* controlLayout = new QHBoxLayout();
    readerSearchEdit_ = new QLineEdit(page);
    readerSearchEdit_->setPlaceholderText("搜索当前列表");
    readerPrevPageButton_ = new QPushButton("上一页", page);
    readerNextPageButton_ = new QPushButton("下一页", page);
    readerPageLabel_ = new QLabel("第 1 / 1 页", page);
    readerPageLabel_->setAlignment(Qt::AlignCenter);
    controlLayout->addWidget(readerSearchEdit_, 1);
    controlLayout->addWidget(readerPrevPageButton_);
    controlLayout->addWidget(readerPageLabel_);
    controlLayout->addWidget(readerNextPageButton_);

    auto* tabs = new QTabWidget(page);
    tabs->setDocumentMode(true);

    readerBorrowTable_ = new QTableWidget(page);
    readerBorrowTable_->setColumnCount(7);
    readerBorrowTable_->setHorizontalHeaderLabels({"记录ID", "图书", "应还时间", "状态", "续借次数", "罚金", "操作"});

    readerReservationTable_ = new QTableWidget(page);
    readerReservationTable_->setColumnCount(6);
    readerReservationTable_->setHorizontalHeaderLabels({"预约ID", "图书", "预约时间", "取书截止", "状态", "操作"});

    readerCatalogTable_ = new QTableWidget(page);
    readerCatalogTable_->setColumnCount(9);
    readerCatalogTable_->setHorizontalHeaderLabels({"ID", "书名", "作者", "分类", "可借", "挂失", "预约", "状态", "操作"});

    const QList<QTableWidget*> tables = {readerBorrowTable_, readerReservationTable_, readerCatalogTable_};
    for (auto* table : tables) {
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        table->setAlternatingRowColors(true);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
    }

    tabs->addTab(readerBorrowTable_, "我的借阅");
    tabs->addTab(readerReservationTable_, "我的订阅");
    tabs->addTab(readerCatalogTable_, "图书目录");

    readerTabs_ = tabs;
    connect(readerTabs_, &QTabWidget::currentChanged, this, [this]() { updateCurrentReaderTableView(); });
    connect(readerSearchEdit_, &QLineEdit::textChanged, this, &MainWindow::onReaderSearchChanged);
    connect(readerPrevPageButton_, &QPushButton::clicked, this, [this]() { onReaderPageChanged(-1); });
    connect(readerNextPageButton_, &QPushButton::clicked, this, [this]() { onReaderPageChanged(1); });

    rootLayout->addLayout(headerLayout);
    rootLayout->addLayout(metricsLayout);
    rootLayout->addLayout(controlLayout);
    rootLayout->addWidget(tabs, 1);
    updateCurrentReaderTableView();
    return page;
}

static void applyTableFilterAndPaging(QTableWidget* table, const QString& filter, int& pageIndex, int pageSize, QLabel* pageLabel, QPushButton* prevButton, QPushButton* nextButton) {
    if (!table) {
        return;
    }

    const QString lowered = filter.trimmed().toLower();
    QVector<int> matches;
    const int rows = table->rowCount();
    const int cols = table->columnCount();

    for (int row = 0; row < rows; ++row) {
        bool match = lowered.isEmpty();
        for (int col = 0; col < cols && !match; ++col) {
            auto* item = table->item(row, col);
            if (item && item->text().toLower().contains(lowered)) {
                match = true;
            }
        }
        if (match) {
            matches.append(row);
        }
    }

    const int totalPages = qMax(1, (static_cast<int>(matches.size()) + pageSize - 1) / pageSize);
    pageIndex = qBound(0, pageIndex, totalPages - 1);

    const int startIndex = pageIndex * pageSize;
    const int endIndex = qMin(startIndex + pageSize, static_cast<int>(matches.size()));

    for (int row = 0; row < rows; ++row) {
        table->setRowHidden(row, true);
    }
    for (int index = startIndex; index < endIndex; ++index) {
        table->setRowHidden(matches[index], false);
    }

    if (pageLabel) {
        pageLabel->setText(QString("第 %1 / %2 页").arg(pageIndex + 1).arg(totalPages));
    }
    if (prevButton) {
        prevButton->setEnabled(pageIndex > 0);
    }
    if (nextButton) {
        nextButton->setEnabled(pageIndex + 1 < totalPages);
    }
}

void MainWindow::switchAdminSubPage(int index) {
    if (!adminContentStack_) {
        return;
    }
    adminContentStack_->setCurrentIndex(index);
    adminSearchEdit_->blockSignals(true);
    adminSearchEdit_->setText(adminSearchText_[index]);
    adminSearchEdit_->blockSignals(false);
    updateCurrentAdminTableView();
}

void MainWindow::updateCurrentAdminTableView() {
    if (!adminContentStack_) {
        return;
    }
    const int currentIndex = adminContentStack_->currentIndex();
    QTableWidget* currentTable = nullptr;
    switch (currentIndex) {
        case 0: currentTable = adminUsersTable_; break;
        case 1: currentTable = adminBooksTable_; break;
        case 2: currentTable = adminBorrowsTable_; break;
        case 3: currentTable = adminReservationsTable_; break;
        default: break;
    }
    applyTableFilterAndPaging(currentTable, adminSearchText_[currentIndex], adminPageIndex_[currentIndex], 10, adminPageLabel_, adminPrevPageButton_, adminNextPageButton_);
}

void MainWindow::onAdminSearchChanged(const QString& text) {
    const int currentIndex = adminContentStack_ ? adminContentStack_->currentIndex() : 0;
    adminSearchText_[currentIndex] = text;
    adminPageIndex_[currentIndex] = 0;
    updateCurrentAdminTableView();
}

void MainWindow::onAdminPageChanged(int delta) {
    const int currentIndex = adminContentStack_ ? adminContentStack_->currentIndex() : 0;
    adminPageIndex_[currentIndex] = qMax(0, adminPageIndex_[currentIndex] + delta);
    updateCurrentAdminTableView();
}

void MainWindow::updateCurrentReaderTableView() {
    if (!readerTabs_) {
        return;
    }
    const int currentIndex = readerTabs_->currentIndex();
    QTableWidget* currentTable = nullptr;
    switch (currentIndex) {
        case 0: currentTable = readerBorrowTable_; break;
        case 1: currentTable = readerReservationTable_; break;
        case 2: currentTable = readerCatalogTable_; break;
        default: break;
    }
    applyTableFilterAndPaging(currentTable, readerSearchText_[currentIndex], readerPageIndex_[currentIndex], 10, readerPageLabel_, readerPrevPageButton_, readerNextPageButton_);
}

void MainWindow::onReaderSearchChanged(const QString& text) {
    const int currentIndex = readerTabs_ ? readerTabs_->currentIndex() : 0;
    readerSearchText_[currentIndex] = text;
    readerPageIndex_[currentIndex] = 0;
    updateCurrentReaderTableView();
}

void MainWindow::onReaderPageChanged(int delta) {
    const int currentIndex = readerTabs_ ? readerTabs_->currentIndex() : 0;
    readerPageIndex_[currentIndex] = qMax(0, readerPageIndex_[currentIndex] + delta);
    updateCurrentReaderTableView();
}

void MainWindow::showTableDetails(QTableWidget* table, int row) {
    if (!table || row < 0 || row >= table->rowCount()) {
        return;
    }
    QStringList details;
    for (int col = 0; col < table->columnCount(); ++col) {
        const auto* header = table->horizontalHeaderItem(col);
        const auto* item = table->item(row, col);
        if (!header || !item) {
            continue;
        }
        const QString label = header->text();
        if (label == "操作") {
            continue;
        }
        details.append(QString("%1: %2").arg(label, item->text()));
    }
    QMessageBox::information(this, "行详情", details.join("\n"));
}

static bool isActiveReservationStatus(const std::string& status) {
    return status == "排队中" || status == "可取书";
}

static std::vector<ReservationRecord> normalizeReservations(const std::vector<ReservationRecord>& records) {
    struct ReservationKey {
        long long userId;
        long long bookId;
        bool operator<(const ReservationKey& other) const {
            return userId != other.userId ? userId < other.userId : bookId < other.bookId;
        }
    };

    std::map<ReservationKey, ReservationRecord> filtered;
    for (const auto& record : records) {
        ReservationKey key{record.userId, record.bookId};
        auto it = filtered.find(key);
        if (it == filtered.end()) {
            filtered[key] = record;
            continue;
        }

        const auto existingStatus = it->second.reservationStatus;
        const auto currentStatus = record.reservationStatus;
        if (isActiveReservationStatus(currentStatus) && !isActiveReservationStatus(existingStatus)) {
            it->second = record;
        } else if (isActiveReservationStatus(currentStatus) == isActiveReservationStatus(existingStatus) && record.reservationId > it->second.reservationId) {
            it->second = record;
        }
    }

    std::vector<ReservationRecord> result;
    result.reserve(filtered.size());
    for (auto& entry : filtered) {
        result.push_back(std::move(entry.second));
    }
    std::sort(result.begin(), result.end(), [](const ReservationRecord& a, const ReservationRecord& b) {
        return a.reservationId > b.reservationId;
    });
    return result;
}

bool MainWindow::confirmAction(const QString& title, const QString& text) {
    return QMessageBox::question(this, title, text, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}

void MainWindow::applyTheme() {
    qApp->setStyleSheet(
        "QMainWindow { background: #f3efe6; }"
        "QWidget { font-family: 'Microsoft YaHei'; font-size: 14px; color: #2f2924; }"
        "QFrame#heroPanel { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #c96f3b, stop:1 #7d4b32); border-radius: 24px; }"
        "QFrame#loginPanel, QFrame[class=\"metricCard\"] { background: #fffaf3; border: 1px solid #d8cabb; border-radius: 20px; }"
        "QLabel#heroTitle { color: white; font-size: 34px; font-weight: 700; }"
        "QLabel#heroSubtitle, QLabel#heroPoints { color: rgba(255,255,255,0.92); font-size: 15px; }"
        "QLabel#panelTitle, QLabel#pageTitle { font-size: 28px; font-weight: 700; color: #3a2d23; }"
        "QLabel#hintLabel { color: #7a6756; }"
        "QLineEdit { background: white; border: 1px solid #cbb8a7; border-radius: 12px; padding: 10px 12px; }"
        "QLineEdit:focus { border-color: #c96f3b; }"
        "QPushButton { background: #c96f3b; color: white; border: none; border-radius: 12px; padding: 10px 18px; font-weight: 600; }"
        "QPushButton:hover { background: #b56031; }"
        "QTabWidget::pane { border: 1px solid #d8cabb; border-radius: 18px; background: #fffaf3; top: -1px; }"
        "QTabBar::tab { background: #eadbc9; padding: 10px 16px; border-top-left-radius: 12px; border-top-right-radius: 12px; margin-right: 6px; }"
        "QTabBar::tab:selected { background: #fffaf3; }"
        "QHeaderView::section { background: #eadbc9; padding: 8px; border: none; font-weight: 600; }"
        "QTableWidget { background: white; border: none; gridline-color: #eadbc9; }"
    );
}

void MainWindow::initializeRepository() {
    std::string errorMessage;
    if (!repository_.initialize(errorMessage)) {
        setLoginStatus(QString::fromStdString(errorMessage), true);
    } else {
        setLoginStatus("数据库连接成功，可以开始登录。", false);
    }
}

void MainWindow::handleLogin() {
    std::string errorMessage;
    if (!repository_.initialize(errorMessage)) {
        setLoginStatus(QString::fromStdString(errorMessage), true);
        return;
    }

    const std::string username = usernameEdit_->text().trimmed().toStdString();
    const std::string password = passwordEdit_->text().toStdString();
    auto user = repository_.login(username, password, errorMessage);
    if (!user.has_value()) {
        setLoginStatus(QString::fromStdString(errorMessage), true);
        return;
    }

    currentUser_ = *user;
    passwordEdit_->clear();
    if (isAdmin(*currentUser_)) {
        showAdminDashboard();
    } else {
        showReaderDashboard();
    }
}

void MainWindow::logout() {
    currentUser_.reset();
    usernameEdit_->clear();
    passwordEdit_->clear();
    setLoginStatus("已退出登录。", false);
    if (adminRefreshTimer_) {
        adminRefreshTimer_->stop();
    }
    pages_->setCurrentIndex(0);
}

void MainWindow::showAdminDashboard() {
    if (!currentUser_.has_value()) {
        return;
    }

    adminGreetingLabel_->setText(QString("管理员工作台 · %1 (%2)")
                                     .arg(toText(currentUser_->realName))
                                     .arg(toText(currentUser_->username)));
    refreshAdminDashboard();
    if (!adminRefreshTimer_) {
        adminRefreshTimer_ = new QTimer(this);
        connect(adminRefreshTimer_, &QTimer::timeout, this, [this]() { refreshAdminDashboard(); });
    }
    adminRefreshTimer_->start(10000); // 10秒自动刷新
    pages_->setCurrentIndex(1);
}

void MainWindow::showReaderDashboard() {
    if (!currentUser_.has_value()) {
        return;
    }

    readerGreetingLabel_->setText(QString("读者工作台 · %1 (%2)")
                                      .arg(toText(currentUser_->realName))
                                      .arg(toText(currentUser_->studentNo)));
    refreshReaderDashboard();
    if (adminRefreshTimer_) {
        adminRefreshTimer_->stop();
    }
    pages_->setCurrentIndex(2);
}

void MainWindow::refreshAdminDashboard() {
    const auto users = repository_.getAllUsers();
    const auto books = repository_.getAllBooks();
    const auto activeBorrows = repository_.getActiveBorrowRecords();
    const auto overdueBorrows = repository_.getOverdueBorrowRecords();
    const auto reservations = repository_.getAllReservations();
    const auto filteredReservations = normalizeReservations(reservations);
    const int activeReservationCount = static_cast<int>(std::count_if(reservations.begin(), reservations.end(), [](const ReservationRecord& r) {
        return isActiveReservationStatus(r.reservationStatus);
    }));

    adminUsersMetric_->setText(QString::number(static_cast<int>(users.size())));
    adminBooksMetric_->setText(QString::number(static_cast<int>(books.size())));
    adminActiveBorrowMetric_->setText(QString::number(static_cast<int>(activeBorrows.size())));
    adminOverdueMetric_->setText(QString::number(static_cast<int>(overdueBorrows.size())));
    adminReservationMetric_->setText(QString::number(activeReservationCount));

    adminUsersTable_->setRowCount(static_cast<int>(users.size()));
    for (int row = 0; row < static_cast<int>(users.size()); ++row) {
        const auto& user = users[row];
        adminUsersTable_->setItem(row, 0, new QTableWidgetItem(QString::number(user.userId)));
        adminUsersTable_->setItem(row, 1, new QTableWidgetItem(toText(user.username)));
        adminUsersTable_->setItem(row, 2, new QTableWidgetItem(toText(user.realName)));
        adminUsersTable_->setItem(row, 3, new QTableWidgetItem(toText(user.roleName)));
        adminUsersTable_->setItem(row, 4, new QTableWidgetItem(toText(user.status)));
        
        double userTotalFine = 0.0;
        const auto userBorrows = repository_.getBorrowRecordsByUser(user.userId);
        for (const auto& borrow : userBorrows) {
            userTotalFine += borrow.fineAmount;
        }
        adminUsersTable_->setItem(row, 5, new QTableWidgetItem(moneyText(userTotalFine)));
    }

    adminBooksTable_->setRowCount(static_cast<int>(books.size()));
    for (int row = 0; row < static_cast<int>(books.size()); ++row) {
        const auto& book = books[row];
        adminBooksTable_->setItem(row, 0, new QTableWidgetItem(QString::number(book.bookId)));
        adminBooksTable_->setItem(row, 1, new QTableWidgetItem(toText(book.bookTitle)));
        adminBooksTable_->setItem(row, 2, new QTableWidgetItem(toText(book.author)));
        adminBooksTable_->setItem(row, 3, new QTableWidgetItem(toText(book.categoryName)));
        adminBooksTable_->setItem(row, 4, new QTableWidgetItem(QString::number(book.totalStock)));
        adminBooksTable_->setItem(row, 5, new QTableWidgetItem(QString::number(book.availableStock)));
        adminBooksTable_->setItem(row, 6, new QTableWidgetItem(QString("%1/%2").arg(book.pendingReservationCount).arg(book.readyReservationCount)));
        adminBooksTable_->setItem(row, 7, new QTableWidgetItem(toText(book.bookStatus)));
    }

    adminBorrowsTable_->setRowCount(static_cast<int>(activeBorrows.size()));
    for (int row = 0; row < static_cast<int>(activeBorrows.size()); ++row) {
        const auto& borrow = activeBorrows[row];
        adminBorrowsTable_->setItem(row, 0, new QTableWidgetItem(QString::number(borrow.recordId)));
        adminBorrowsTable_->setItem(row, 1, new QTableWidgetItem(toText(borrow.realName)));
        adminBorrowsTable_->setItem(row, 2, new QTableWidgetItem(toText(borrow.bookTitle)));
        adminBorrowsTable_->setItem(row, 3, new QTableWidgetItem(toText(borrow.borrowTime)));
        adminBorrowsTable_->setItem(row, 4, new QTableWidgetItem(toText(borrow.dueTime)));
        adminBorrowsTable_->setItem(row, 5, new QTableWidgetItem(toText(borrow.recordStatus)));
        adminBorrowsTable_->setItem(row, 6, new QTableWidgetItem(moneyText(borrow.fineAmount)));

        const bool canReturn = borrow.recordStatus != "挂失" && borrow.returnTime.empty();
        const QString buttonText = borrow.recordStatus == "挂失" ? "已挂失" : (borrow.returnTime.empty() ? "还书" : "已归还");
        auto* actionButton = new QPushButton(buttonText, adminBorrowsTable_);
        actionButton->setProperty("recordId", QVariant::fromValue<qint64>(borrow.recordId));
        actionButton->setEnabled(canReturn);
        connect(actionButton, &QPushButton::clicked, this, [this, actionButton]() {
            if (!confirmAction("确认还书", "确定要将此借阅记录标记为已还书吗？")) {
                return;
            }
            const qint64 recordId = actionButton->property("recordId").toLongLong();
            std::string errorMessage;
            if (repository_.returnBook(recordId, currentUser_ ? currentUser_->userId : 0, errorMessage)) {
                refreshAdminDashboard();
                QMessageBox::information(this, "操作成功", "借阅记录已还书。当前列表已刷新。");
            } else {
                QMessageBox::warning(this, "操作失败", QString::fromStdString(errorMessage));
            }
        });
        auto* actionWidget = new QWidget(adminBorrowsTable_);
        auto* actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(0, 0, 0, 0);
        actionLayout->addWidget(actionButton);
        adminBorrowsTable_->setCellWidget(row, 7, actionWidget);
    }

    adminReservationsTable_->setRowCount(static_cast<int>(filteredReservations.size()));
    for (int row = 0; row < static_cast<int>(filteredReservations.size()); ++row) {
        const auto& reservation = filteredReservations[row];
        adminReservationsTable_->setItem(row, 0, new QTableWidgetItem(QString::number(reservation.reservationId)));
        adminReservationsTable_->setItem(row, 1, new QTableWidgetItem(toText(reservation.realName)));
        adminReservationsTable_->setItem(row, 2, new QTableWidgetItem(toText(reservation.bookTitle)));
        adminReservationsTable_->setItem(row, 3, new QTableWidgetItem(toText(reservation.reservationTime)));
        adminReservationsTable_->setItem(row, 4, new QTableWidgetItem(toText(reservation.pickupDeadline)));
        adminReservationsTable_->setItem(row, 5, new QTableWidgetItem(toText(reservation.reservationStatus)));

        auto* cancelButton = new QPushButton(reservation.reservationStatus == "已取消" ? "恢复预约" : "取消预约", adminReservationsTable_);
        cancelButton->setProperty("reservationId", QVariant::fromValue<qint64>(reservation.reservationId));
        cancelButton->setProperty("bookId", QVariant::fromValue<qint64>(reservation.bookId));
        cancelButton->setProperty("reservationUserId", QVariant::fromValue<qint64>(reservation.userId));
        cancelButton->setEnabled(reservation.reservationStatus != "已完成");
        if (reservation.reservationStatus == "已取消") {
            connect(cancelButton, &QPushButton::clicked, this, [this, cancelButton]() {
                if (!confirmAction("确认恢复预约", "确定要恢复此已取消的预约吗？")) {
                    return;
                }
                const qint64 userId = cancelButton->property("reservationUserId").toLongLong();
                const qint64 bookId = cancelButton->property("bookId").toLongLong();
                std::string errorMessage;
                if (repository_.reserveBook(userId, bookId, currentUser_ ? currentUser_->userId : 0, "恢复取消预约", errorMessage)) {
                    refreshAdminDashboard();
                    QMessageBox::information(this, "操作成功", "预约已恢复，当前列表已刷新。");
                } else {
                    QMessageBox::warning(this, "恢复失败", QString::fromStdString(errorMessage));
                }
            });
        } else {
            connect(cancelButton, &QPushButton::clicked, this, [this, cancelButton]() {
                if (!confirmAction("确认取消预约", "确定要取消此预约吗？")) {
                    return;
                }
                const qint64 reservationId = cancelButton->property("reservationId").toLongLong();
                std::string errorMessage;
                if (repository_.cancelReservation(reservationId, currentUser_ ? currentUser_->userId : 0, errorMessage)) {
                    refreshAdminDashboard();
                    QMessageBox::information(this, "操作成功", "预约已取消，当前列表已刷新。");
                } else {
                    QMessageBox::warning(this, "操作失败", QString::fromStdString(errorMessage));
                }
            });
        }
        auto* cancelWidget = new QWidget(adminReservationsTable_);
        auto* cancelLayout = new QHBoxLayout(cancelWidget);
        cancelLayout->setContentsMargins(0, 0, 0, 0);
        cancelLayout->addWidget(cancelButton);
        adminReservationsTable_->setCellWidget(row, 6, cancelWidget);
    }
    updateCurrentAdminTableView();
}

void MainWindow::refreshReaderDashboard() {
    if (!currentUser_.has_value()) {
        return;
    }

    const auto allBooks = repository_.getAllBooks();
    const auto borrows = repository_.getBorrowRecordsByUser(currentUser_->userId);
    const auto reservations = repository_.getReservationsByUser(currentUser_->userId);
    const auto filteredReservations = normalizeReservations(reservations);

    int activeBorrowCount = 0;
    double currentFine = 0.0;
    for (const auto& borrow : borrows) {
        if (borrow.returnTime.empty()) {
            ++activeBorrowCount;
        }
        currentFine += borrow.fineAmount;
    }

    const int activeReservationCount = static_cast<int>(std::count_if(filteredReservations.begin(), filteredReservations.end(), [](const ReservationRecord& r) {
        return isActiveReservationStatus(r.reservationStatus);
    }));

    readerBorrowMetric_->setText(QString::number(activeBorrowCount));
    readerFineMetric_->setText(moneyText(currentFine));
    readerReservationMetric_->setText(QString::number(activeReservationCount));

    readerBorrowTable_->setRowCount(static_cast<int>(borrows.size()));
    for (int row = 0; row < static_cast<int>(borrows.size()); ++row) {
        const auto& borrow = borrows[row];
        readerBorrowTable_->setItem(row, 0, new QTableWidgetItem(QString::number(borrow.recordId)));
        readerBorrowTable_->setItem(row, 1, new QTableWidgetItem(toText(borrow.bookTitle)));
        readerBorrowTable_->setItem(row, 2, new QTableWidgetItem(toText(borrow.dueTime)));
        readerBorrowTable_->setItem(row, 3, new QTableWidgetItem(toText(borrow.recordStatus)));
        readerBorrowTable_->setItem(row, 4, new QTableWidgetItem(QString("%1/%2").arg(borrow.renewCount).arg(borrow.maxRenewCount)));
        readerBorrowTable_->setItem(row, 5, new QTableWidgetItem(moneyText(borrow.fineAmount)));

        auto* actionWidget = new QWidget(readerBorrowTable_);
        auto* actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(0, 0, 0, 0);
        actionLayout->setSpacing(4);

        auto* returnButton = new QPushButton("还书", actionWidget);
        returnButton->setProperty("recordId", QVariant::fromValue<qint64>(borrow.recordId));
        returnButton->setEnabled(borrow.returnTime.empty());
        connect(returnButton, &QPushButton::clicked, this, [this, returnButton]() {
            if (!confirmAction("确认还书", "确定要将此借阅记录标记为已还书吗？")) {
                return;
            }
            const qint64 recordId = returnButton->property("recordId").toLongLong();
            std::string errorMessage;
            if (repository_.returnBook(recordId, currentUser_ ? currentUser_->userId : 0, errorMessage)) {
                refreshReaderDashboard();
                QMessageBox::information(this, "操作成功", "已还书，列表已刷新。");
            } else {
                QMessageBox::warning(this, "操作失败", QString::fromStdString(errorMessage));
            }
        });
        actionLayout->addWidget(returnButton);

        auto* lostButton = new QPushButton("挂失", actionWidget);
        lostButton->setProperty("recordId", QVariant::fromValue<qint64>(borrow.recordId));
        lostButton->setEnabled(borrow.returnTime.empty());
        connect(lostButton, &QPushButton::clicked, this, [this, lostButton]() {
            if (!confirmAction("确认挂失", "确定要挂失此借阅记录吗？挂失后将无法继续借阅此书，且操作不可恢复。")) {
                return;
            }
            const qint64 recordId = lostButton->property("recordId").toLongLong();
            std::string errorMessage;
            if (repository_.reportLost(recordId, currentUser_ ? currentUser_->userId : 0, errorMessage)) {
                refreshReaderDashboard();
                QMessageBox::information(this, "操作成功", "已挂失，列表已刷新。");
            } else {
                QMessageBox::warning(this, "操作失败", QString::fromStdString(errorMessage));
            }
        });
        actionLayout->addWidget(lostButton);

        readerBorrowTable_->setCellWidget(row, 6, actionWidget);
    }

    readerReservationTable_->setRowCount(static_cast<int>(filteredReservations.size()));
    for (int row = 0; row < static_cast<int>(filteredReservations.size()); ++row) {
        const auto& reservation = filteredReservations[row];
        readerReservationTable_->setItem(row, 0, new QTableWidgetItem(QString::number(reservation.reservationId)));
        readerReservationTable_->setItem(row, 1, new QTableWidgetItem(toText(reservation.bookTitle)));
        readerReservationTable_->setItem(row, 2, new QTableWidgetItem(toText(reservation.reservationTime)));
        readerReservationTable_->setItem(row, 3, new QTableWidgetItem(toText(reservation.pickupDeadline)));
        readerReservationTable_->setItem(row, 4, new QTableWidgetItem(toText(reservation.reservationStatus)));

        auto* cancelButton = new QPushButton(reservation.reservationStatus == "已取消" ? "恢复预约" : "取消预约", readerReservationTable_);
        cancelButton->setProperty("reservationId", QVariant::fromValue<qint64>(reservation.reservationId));
        cancelButton->setProperty("bookId", QVariant::fromValue<qint64>(reservation.bookId));
        cancelButton->setEnabled(reservation.reservationStatus != "已完成");

        const auto bookInfoIt = std::find_if(allBooks.begin(), allBooks.end(), [&reservation](const auto& book) {
            return book.bookId == reservation.bookId;
        });
        const bool hasAvailableStock = bookInfoIt != allBooks.end() && bookInfoIt->availableStock > bookInfoIt->readyReservationCount && bookInfoIt->bookStatus != "已挂失";
        const bool canBorrowFromSubscription = reservation.reservationStatus == "可取书" || (reservation.reservationStatus == "已取消" && hasAvailableStock);

        if (reservation.reservationStatus == "已取消") {
            connect(cancelButton, &QPushButton::clicked, this, [this, cancelButton]() {
                if (!confirmAction("确认恢复预约", "确定要恢复此已取消的预约吗？")) {
                    return;
                }
                const qint64 bookId = cancelButton->property("bookId").toLongLong();
                std::string errorMessage;
                if (repository_.reserveBook(currentUser_ ? currentUser_->userId : 0, bookId, currentUser_ ? currentUser_->userId : 0, "恢复取消预约", errorMessage)) {
                    refreshReaderDashboard();
                    QMessageBox::information(this, "操作成功", "预约已恢复，列表已刷新。");
                } else {
                    QMessageBox::warning(this, "恢复失败", QString::fromStdString(errorMessage));
                }
            });
        } else {
            connect(cancelButton, &QPushButton::clicked, this, [this, cancelButton]() {
                if (!confirmAction("确认取消预约", "确定要取消此预约吗？")) {
                    return;
                }
                const qint64 reservationId = cancelButton->property("reservationId").toLongLong();
                std::string errorMessage;
                if (repository_.cancelReservation(reservationId, currentUser_ ? currentUser_->userId : 0, errorMessage)) {
                    refreshReaderDashboard();
                    QMessageBox::information(this, "操作成功", "预约已取消，列表已刷新。");
                } else {
                    QMessageBox::warning(this, "操作失败", QString::fromStdString(errorMessage));
                }
            });
        }

        auto* borrowButton = new QPushButton("借书", readerReservationTable_);
        borrowButton->setProperty("bookId", QVariant::fromValue<qint64>(reservation.bookId));
        borrowButton->setEnabled(canBorrowFromSubscription);
        borrowButton->setStyleSheet(canBorrowFromSubscription ? "background:#3fa55a; color:white;" : "");
        connect(borrowButton, &QPushButton::clicked, this, [this, borrowButton]() {
            if (!confirmAction("确认借书", "确定要借阅这本已预约的图书吗？")) {
                return;
            }
            const qint64 bookId = borrowButton->property("bookId").toLongLong();
            std::string errorMessage;
            if (repository_.borrowBook(currentUser_ ? currentUser_->userId : 0, bookId, currentUser_ ? currentUser_->userId : 0, errorMessage)) {
                refreshReaderDashboard();
                QMessageBox::information(this, "操作成功", "借书成功，列表已刷新。");
            } else {
                QMessageBox::warning(this, "操作失败", QString::fromStdString(errorMessage));
            }
        });

        auto* cancelWidget = new QWidget(readerReservationTable_);
        auto* cancelLayout = new QHBoxLayout(cancelWidget);
        cancelLayout->setContentsMargins(0, 0, 0, 0);
        cancelLayout->setSpacing(4);
        cancelLayout->addWidget(cancelButton);
        cancelLayout->addWidget(borrowButton);
        readerReservationTable_->setCellWidget(row, 5, cancelWidget);
    }

    readerCatalogTable_->setRowCount(static_cast<int>(allBooks.size()));
    for (int row = 0; row < static_cast<int>(allBooks.size()); ++row) {
        const auto& book = allBooks[row];
        readerCatalogTable_->setItem(row, 0, new QTableWidgetItem(QString::number(book.bookId)));
        readerCatalogTable_->setItem(row, 1, new QTableWidgetItem(toText(book.bookTitle)));
        readerCatalogTable_->setItem(row, 2, new QTableWidgetItem(toText(book.author)));
        readerCatalogTable_->setItem(row, 3, new QTableWidgetItem(toText(book.categoryName)));
        readerCatalogTable_->setItem(row, 4, new QTableWidgetItem(QString::number(book.availableStock)));
        readerCatalogTable_->setItem(row, 5, new QTableWidgetItem(QString::number(book.lostCount)));
        readerCatalogTable_->setItem(row, 6, new QTableWidgetItem(QString("%1/%2").arg(book.pendingReservationCount).arg(book.readyReservationCount)));
        readerCatalogTable_->setItem(row, 7, new QTableWidgetItem(toText(book.bookStatus)));

        const bool canBorrow = book.availableStock > 0 && book.bookStatus != "已挂失";
        const bool canSubscribe = !canBorrow && book.bookStatus != "已挂失";
        const QString buttonText = canBorrow ? "借书" : (canSubscribe ? "订阅" : "不可用");
        auto* actionButton = new QPushButton(buttonText, readerCatalogTable_);
        actionButton->setProperty("bookId", QVariant::fromValue<qint64>(book.bookId));
        actionButton->setEnabled(canBorrow || canSubscribe);
        if (canSubscribe) {
            actionButton->setStyleSheet("background:#4f93e6; color:white;");
        } else if (canBorrow) {
            actionButton->setStyleSheet("background:#3fa55a; color:white;");
        }
        connect(actionButton, &QPushButton::clicked, this, [this, actionButton, canBorrow]() {
            const qint64 bookId = actionButton->property("bookId").toLongLong();
            std::string errorMessage;
            if (canBorrow) {
                if (repository_.borrowBook(currentUser_ ? currentUser_->userId : 0, bookId, currentUser_ ? currentUser_->userId : 0, errorMessage)) {
                    refreshReaderDashboard();
                    QMessageBox::information(this, "操作成功", "借书成功，列表已刷新。");
                    return;
                }
            } else {
                if (repository_.reserveBook(currentUser_ ? currentUser_->userId : 0, bookId, currentUser_ ? currentUser_->userId : 0, "桌面端订阅", errorMessage)) {
                    refreshReaderDashboard();
                    QMessageBox::information(this, "操作成功", "订阅成功，列表已刷新。");
                    return;
                }
            }
            QMessageBox::warning(this, "操作失败", QString::fromStdString(errorMessage));
        });
        readerCatalogTable_->setCellWidget(row, 8, actionButton);
    }
    updateCurrentReaderTableView();
}

void MainWindow::setLoginStatus(const QString& message, bool isError) {
    loginStatusLabel_->setText(message);
    loginStatusLabel_->setStyleSheet(isError ? "color:#b23a2f;" : "color:#2f6f44;");
    loginStatusLabel_->show();
}

QWidget* MainWindow::createMetricCard(const QString& title, QLabel*& valueLabel) {
    auto* card = new QFrame(this);
    card->setObjectName("metricCard");
    card->setProperty("class", "metricCard");
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(8);

    auto* titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet("color:#7a6756; font-size:13px;");
    valueLabel = new QLabel("0", card);
    valueLabel->setStyleSheet("font-size:26px; font-weight:700; color:#3a2d23;");

    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    layout->addStretch();
    return card;
}

bool MainWindow::isAdmin(const User& user) {
    return user.roleId == 1 || user.roleName == "admin";
}

QString MainWindow::toText(const std::string& value) {
    return QString::fromUtf8(value.c_str());
}

QString MainWindow::moneyText(double value) {
    return QString("¥ %1").arg(QString::number(value, 'f', 2));
}

#include "AdminDashboard.h"
#include "Messages.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QGroupBox>

namespace bus {

AdminDashboard::AdminDashboard(NetworkManager* net, int userId, const QString& username, QWidget* parent)
    : QWidget(parent), m_net(net), m_userId(userId)
{
    buildUI();
    setWindowTitle("Admin Panel - " + username);
    connect(m_net, &NetworkManager::messageReceived, this,  &AdminDashboard::onMessageReceived);
    connect(m_net, &NetworkManager::networkError, this,  &AdminDashboard::onNetworkError);
}

void AdminDashboard::buildUI() {
    auto* root   = new QVBoxLayout(this);
    auto* topBar = new QHBoxLayout;
    auto* logoutBtn  = new QPushButton("Logout");
    auto* refreshBtn = new QPushButton("Refresh");
    topBar->addStretch();
    topBar->addWidget(refreshBtn);
    topBar->addWidget(logoutBtn);
    root->addLayout(topBar);

    connect(logoutBtn,  &QPushButton::clicked, this, &AdminDashboard::logoutRequested);
    connect(refreshBtn, &QPushButton::clicked, this, [this]{ refresh(); });

    m_tabs = new QTabWidget;
    auto* tabs = m_tabs;

    // ---- Manage Routes tab ----
    auto* routesW = new QWidget;
    auto* routesL = new QVBoxLayout(routesW);
    m_routesList  = new QListWidget;
    auto* form    = new QFormLayout;
    m_routeNameEdit     = new QLineEdit;
    m_routeStopsEdit    = new QLineEdit;
    m_routeScheduleEdit = new QLineEdit;
    form->addRow("Route Name:", m_routeNameEdit);
    form->addRow("Stops:",      m_routeStopsEdit);
    form->addRow("Schedule:",   m_routeScheduleEdit);
    auto* addBtn = new QPushButton("Add Route");
    auto* delBtn = new QPushButton("Delete Selected Route");
    routesL->addWidget(new QLabel("Existing Routes:"));
    routesL->addWidget(m_routesList);
    routesL->addLayout(form);
    routesL->addWidget(addBtn);
    routesL->addWidget(delBtn);
    connect(addBtn, &QPushButton::clicked, this, &AdminDashboard::onAddRouteClicked);
    connect(delBtn, &QPushButton::clicked, this, &AdminDashboard::onDeleteRouteClicked);
    tabs->addTab(routesW, "Manage Routes");

    // ---- Bus Dispatch / Demand tab ----
    auto* demandW = new QWidget;
    auto* demandL = new QVBoxLayout(demandW);
    m_demandList  = new QListWidget;
    auto* dispatchBtn = new QPushButton("Dispatch Bus for Selected Demand");
    demandL->addWidget(new QLabel("Highest Voted Slots (System Suggested Buses):"));
    demandL->addWidget(m_demandList);
    demandL->addWidget(dispatchBtn);
    connect(dispatchBtn, &QPushButton::clicked, this, &AdminDashboard::onDispatchBusClicked);
    tabs->addTab(demandW, "Bus Dispatch");

    // ---- All Users tab ----
    auto* usersW     = new QWidget;
    auto* usersL     = new QVBoxLayout(usersW);
    m_usersList      = new QListWidget;
    auto* inspectBtn = new QPushButton("Inspect Selected User →");
    usersL->addWidget(new QLabel("Registered Users (double-click to inspect):"));
    usersL->addWidget(m_usersList);
    usersL->addWidget(inspectBtn);
    tabs->addTab(usersW, "All Users");

    connect(inspectBtn, &QPushButton::clicked, this, [this]{
        auto* item = m_usersList->currentItem();
        if (item) onUserSelected(item);
    });
    connect(m_usersList, &QListWidget::itemDoubleClicked,
            this, &AdminDashboard::onUserSelected);

    // ---- User Detail tab ----
    auto* detailW = new QWidget;
    auto* detailL = new QVBoxLayout(detailW);

    m_userDetailNameLabel = new QLabel("Select a user from the \"All Users\" tab to inspect.");
    m_userDetailNameLabel->setAlignment(Qt::AlignCenter);
    m_userDetailNameLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    detailL->addWidget(m_userDetailNameLabel);

    auto* splitter = new QSplitter(Qt::Vertical);

    auto* subsGroup  = new QGroupBox("Subscribed Routes");
    auto* subsGroupL = new QVBoxLayout(subsGroup);
    m_userDetailSubsList = new QListWidget;
    subsGroupL->addWidget(m_userDetailSubsList);
    splitter->addWidget(subsGroup);

    auto* ticketsGroup  = new QGroupBox("Tickets Issued");
    auto* ticketsGroupL = new QVBoxLayout(ticketsGroup);
    m_userDetailTicketsList = new QListWidget;
    ticketsGroupL->addWidget(m_userDetailTicketsList);
    splitter->addWidget(ticketsGroup);

    auto* freeGroup  = new QGroupBox("Preferred Free Time Slots (Votes)");
    auto* freeGroupL = new QVBoxLayout(freeGroup);
    m_userDetailFreeTime = new QListWidget;
    freeGroupL->addWidget(m_userDetailFreeTime);
    splitter->addWidget(freeGroup);

    detailL->addWidget(splitter);
    tabs->addTab(detailW, "User Detail");

    root->addWidget(tabs);

    m_statusLabel = new QLabel;
    m_statusLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_statusLabel);
}

void AdminDashboard::refresh() {
    nlohmann::json r1; r1["type"] = MSG_GET_ROUTES; m_net->sendRequest(r1);
    nlohmann::json r2; r2["type"] = MSG_GET_USERS; m_net->sendRequest(r2);
    nlohmann::json r3; r3["type"] = MSG_GET_DEMAND; m_net->sendRequest(r3);
}

void AdminDashboard::onAddRouteClicked() {
    if (m_routeNameEdit->text().isEmpty()) { showStatus("Route name cannot be empty.", true); return; }
    nlohmann::json req;
    req["type"]     = MSG_ADD_ROUTE;
    req["name"]     = m_routeNameEdit->text().toStdString();
    req["stops"]    = m_routeStopsEdit->text().toStdString();
    req["schedule"] = m_routeScheduleEdit->text().toStdString();
    m_net->sendRequest(req);
    m_routeNameEdit->clear(); m_routeStopsEdit->clear(); m_routeScheduleEdit->clear();
}

void AdminDashboard::onDeleteRouteClicked() {
    auto* item = m_routesList->currentItem();
    if (!item) { showStatus("Select a route to delete.", true); return; }
    nlohmann::json req; req["type"] = MSG_DEL_ROUTE; req["routeId"] = item->data(Qt::UserRole).toInt();
    m_net->sendRequest(req);
}

void AdminDashboard::onDispatchBusClicked() {
    auto* item = m_demandList->currentItem();
    if (!item) { showStatus("Select a demand slot to dispatch a bus for.", true); return; }
    nlohmann::json req;
    req["type"]      = MSG_DISPATCH_BUS;
    req["routeId"]   = item->data(Qt::UserRole).toInt();
    req["day"]       = item->data(Qt::UserRole + 1).toString().toStdString();
    req["timeSlot"]  = item->data(Qt::UserRole + 2).toString().toStdString();
    req["direction"] = item->data(Qt::UserRole + 3).toString().toStdString();
    m_net->sendRequest(req);
}

void AdminDashboard::onMessageReceived(const nlohmann::json& msg) {
    std::string type = msg.value("type", "");
    if (type == MSG_ROUTES_LIST) {
        m_routesList->clear();
        for (const auto& r : msg["routes"]) {
            auto* item = new QListWidgetItem(QString::fromStdString(r["name"].get<std::string>()));
            item->setData(Qt::UserRole, r["id"].get<int>());
            m_routesList->addItem(item);
        }
    } else if (type == MSG_USERS_LIST) {
        m_usersList->clear();
        for (const auto& u : msg["users"]) {
            QString label = QString::fromStdString(u["username"].get<std::string>())
                          + "  [" + QString::fromStdString(u["role"].get<std::string>()) + "]";
            auto* item = new QListWidgetItem(label);
            item->setData(Qt::UserRole, u["id"].get<int>());
            m_usersList->addItem(item);
        }
    } else if (type == MSG_DEMAND_LIST) {
        m_demandList->clear();
        for (const auto& d : msg["demand"]) {
            int routeId = d["routeId"].get<int>();
            QString rName = QString::fromStdString(d["routeName"].get<std::string>());
            QString day = QString::fromStdString(d["day"].get<std::string>());
            QString time = QString::fromStdString(d["timeSlot"].get<std::string>());
            QString dir = QString::fromStdString(d["direction"].get<std::string>());
            int votes = d["votes"].get<int>();
            QString bus = QString::fromStdString(d["busType"].get<std::string>());
            QString text = QString("Route: %1 | %2 | %3 %4 | Votes: %5 -> Use: %6").arg(rName, day, dir, time, QString::number(votes), bus);
            auto* item = new QListWidgetItem(text);
            item->setData(Qt::UserRole, routeId);
            item->setData(Qt::UserRole + 1, day);
            item->setData(Qt::UserRole + 2, time);
            item->setData(Qt::UserRole + 3, dir);
            m_demandList->addItem(item);
        }
    } else if (type == MSG_OK) {
        showStatus(QString::fromStdString(msg.value("message","")), false);
        refresh();
    } else if (type == MSG_ROUTE_OK) {
        showStatus("Route added successfully.", false);
        refresh();
    } else if (type == MSG_ERROR) {
        showStatus(QString::fromStdString(msg.value("message","Error")), true);
    } else if (type == MSG_USER_DETAIL) {
        onMessageUserDetail(msg);
    }
}

void AdminDashboard::onNetworkError(const QString& error) {
    showStatus("Network error: " + error, true);
}

void AdminDashboard::showStatus(const QString& text, bool isError) {
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(isError ? "color:red;" : "color:green;");
}

void AdminDashboard::onUserSelected(QListWidgetItem* item) {
    m_selectedUserId = item->data(Qt::UserRole).toInt();
    nlohmann::json req;
    req["type"]   = MSG_GET_USER_DETAIL;
    req["userId"] = m_selectedUserId;
    m_net->sendRequest(req);
    m_tabs->setCurrentIndex(3);
}

void AdminDashboard::onMessageUserDetail(const nlohmann::json& msg) {
    QString username = QString::fromStdString(msg["user"]["username"].get<std::string>());
    m_userDetailNameLabel->setText("Details for: " + username);

    m_userDetailSubsList->clear();
    for (const auto& r : msg["subscriptions"]) {
        QString text = QString("Route #%1: %2  |  Stops: %3  |  Schedule: %4")
            .arg(r["id"].get<int>())
            .arg(QString::fromStdString(r["name"].get<std::string>()))
            .arg(QString::fromStdString(r["stops"].get<std::string>()))
            .arg(QString::fromStdString(r["schedule"].get<std::string>()));
        m_userDetailSubsList->addItem(text);
    }
    if (msg["subscriptions"].empty())
        m_userDetailSubsList->addItem("(no subscriptions)");

    m_userDetailTicketsList->clear();
    for (const auto& t : msg["tickets"]) {
        QString text = QString("Ticket #%1  |  Route #%2  |  Issued: %3  |  Price: %4 EGP")
            .arg(t["id"].get<int>())
            .arg(t["routeId"].get<int>())
            .arg(QString::fromStdString(t["issuedAt"].get<std::string>()))
            .arg(t["price"].get<int>());
        m_userDetailTicketsList->addItem(text);
    }
    if (msg["tickets"].empty())
        m_userDetailTicketsList->addItem("(no tickets issued)");

    m_userDetailFreeTime->clear();
    for (const auto& v : msg["freeTime"]) {
        QString text = QString("Route #%1  |  Day: %2  |  Time: %3  |  Direction: %4")
            .arg(v["routeId"].get<int>())
            .arg(QString::fromStdString(v["day"].get<std::string>()))
            .arg(QString::fromStdString(v["timeSlot"].get<std::string>()))
            .arg(QString::fromStdString(v["direction"].get<std::string>()));
        m_userDetailFreeTime->addItem(text);
    }
    if (msg["freeTime"].empty())
        m_userDetailFreeTime->addItem("(no free time preferences set)");
}

} // namespace bus
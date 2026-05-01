#include "AdminDashboard.h"
#include "Messages.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

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

    auto* tabs = new QTabWidget;

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
    auto* usersW = new QWidget;
    auto* usersL = new QVBoxLayout(usersW);
    m_usersList  = new QListWidget;
    usersL->addWidget(new QLabel("Registered Users:"));
    usersL->addWidget(m_usersList);
    tabs->addTab(usersW, "All Users");

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
            m_usersList->addItem(QString::fromStdString(u["username"].get<std::string>()) + "  [" + QString::fromStdString(u["role"].get<std::string>()) + "]");
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
    }
}

void AdminDashboard::onNetworkError(const QString& error) {
    showStatus("Network error: " + error, true);
}

void AdminDashboard::showStatus(const QString& text, bool isError) {
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(isError ? "color:red;" : "color:green;");
}

} // namespace bus

#include "UserDashboard.h"
#include "Messages.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTime>

namespace bus {

static const QStringList WEEKDAYS = {
    "Monday", "Tuesday", "Wednesday", "Thursday", "Friday"
};

UserDashboard::UserDashboard(NetworkManager* net,
                             int userId,
                             const QString& username,
                             QWidget* parent)
    : QWidget(parent), m_net(net), m_userId(userId)
{
    buildUI();
    setWindowTitle("Welcome, " + username);
    connect(m_net, &NetworkManager::messageReceived,
            this,  &UserDashboard::onMessageReceived);
    connect(m_net, &NetworkManager::networkError,
            this,  &UserDashboard::onNetworkError);
}

void UserDashboard::buildUI() {
    auto* root   = new QVBoxLayout(this);
    auto* topBar = new QHBoxLayout;

    auto* logoutBtn  = new QPushButton("Logout");
    auto* refreshBtn = new QPushButton("Refresh");
    topBar->addStretch();
    topBar->addWidget(refreshBtn);
    topBar->addWidget(logoutBtn);
    root->addLayout(topBar);
    connect(logoutBtn,  &QPushButton::clicked, this, &UserDashboard::logoutRequested);
    connect(refreshBtn, &QPushButton::clicked, this, &UserDashboard::onRefreshClicked);

    auto* tabs = new QTabWidget;

    // ---- Routes tab ----
    auto* routesW  = new QWidget;
    auto* routesL  = new QVBoxLayout(routesW);
    m_routesList   = new QListWidget;
    auto* subBtn   = new QPushButton("Subscribe to Selected Route");
    routesL->addWidget(new QLabel("Available Routes:"));
    routesL->addWidget(m_routesList);
    routesL->addWidget(subBtn);
    connect(subBtn, &QPushButton::clicked, this, &UserDashboard::onSubscribeClicked);
    tabs->addTab(routesW, "Routes");

    // ---- Subscriptions tab ----
    auto* subsW    = new QWidget;
    auto* subsL    = new QVBoxLayout(subsW);
    m_subsList     = new QListWidget;
    auto* unsubBtn = new QPushButton("Unsubscribe from Selected");
    auto* rideBtn  = new QPushButton("Report Ride on Selected Route");
    subsL->addWidget(new QLabel("My Subscriptions:"));
    subsL->addWidget(m_subsList);
    subsL->addWidget(unsubBtn);
    subsL->addWidget(rideBtn);
    connect(unsubBtn, &QPushButton::clicked, this, &UserDashboard::onUnsubscribeClicked);
    connect(rideBtn,  &QPushButton::clicked, this, &UserDashboard::onReportRideClicked);
    tabs->addTab(subsW, "My Subscriptions");

    // ---- Tickets tab ----
    auto* ticketsW = new QWidget;
    auto* ticketsL = new QVBoxLayout(ticketsW);
    m_ticketsList  = new QListWidget;
    ticketsL->addWidget(new QLabel("Tickets (issued when riding without a subscription):"));
    ticketsL->addWidget(m_ticketsList);
    tabs->addTab(ticketsW, "Tickets");

    // ---- Free Time tab ----
    auto* ftW   = new QWidget;
    auto* ftL   = new QVBoxLayout(ftW);
    ftL->addWidget(new QLabel("Check the days you are free and set your time:"));
    for (const QString& day : WEEKDAYS) {
        auto* row   = new QHBoxLayout;
        auto* check = new QCheckBox(day);
        auto* start = new QTimeEdit(QTime(8, 0));
        auto* end   = new QTimeEdit(QTime(10, 0));
        start->setDisplayFormat("HH:mm");
        end->setDisplayFormat("HH:mm");
        row->addWidget(check);
        row->addWidget(new QLabel("from"));
        row->addWidget(start);
        row->addWidget(new QLabel("to"));
        row->addWidget(end);
        ftL->addLayout(row);
        m_dayChecks.push_back(check);
        m_startEdits.push_back(start);
        m_endEdits.push_back(end);
    }
    auto* saveBtn = new QPushButton("Save Free Times");
    ftL->addWidget(saveBtn);
    connect(saveBtn, &QPushButton::clicked, this, &UserDashboard::onSaveFreeTimeClicked);
    tabs->addTab(ftW, "Free Time");

    root->addWidget(tabs);

    m_statusLabel = new QLabel;
    m_statusLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_statusLabel);
}

void UserDashboard::refresh() {
    nlohmann::json r1;
    r1["type"] = MSG_GET_ROUTES;
    m_net->sendRequest(r1);

    nlohmann::json r2;
    r2["type"] = MSG_GET_SUBS;
    m_net->sendRequest(r2);

    nlohmann::json r3;
    r3["type"] = MSG_GET_TICKETS;
    m_net->sendRequest(r3);

    nlohmann::json r4;
    r4["type"] = MSG_GET_FREETIME;
    m_net->sendRequest(r4);
}

void UserDashboard::onRefreshClicked() {
    refresh();
}

void UserDashboard::onSubscribeClicked() {
    auto* item = m_routesList->currentItem();
    if (!item) { showStatus("Select a route first.", true); return; }
    nlohmann::json req;
    req["type"]    = MSG_SUBSCRIBE;
    req["routeId"] = item->data(Qt::UserRole).toInt();
    m_net->sendRequest(req);
}

void UserDashboard::onUnsubscribeClicked() {
    auto* item = m_subsList->currentItem();
    if (!item) { showStatus("Select a subscription first.", true); return; }
    nlohmann::json req;
    req["type"]    = MSG_UNSUBSCRIBE;
    req["routeId"] = item->data(Qt::UserRole).toInt();
    m_net->sendRequest(req);
}

void UserDashboard::onReportRideClicked() {
    auto* item = m_subsList->currentItem();
    if (!item) { showStatus("Select a route first.", true); return; }
    nlohmann::json req;
    req["type"]    = MSG_RIDE;
    req["routeId"] = item->data(Qt::UserRole).toInt();
    m_net->sendRequest(req);
}

void UserDashboard::onSaveFreeTimeClicked() {
    // Build JSON without any nested brace initialization
    nlohmann::json slots = nlohmann::json::array();
    for (int i = 0; i < m_dayChecks.size(); i++) {
        if (!m_dayChecks[i]->isChecked()) continue;
        nlohmann::json slot;
        slot["day"]       = WEEKDAYS[i].toStdString();
        slot["startTime"] = m_startEdits[i]->time().toString("HH:mm").toStdString();
        slot["endTime"]   = m_endEdits[i]->time().toString("HH:mm").toStdString();
        slots.push_back(slot);
    }
    nlohmann::json req;
    req["type"]  = MSG_SET_FREETIME;
    req["slots"] = slots;
    m_net->sendRequest(req);
}

void UserDashboard::onMessageReceived(const nlohmann::json& msg) {
    std::string type = msg.value("type", "");

    if (type == MSG_ROUTES_LIST) {
        m_routesList->clear();
        for (const auto& r : msg["routes"]) {
            auto* item = new QListWidgetItem(
                QString::fromStdString(r["name"].get<std::string>())
                + "  |  " + QString::fromStdString(r["stops"].get<std::string>()));
            item->setData(Qt::UserRole, r["id"].get<int>());
            m_routesList->addItem(item);
        }

    } else if (type == MSG_SUBS_LIST) {
        m_subsList->clear();
        for (const auto& r : msg["subscriptions"]) {
            auto* item = new QListWidgetItem(
                QString::fromStdString(r["name"].get<std::string>()));
            item->setData(Qt::UserRole, r["id"].get<int>());
            m_subsList->addItem(item);
        }

    } else if (type == MSG_TICKETS) {
        // Could be a ride response OR a ticket list response
        if (msg.contains("tickets")) {
            m_ticketsList->clear();
            for (const auto& t : msg["tickets"]) {
                QString text = "Ticket #" + QString::number(t["id"].get<int>())
                             + "  Route: " + QString::number(t["routeId"].get<int>())
                             + "  At: " + QString::fromStdString(t["issuedAt"].get<std::string>());
                m_ticketsList->addItem(text);
            }
        } else if (msg.contains("message")) {
            showStatus(QString::fromStdString(msg["message"].get<std::string>()),
                       msg.value("ticketIssued", false));
            // Refresh tickets list after a ride
            nlohmann::json r;
            r["type"] = MSG_GET_TICKETS;
            m_net->sendRequest(r);
        }

    } else if (type == MSG_FREETIME) {
        // Fill in the checkboxes/times from the server data
        for (const auto& slot : msg["slots"]) {
            QString day = QString::fromStdString(slot.value("day",""));
            for (int i = 0; i < WEEKDAYS.size(); i++) {
                if (WEEKDAYS[i] == day) {
                    m_dayChecks[i]->setChecked(true);
                    m_startEdits[i]->setTime(
                        QTime::fromString(
                            QString::fromStdString(slot.value("startTime","08:00")),
                            "HH:mm"));
                    m_endEdits[i]->setTime(
                        QTime::fromString(
                            QString::fromStdString(slot.value("endTime","10:00")),
                            "HH:mm"));
                }
            }
        }

    } else if (type == MSG_OK) {
        showStatus(QString::fromStdString(msg.value("message","")), false);
        refresh();

    } else if (type == MSG_ERROR) {
        showStatus(QString::fromStdString(msg.value("message","Error")), true);
    }
}

void UserDashboard::onNetworkError(const QString& error) {
    showStatus("Network error: " + error, true);
}

void UserDashboard::showStatus(const QString& text, bool isError) {
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(isError ? "color:red;" : "color:green;");
}

} // namespace bus

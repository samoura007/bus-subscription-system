#include "UserDashboard.h"
#include "Messages.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <set>

namespace bus {

UserDashboard::UserDashboard(NetworkManager* net, int userId, const QString& username, QWidget* parent)
    : QWidget(parent), m_net(net), m_userId(userId)
{
    buildUI();
    setWindowTitle("User Dashboard - " + username);
    connect(m_net, &NetworkManager::messageReceived, this, &UserDashboard::onMessageReceived);
    connect(m_net, &NetworkManager::networkError, this, &UserDashboard::onNetworkError);
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
    connect(refreshBtn, &QPushButton::clicked, this, [this]{ refresh(); });

    m_tabs = new QTabWidget;

    // ---- Routes tab ----
    auto* routesW = new QWidget;
    auto* routesL = new QVBoxLayout(routesW);
    m_routesList  = new QListWidget;
    auto* subBtn  = new QPushButton("Subscribe to Selected");
    auto* bookBtn = new QPushButton("Book Ticket/Ride on Selected Route");
    routesL->addWidget(new QLabel("Available Routes:"));
    routesL->addWidget(m_routesList);
    routesL->addWidget(subBtn);
    routesL->addWidget(bookBtn);
    connect(subBtn,  &QPushButton::clicked, this, &UserDashboard::onSubscribeClicked);
    connect(bookBtn, &QPushButton::clicked, this, &UserDashboard::onBookRideClicked);
    m_tabs->addTab(routesW, "Routes");

    // ---- Request Time Slots tab ----
    auto* reqW = new QWidget;
    auto* reqL = new QVBoxLayout(reqW);
    auto* reqForm = new QFormLayout;
    m_reqRouteBox = new QComboBox;
    m_reqDayBox = new QComboBox;
    m_reqDayBox->addItems({"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday"});
    m_reqTimeBox = new QComboBox;
    m_reqTimeBox->addItems({"07:00 AM", "08:30 AM", "10:00 AM", "11:30 AM", "01:00 PM", "02:30 PM", "04:00 PM", "05:30 PM"});
    m_reqDestBox = new QComboBox;
    reqForm->addRow("Select Route:", m_reqRouteBox);
    reqForm->addRow("Day:", m_reqDayBox);
    reqForm->addRow("Time:", m_reqTimeBox);
    reqForm->addRow("Destination:", m_reqDestBox);
    connect(m_reqRouteBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &UserDashboard::onReqRouteSelectionChanged);
    
    auto* addReqBtn = new QPushButton("Submit Request");
    auto* delReqBtn = new QPushButton("Delete Selected Request");
    m_myRequestsList = new QListWidget;
    reqL->addWidget(new QLabel("Request specific bus times:"));
    reqL->addLayout(reqForm);
    reqL->addWidget(addReqBtn);
    reqL->addWidget(new QLabel("My Current Requests:"));
    reqL->addWidget(m_myRequestsList);
    reqL->addWidget(delReqBtn);
    connect(addReqBtn, &QPushButton::clicked, this, &UserDashboard::onAddRequestClicked);
    connect(delReqBtn, &QPushButton::clicked, this, &UserDashboard::onDeleteRequestClicked);
    m_tabs->addTab(reqW, "Request Time Slots");

    // ---- Subscriptions tab ----
    auto* subsW = new QWidget;
    auto* subsL = new QVBoxLayout(subsW);
    m_subsList  = new QListWidget;
    auto* unsubBtn = new QPushButton("Unsubscribe from Selected");
    subsL->addWidget(new QLabel("My Subscriptions:"));
    subsL->addWidget(m_subsList);
    subsL->addWidget(unsubBtn);
    connect(unsubBtn, &QPushButton::clicked, this, &UserDashboard::onUnsubscribeClicked);
    m_tabs->addTab(subsW, "My Subscriptions");

    // ---- Tickets tab ----
    auto* ticketsW = new QWidget;
    auto* ticketsL = new QVBoxLayout(ticketsW);
    m_ticketsList  = new QListWidget;
    m_totalPaymentLabel = new QLabel("Total Balance Due: 0 EGP");
    m_totalPaymentLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: blue;");
    ticketsL->addWidget(new QLabel("Tickets (issued when riding without a subscription):"));
    ticketsL->addWidget(m_ticketsList);
    ticketsL->addWidget(m_totalPaymentLabel);
    m_tabs->addTab(ticketsW, "Tickets");

    root->addWidget(m_tabs);
    m_statusLabel = new QLabel;
    m_statusLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_statusLabel);
}

void UserDashboard::refresh() {
    nlohmann::json r1; r1["type"] = MSG_GET_ROUTES;  m_net->sendRequest(r1);
    nlohmann::json r2; r2["type"] = MSG_GET_SUBS;    m_net->sendRequest(r2);
    nlohmann::json r3; r3["type"] = MSG_GET_TICKETS; m_net->sendRequest(r3);
    nlohmann::json r4; r4["type"] = MSG_GET_VOTES;   m_net->sendRequest(r4);
}

void UserDashboard::onReqRouteSelectionChanged() {
    m_reqDestBox->clear();
    if (m_reqRouteBox->count() == 0) return;
    
    m_reqDestBox->addItem("AUC");
    
    QString routeName = m_reqRouteBox->currentText();
    int arrowIdx = routeName.indexOf("->");
    if (arrowIdx != -1) {
        QString dest = routeName.mid(arrowIdx + 2).trimmed();
        m_reqDestBox->addItem(dest);
    } else {
        QString dest = routeName;
        dest.remove("AUC");
        dest = dest.trimmed(); // Fixed warning: capture return value
        if (dest.isEmpty()) dest = "Other Stop";
        m_reqDestBox->addItem(dest);
    }
}

void UserDashboard::onSubscribeClicked() {
    auto* item = m_routesList->currentItem();
    if (!item) { showStatus("Select a route.", true); return; }
    nlohmann::json req; req["type"] = MSG_SUBSCRIBE; req["routeId"] = item->data(Qt::UserRole).toInt();
    m_net->sendRequest(req);
}

void UserDashboard::onUnsubscribeClicked() {
    auto* item = m_subsList->currentItem();
    if (!item) { showStatus("Select a subscription.", true); return; }
    nlohmann::json req; req["type"] = MSG_UNSUBSCRIBE; req["routeId"] = item->data(Qt::UserRole).toInt();
    m_net->sendRequest(req);
}

void UserDashboard::onBookRideClicked() {
    auto* item = m_routesList->currentItem();
    if (!item) { showStatus("Select a route to ride.", true); return; }
    nlohmann::json req; req["type"] = MSG_RIDE; req["routeId"] = item->data(Qt::UserRole).toInt();
    m_net->sendRequest(req);
}

void UserDashboard::onAddRequestClicked() {
    int idx = m_reqRouteBox->currentIndex();
    if (idx < 0) return;
    int rid = m_reqRouteBox->itemData(idx).toInt();

    nlohmann::json v;
    v["routeId"] = rid;
    v["day"] = m_reqDayBox->currentText().toStdString();
    v["timeSlot"] = m_reqTimeBox->currentText().toStdString();
    v["destination"] = m_reqDestBox->currentText().toStdString();

    m_myVotes.push_back(v);

    nlohmann::json req;
    req["type"] = MSG_SET_VOTES;
    req["votes"] = m_myVotes;
    m_net->sendRequest(req);
}

void UserDashboard::onDeleteRequestClicked() {
    auto* item = m_myRequestsList->currentItem();
    if (!item) return;
    int row = m_myRequestsList->row(item);
    if (row >= 0 && (size_t)row < m_myVotes.size()) {
        m_myVotes.erase(m_myVotes.begin() + row);
        nlohmann::json req;
        req["type"] = MSG_SET_VOTES;
        req["votes"] = m_myVotes;
        m_net->sendRequest(req);
    }
}

void UserDashboard::onMessageReceived(const nlohmann::json& msg) {
    std::string type = msg.value("type", "");

    if (type == MSG_ROUTES_LIST) {
        if (msg.contains("routes")) {
            m_routesList->clear();
            m_reqRouteBox->clear();
            for (const auto& r : msg["routes"]) {
                QString name = QString::fromStdString(r.value("name", "Unknown"));
                QString stops = QString::fromStdString(r.value("stops", "No stops"));
                int rid = r.value("id", 0);
                
                auto* item = new QListWidgetItem(name + " | " + stops);
                item->setData(Qt::UserRole, rid);
                m_routesList->addItem(item);
                m_reqRouteBox->addItem(name, rid);
            }
        }
    } else if (type == MSG_SUBS_LIST) {
        if (msg.contains("subs")) {
            m_subsList->clear();
            std::set<int> subIds;
            for (int rid : msg["subs"]) subIds.insert(rid);

            for (int i = 0; i < m_routesList->count(); ++i) {
                auto* item = m_routesList->item(i);
                int rid = item->data(Qt::UserRole).toInt();
                if (subIds.count(rid)) {
                    m_subsList->addItem(item->text());
                }
            }
        }
    } else if (type == MSG_VOTES_LIST) {
        m_myRequestsList->clear();
        m_myVotes.clear(); // Clear local cache to sync with server
        for (const auto& v : msg.value("votes", nlohmann::json::array())) {
            m_myVotes.push_back(v);
            int rid = v.value("routeId", 0);
            QString text = "Route ID " + QString::number(rid) + " | " +
                           QString::fromStdString(v.value("day", "")) + " | " +
                           QString::fromStdString(v.value("timeSlot", "")) + " | " +
                           QString::fromStdString(v.value("destination", ""));
            m_myRequestsList->addItem(text);
        }
    } else if (type == MSG_TICKETS) {
        if (msg.contains("tickets")) {
            m_ticketsList->clear();
            int overallPrice = 0;
            for (const auto& t : msg["tickets"]) {
                int price = t.value("price", 0);
                overallPrice += price;
                QString text = QString("Ticket #%1 | Route #%2 | Issued: %3 | Price: %4 EGP")
                    .arg(t.value("id", 0))
                    .arg(t.value("routeId", 0))
                    .arg(QString::fromStdString(t.value("issuedAt", "N/A")))
                    .arg(price);
                m_ticketsList->addItem(text);
            }
            m_totalPaymentLabel->setText(QString("Total Balance Due: %1 EGP").arg(overallPrice));
        }
    } else if (type == MSG_OK) {
        if (msg.contains("message")) {
            showStatus(QString::fromStdString(msg["message"].get<std::string>()), msg.value("ticketIssued", false));
        }
        refresh();
    } else if (type == MSG_ERROR) {
        showStatus(QString::fromStdString(msg.value("message", "Error")), true);
    }
}

void UserDashboard::onNetworkError(const QString& error) {
    showStatus("Network error: " + error, true);
}

void UserDashboard::showStatus(const QString& text, bool isError) {
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(isError ? "color:red; font-weight:bold;" : "color:green;");
}

} // namespace bus

#include "UserDashboard.h"
#include "Messages.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

namespace bus {

static const QStringList WEEKDAYS = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday"};
static const QStringList DIRECTIONS = {"To Uni", "From Uni"};

UserDashboard::UserDashboard(NetworkManager* net, int userId, const QString& username, QWidget* parent)
    : QWidget(parent), m_net(net), m_userId(userId)
{
    buildUI();
    setWindowTitle("Welcome, " + username);
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
    connect(refreshBtn, &QPushButton::clicked, this, &UserDashboard::onRefreshClicked);

    auto* tabs = new QTabWidget;

    // ---- Routes tab ----
    auto* routesW  = new QWidget;
    auto* routesL  = new QVBoxLayout(routesW);
    m_routesList   = new QListWidget;
    auto* subBtn   = new QPushButton("Subscribe to Selected Route");
    auto* bookBtn = new QPushButton("Book Ticket/Ride on Selected Route");
    routesL->addWidget(new QLabel("Available Routes:"));
    routesL->addWidget(m_routesList);
    routesL->addWidget(subBtn);
    routesL->addWidget(bookBtn);
    connect(subBtn, &QPushButton::clicked, this, &UserDashboard::onSubscribeClicked);
    connect(bookBtn, &QPushButton::clicked, this, &UserDashboard::onBookRideClicked);
    tabs->addTab(routesW, "Routes");

    // ---- Subscriptions tab ----
    auto* subsW    = new QWidget;
    auto* subsL    = new QVBoxLayout(subsW);
    m_subsList     = new QListWidget;
    auto* unsubBtn = new QPushButton("Unsubscribe from Selected");
    subsL->addWidget(new QLabel("My Subscriptions:"));
    subsL->addWidget(m_subsList);
    subsL->addWidget(unsubBtn);
    connect(unsubBtn, &QPushButton::clicked, this, &UserDashboard::onUnsubscribeClicked);
    tabs->addTab(subsW, "My Subscriptions");

    // ---- Tickets tab ----
    auto* ticketsW = new QWidget;
    auto* ticketsL = new QVBoxLayout(ticketsW);
    m_ticketsList  = new QListWidget;
    ticketsL->addWidget(new QLabel("Tickets (issued when riding without a subscription):"));
    ticketsL->addWidget(m_ticketsList);
    tabs->addTab(ticketsW, "Tickets");

    // ---- Vote Timings tab ----
    auto* voteW = new QWidget;
    auto* voteL = new QVBoxLayout(voteW);
    auto* form  = new QFormLayout;
    
    m_voteRouteBox = new QComboBox;
    m_voteDayBox   = new QComboBox;
    m_voteDayBox->addItems(WEEKDAYS);
    m_voteDirBox   = new QComboBox;
    m_voteDirBox->addItems(DIRECTIONS);
    m_voteTimeBox  = new QComboBox;
    
    form->addRow("Subscribed Route:", m_voteRouteBox);
    form->addRow("Day:", m_voteDayBox);
    form->addRow("Direction:", m_voteDirBox);
    form->addRow("Time Slot:", m_voteTimeBox);
    
    connect(m_voteRouteBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &UserDashboard::onRouteSelectionChanged);
    
    auto* addVoteBtn = new QPushButton("Add to My Votes");
    m_myVotesList = new QListWidget;
    auto* saveVotesBtn = new QPushButton("Commit All Votes to Server");
    
    voteL->addLayout(form);
    voteL->addWidget(addVoteBtn);
    voteL->addWidget(new QLabel("My Prepared Votes:"));
    voteL->addWidget(m_myVotesList);
    voteL->addWidget(saveVotesBtn);
    
    connect(addVoteBtn, &QPushButton::clicked, this, &UserDashboard::onAddVoteClicked);
    connect(saveVotesBtn, &QPushButton::clicked, this, &UserDashboard::onSaveVotesClicked);
    
    tabs->addTab(voteW, "Vote Timings");
    root->addWidget(tabs);

    m_statusLabel = new QLabel;
    m_statusLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_statusLabel);
}

void UserDashboard::refresh() {
    nlohmann::json r1; r1["type"] = MSG_GET_ROUTES; m_net->sendRequest(r1);
    nlohmann::json r2; r2["type"] = MSG_GET_SUBS; m_net->sendRequest(r2);
    nlohmann::json r3; r3["type"] = MSG_GET_TICKETS; m_net->sendRequest(r3);
    nlohmann::json r4; r4["type"] = MSG_GET_VOTES; m_net->sendRequest(r4);
}

void UserDashboard::onRefreshClicked() { refresh(); }

void UserDashboard::onSubscribeClicked() {
    auto* item = m_routesList->currentItem();
    if (!item) { showStatus("Select a route first.", true); return; }
    nlohmann::json req; req["type"] = MSG_SUBSCRIBE; req["routeId"] = item->data(Qt::UserRole).toInt();
    m_net->sendRequest(req);
}

void UserDashboard::onUnsubscribeClicked() {
    auto* item = m_subsList->currentItem();
    if (!item) { showStatus("Select a subscription first.", true); return; }
    nlohmann::json req; req["type"] = MSG_UNSUBSCRIBE; req["routeId"] = item->data(Qt::UserRole).toInt();
    m_net->sendRequest(req);
}

void UserDashboard::onBookRideClicked() {
    auto* item = m_routesList->currentItem();
    if (!item) { showStatus("Select a route first.", true); return; }
    nlohmann::json req; req["type"] = MSG_RIDE; req["routeId"] = item->data(Qt::UserRole).toInt();
    m_net->sendRequest(req);
}

void UserDashboard::onRouteSelectionChanged(int index) {
    m_voteTimeBox->clear();
    if (index < 0) return;
    int routeId = m_voteRouteBox->itemData(index).toInt();
    QString sched = m_routeSchedules[routeId];
    for (const QString& t : sched.split(",", Qt::SkipEmptyParts)) {
        m_voteTimeBox->addItem(t.trimmed());
    }
}

void UserDashboard::onAddVoteClicked() {
    if (m_voteRouteBox->count() == 0 || m_voteTimeBox->count() == 0) return;
    int routeId = m_voteRouteBox->currentData().toInt();
    QString text = m_voteRouteBox->currentText() + " | " + m_voteDayBox->currentText() + " | " + m_voteDirBox->currentText() + " | " + m_voteTimeBox->currentText();
    
    // Check duplicates
    for(int i = 0; i < m_myVotesList->count(); i++) {
        if(m_myVotesList->item(i)->text() == text) return;
    }
    
    auto* item = new QListWidgetItem(text);
    item->setData(Qt::UserRole, routeId);
    item->setData(Qt::UserRole + 1, m_voteDayBox->currentText());
    item->setData(Qt::UserRole + 2, m_voteTimeBox->currentText());
    item->setData(Qt::UserRole + 3, m_voteDirBox->currentText());
    m_myVotesList->addItem(item);
}

void UserDashboard::onSaveVotesClicked() {
    nlohmann::json votesArray = nlohmann::json::array();
    for (int i = 0; i < m_myVotesList->count(); i++) {
        auto* item = m_myVotesList->item(i);
        nlohmann::json vote;
        vote["routeId"]   = item->data(Qt::UserRole).toInt();
        vote["day"]       = item->data(Qt::UserRole + 1).toString().toStdString();
        vote["timeSlot"]  = item->data(Qt::UserRole + 2).toString().toStdString();
        vote["direction"] = item->data(Qt::UserRole + 3).toString().toStdString();
        votesArray.push_back(vote);
    }
    nlohmann::json req; req["type"] = MSG_SET_VOTES; req["votes"] = votesArray;
    m_net->sendRequest(req);
}

void UserDashboard::onMessageReceived(const nlohmann::json& msg) {
    std::string type = msg.value("type", "");
    if (type == MSG_ROUTES_LIST) {
        m_routesList->clear();
        for (const auto& r : msg["routes"]) {
            auto* item = new QListWidgetItem(QString::fromStdString(r["name"].get<std::string>()) + "  |  " + QString::fromStdString(r["stops"].get<std::string>()));
            item->setData(Qt::UserRole, r["id"].get<int>());
            m_routesList->addItem(item);
        }
    } else if (type == MSG_SUBS_LIST) {
        m_subsList->clear();
        m_voteRouteBox->clear();
        m_routeSchedules.clear();
        for (const auto& r : msg["subscriptions"]) {
            int rId = r["id"].get<int>();
            QString name = QString::fromStdString(r["name"].get<std::string>());
            QString sched = QString::fromStdString(r["schedule"].get<std::string>());
            m_routeSchedules[rId] = sched;
            
            auto* item = new QListWidgetItem(name);
            item->setData(Qt::UserRole, rId);
            m_subsList->addItem(item);
            m_voteRouteBox->addItem(name, rId);
        }
    } else if (type == MSG_TICKETS) {
        if (msg.contains("tickets")) {
            m_ticketsList->clear();
            for (const auto& t : msg["tickets"]) {
                QString chargeStatus = t.value("charged", false) ? "[Charged]" : "[Free]";
                int price = t.value("price", 0);
                QString text = "Ticket #" + QString::number(t["id"].get<int>()) + "  Route: " + QString::number(t["routeId"].get<int>()) + "  At: " + QString::fromStdString(t["issuedAt"].get<std::string>()) + " " + chargeStatus + " | Price: " + QString::number(price) + " EGP";
                m_ticketsList->addItem(text);
            }
        } else if (msg.contains("message")) {
            showStatus(QString::fromStdString(msg["message"].get<std::string>()), msg.value("ticketIssued", false));
            nlohmann::json r; r["type"] = MSG_GET_TICKETS; m_net->sendRequest(r);
        }
    } else if (type == MSG_VOTES_LIST) {
        m_myVotesList->clear();
        for (const auto& v : msg["votes"]) {
            int routeId = v.value("routeId", 0);
            QString day = QString::fromStdString(v.value("day",""));
            QString timeSlot = QString::fromStdString(v.value("timeSlot",""));
            QString dir = QString::fromStdString(v.value("direction",""));
            QString text = "Route " + QString::number(routeId) + " | " + day + " | " + dir + " | " + timeSlot;
            auto* item = new QListWidgetItem(text);
            item->setData(Qt::UserRole, routeId);
            item->setData(Qt::UserRole + 1, day);
            item->setData(Qt::UserRole + 2, timeSlot);
            item->setData(Qt::UserRole + 3, dir);
            m_myVotesList->addItem(item);
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

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
    form->addRow("Route Name:", m_routeNameEdit);
    form->addRow("Stops:",      m_routeStopsEdit);
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

    // ---- Manage Schedules & View Demand tab ----
    auto* scheduleW = new QWidget;
    auto* scheduleSplit = new QSplitter(Qt::Horizontal, scheduleW);
    auto* schedMainL = new QVBoxLayout(scheduleW);
    schedMainL->addWidget(scheduleSplit);

    auto* editorGroup = new QGroupBox("Schedule Editor");
    auto* scheduleL = new QVBoxLayout(editorGroup);
    m_scheduleRouteBox = new QComboBox;
    m_tripsList = new QListWidget;
    auto* tripForm = new QFormLayout;
    m_addTripTimeEdit = new QLineEdit;
    m_addTripTimeEdit->setPlaceholderText("e.g. 05:00 PM");
    m_addTripDestEdit = new QLineEdit;
    m_addTripDestEdit->setPlaceholderText("e.g. Nasr City");
    tripForm->addRow("Time:", m_addTripTimeEdit);
    tripForm->addRow("Destination:", m_addTripDestEdit);
    auto* addTripBtn = new QPushButton("Add Trip");
    auto* delTripBtn = new QPushButton("Delete Selected Trip");

    scheduleL->addWidget(new QLabel("Select Route:"));
    scheduleL->addWidget(m_scheduleRouteBox);
    scheduleL->addWidget(new QLabel("Schedule (Trips):"));
    scheduleL->addWidget(m_tripsList);
    scheduleL->addLayout(tripForm);
    scheduleL->addWidget(addTripBtn);
    scheduleL->addWidget(delTripBtn);

    auto* demandGroup = new QGroupBox("Aggregated User Requests");
    auto* demandL = new QVBoxLayout(demandGroup);
    m_demandList = new QListWidget;
    demandL->addWidget(m_demandList);

    scheduleSplit->addWidget(editorGroup);
    scheduleSplit->addWidget(demandGroup);

    connect(m_scheduleRouteBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AdminDashboard::onRouteSelectionChanged);
    connect(addTripBtn, &QPushButton::clicked, this, &AdminDashboard::onAddTripClicked);
    connect(delTripBtn, &QPushButton::clicked, this, &AdminDashboard::onDeleteTripClicked);
    tabs->addTab(scheduleW, "Manage Schedules");

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
    connect(m_usersList, &QListWidget::itemDoubleClicked, this, &AdminDashboard::onUserSelected);

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
    req["schedule"] = ""; 
    m_net->sendRequest(req);
    m_routeNameEdit->clear(); m_routeStopsEdit->clear();
}

void AdminDashboard::onDeleteRouteClicked() {
    auto* item = m_routesList->currentItem();
    if (!item) { showStatus("Select a route to delete.", true); return; }
    nlohmann::json req; req["type"] = MSG_DEL_ROUTE; req["routeId"] = item->data(Qt::UserRole).toInt();
    m_net->sendRequest(req);
}

void AdminDashboard::onRouteSelectionChanged() {
    m_tripsList->clear();
    int idx = m_scheduleRouteBox->currentIndex();
    if (idx < 0) return;
    int rid = m_scheduleRouteBox->itemData(idx).toInt();
    if (m_routesData.count(rid)) {
        auto arr = m_routesData[rid];
        for (size_t i = 0; i < arr.size(); ++i) {
            auto t = arr[i];
            QString text = QString::fromStdString(t.value("time", "")) + " -> " + QString::fromStdString(t.value("destination", ""));
            auto* item = new QListWidgetItem(text);
            item->setData(Qt::UserRole, static_cast<int>(i));
            m_tripsList->addItem(item);
        }
    }
}

void AdminDashboard::onAddTripClicked() {
    int idx = m_scheduleRouteBox->currentIndex();
    if (idx < 0) return;
    int rid = m_scheduleRouteBox->itemData(idx).toInt();
    
    std::string t = m_addTripTimeEdit->text().toStdString();
    std::string d = m_addTripDestEdit->text().toStdString();
    if (t.empty() || d.empty()) { showStatus("Time and Destination required.", true); return; }
    
    nlohmann::json trip;
    trip["time"] = t;
    trip["destination"] = d;
    m_routesData[rid].push_back(trip);
    
    nlohmann::json req;
    req["type"] = MSG_UPDATE_SCHEDULE;
    req["routeId"] = rid;
    req["schedule"] = m_routesData[rid].dump();
    m_net->sendRequest(req);
    
    m_addTripTimeEdit->clear();
    m_addTripDestEdit->clear();
}

void AdminDashboard::onDeleteTripClicked() {
    auto* item = m_tripsList->currentItem();
    if (!item) return;
    int tripIdx = item->data(Qt::UserRole).toInt();
    
    int idx = m_scheduleRouteBox->currentIndex();
    if (idx < 0) return;
    int rid = m_scheduleRouteBox->itemData(idx).toInt();
    
    m_routesData[rid].erase(tripIdx);
    
    nlohmann::json req;
    req["type"] = MSG_UPDATE_SCHEDULE;
    req["routeId"] = rid;
    req["schedule"] = m_routesData[rid].dump();
    m_net->sendRequest(req);
}

void AdminDashboard::onMessageReceived(const nlohmann::json& msg) {
    std::string type = msg.value("type", "");

    if (type == MSG_ROUTES_LIST) {
        m_routesList->clear();
        m_scheduleRouteBox->blockSignals(true);
        m_scheduleRouteBox->clear();
        m_routesData.clear();
        
        for (const auto& r : msg["routes"]) {
            int rid = r["id"].get<int>();
            QString qname = QString::fromStdString(r["name"].get<std::string>());
            
            auto* item = new QListWidgetItem(qname);
            item->setData(Qt::UserRole, rid);
            m_routesList->addItem(item);
            
            m_scheduleRouteBox->addItem(qname, rid);
            std::string schedStr = r["schedule"].get<std::string>();
            try {
                m_routesData[rid] = nlohmann::json::parse(schedStr);
            } catch(...) {
                m_routesData[rid] = nlohmann::json::array();
            }
        }
        m_scheduleRouteBox->blockSignals(false);
        onRouteSelectionChanged();
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
            std::string key = d["key"].get<std::string>();
            int count = d["count"].get<int>();
            QStringList parts = QString::fromStdString(key).split('|');
            if (parts.size() == 4) {
                int rid = parts[0].toInt();
                QString rname = "Route " + parts[0];
                for(int i=0; i<m_scheduleRouteBox->count(); ++i) {
                    if (m_scheduleRouteBox->itemData(i).toInt() == rid) {
                        rname = m_scheduleRouteBox->itemText(i); break;
                    }
                }
                QString text = QString("[%1] %2 | %3 | %4  --> %5 requests")
                               .arg(rname).arg(parts[1]).arg(parts[2]).arg(parts[3]).arg(count);
                m_demandList->addItem(text);
            }
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
    m_tabs->setCurrentIndex(2);
}

void AdminDashboard::onMessageUserDetail(const nlohmann::json& msg) {
    QString username = QString::fromStdString(msg["user"]["username"].get<std::string>());
    m_userDetailNameLabel->setText("Details for: " + username);

    m_userDetailSubsList->clear();
    for (const auto& r : msg["subscriptions"]) {
        QString text = QString("Route #%1: %2  |  Stops: %3")
            .arg(r["id"].get<int>())
            .arg(QString::fromStdString(r["name"].get<std::string>()))
            .arg(QString::fromStdString(r["stops"].get<std::string>()));
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
}

} // namespace bus

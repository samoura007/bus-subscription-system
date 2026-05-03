#pragma once
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QComboBox>
#include <map>
#include "../network/NetworkManager.h"

namespace bus {

class AdminDashboard : public QWidget {
    Q_OBJECT
public:
    explicit AdminDashboard(NetworkManager* net, int userId, const QString& username, QWidget* parent = nullptr);
    void refresh();

signals:
    void logoutRequested();

private slots:
    void onAddRouteClicked();
    void onDeleteRouteClicked();
    void onAddTripClicked();
    void onDeleteTripClicked();
    void onRouteSelectionChanged();
    void onMessageReceived(const nlohmann::json& msg);
    void onNetworkError(const QString& error);
    void onUserSelected(QListWidgetItem* item);
    void onMessageUserDetail(const nlohmann::json& msg);

private:
    void buildUI();
    void showStatus(const QString& text, bool isError = false);

    NetworkManager* m_net;
    int             m_userId;
    QListWidget* m_routesList;
    QLineEdit* m_routeNameEdit;
    QLineEdit* m_routeStopsEdit;
    QListWidget* m_usersList;
    QTabWidget* m_tabs;
    QLabel* m_statusLabel;
    int             m_selectedUserId = 0;

    // Schedule Management UI
    QComboBox* m_scheduleRouteBox;
    QListWidget* m_tripsList;
    QLineEdit* m_addTripTimeEdit;
    QLineEdit* m_addTripDestEdit;
    QListWidget* m_demandList; // To show user requests
    std::map<int, nlohmann::json> m_routesData;

    // User details UI
    QLabel* m_userDetailNameLabel;
    QListWidget* m_userDetailSubsList;
    QListWidget* m_userDetailTicketsList;
};

} // namespace bus

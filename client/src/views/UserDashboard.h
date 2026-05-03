#pragma once
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QComboBox>
#include <vector>
#include <nlohmann/json.hpp>
#include "../network/NetworkManager.h"

namespace bus {

class UserDashboard : public QWidget {
    Q_OBJECT
public:
    explicit UserDashboard(NetworkManager* net, int userId, const QString& username, QWidget* parent = nullptr);
    void refresh();

signals:
    void logoutRequested();

private slots:
    void onSubscribeClicked();
    void onUnsubscribeClicked();
    void onBookRideClicked();
    void onAddRequestClicked();
    void onDeleteRequestClicked();
    void onReqRouteSelectionChanged(); // NEW: Slot to update Destination combobox
    void onMessageReceived(const nlohmann::json& msg);
    void onNetworkError(const QString& error);

private:
    void buildUI();
    void showStatus(const QString& text, bool isError = false);

    NetworkManager* m_net;
    int             m_userId;
    QListWidget* m_routesList;
    QListWidget* m_subsList;
    QListWidget* m_ticketsList;
    QLabel* m_statusLabel;
    QTabWidget* m_tabs;

    // Time Slot Requests
    QComboBox* m_reqRouteBox;
    QComboBox* m_reqDayBox;
    QComboBox* m_reqTimeBox;
    QComboBox* m_reqDestBox; // CHANGED: Replaced DirBox with DestBox
    QListWidget* m_myRequestsList;
    std::vector<nlohmann::json> m_myVotes;
};

} // namespace bus

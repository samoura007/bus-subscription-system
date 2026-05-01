#pragma once
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QComboBox>
#include <map>
#include "../network/NetworkManager.h"

namespace bus {

class UserDashboard : public QWidget {
    Q_OBJECT
public:
    explicit UserDashboard(NetworkManager* net,
                           int userId,
                           const QString& username,
                           QWidget* parent = nullptr);
    void refresh();

signals:
    void logoutRequested();

private slots:
    void onSubscribeClicked();
    void onUnsubscribeClicked();
    void onBookRideClicked();
    void onAddVoteClicked();
    void onSaveVotesClicked();
    void onRouteSelectionChanged(int index);
    void onRefreshClicked();
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
    QLabel*      m_statusLabel;

    // Vote tab widgets
    QComboBox*   m_voteRouteBox;
    QComboBox*   m_voteDayBox;
    QComboBox*   m_voteDirBox;
    QComboBox*   m_voteTimeBox;
    QListWidget* m_myVotesList;
    
    // Store schedules to populate time combobox
    std::map<int, QString> m_routeSchedules;
};

} // namespace bus

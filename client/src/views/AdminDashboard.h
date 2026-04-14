#pragma once
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include "../network/NetworkManager.h"

namespace bus {

class AdminDashboard : public QWidget {
    Q_OBJECT
public:
    explicit AdminDashboard(NetworkManager* net,
                            int userId,
                            const QString& username,
                            QWidget* parent = nullptr);
    void refresh();

signals:
    void logoutRequested();

private slots:
    void onAddRouteClicked();
    void onDeleteRouteClicked();
    void onMessageReceived(const nlohmann::json& msg);
    void onNetworkError(const QString& error);

private:
    void buildUI();
    void showStatus(const QString& text, bool isError = false);

    NetworkManager* m_net;
    int             m_userId;

    QListWidget* m_routesList;
    QListWidget* m_usersList;
    QLineEdit*   m_routeNameEdit;
    QLineEdit*   m_routeStopsEdit;
    QLineEdit*   m_routeScheduleEdit;
    QLabel*      m_statusLabel;
};

} // namespace bus

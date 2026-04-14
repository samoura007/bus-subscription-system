#pragma once
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QCheckBox>
#include <QTimeEdit>
#include <QVector>
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
    void onReportRideClicked();
    void onSaveFreeTimeClicked();
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

    // Free Time tab widgets (parallel vectors, one per weekday)
    QVector<QCheckBox*> m_dayChecks;
    QVector<QTimeEdit*> m_startEdits;
    QVector<QTimeEdit*> m_endEdits;
};

} // namespace bus

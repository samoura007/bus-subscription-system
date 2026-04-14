#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../network/NetworkManager.h"

namespace bus {

class LoginView : public QWidget {
    Q_OBJECT
public:
    explicit LoginView(NetworkManager* net, QWidget* parent = nullptr);

signals:
    void loginSuccess(int userId, const QString& username, const QString& role);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onConnected();
    void onDisconnected();
    void onMessageReceived(const nlohmann::json& msg);
    void onNetworkError(const QString& error);

private:
    void buildUI();
    void showStatus(const QString& text, bool isError = true);

    NetworkManager* m_net;
    QLineEdit*  m_loginUser;
    QLineEdit*  m_loginPass;
    QPushButton* m_loginBtn;
    QLineEdit*  m_regUser;
    QLineEdit*  m_regPass;
    QLineEdit*  m_regConfirm;
    QPushButton* m_registerBtn;
    QLabel*     m_statusLabel;
};

} // namespace bus

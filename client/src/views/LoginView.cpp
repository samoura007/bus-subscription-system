#include "LoginView.h"
#include "Messages.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>

namespace bus {

LoginView::LoginView(NetworkManager* net, QWidget* parent)
    : QWidget(parent), m_net(net)
{
    buildUI();
    connect(m_net, &NetworkManager::connected,
            this,  &LoginView::onConnected);
    connect(m_net, &NetworkManager::disconnected,
            this,  &LoginView::onDisconnected);
    connect(m_net, &NetworkManager::messageReceived,
            this,  &LoginView::onMessageReceived);
    connect(m_net, &NetworkManager::networkError,
            this,  &LoginView::onNetworkError);

    m_net->connectToServer("127.0.0.1", 9000);
}

void LoginView::buildUI() {
    auto* root  = new QVBoxLayout(this);
    auto* title = new QLabel("<h2>AUC Bus Subscription System</h2>");
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    auto* tabs = new QTabWidget(this);

    // ----- Login tab -----
    auto* loginWidget = new QWidget;
    auto* loginForm   = new QFormLayout(loginWidget);
    m_loginUser = new QLineEdit;
    m_loginPass = new QLineEdit;
    m_loginPass->setEchoMode(QLineEdit::Password);
    m_loginBtn  = new QPushButton("Login");
    loginForm->addRow("Username:", m_loginUser);
    loginForm->addRow("Password:", m_loginPass);
    loginForm->addRow(m_loginBtn);
    tabs->addTab(loginWidget, "Login");

    // ----- Register tab -----
    auto* regWidget = new QWidget;
    auto* regForm   = new QFormLayout(regWidget);
    m_regUser    = new QLineEdit;
    m_regPass    = new QLineEdit;
    m_regPass->setEchoMode(QLineEdit::Password);
    m_regConfirm = new QLineEdit;
    m_regConfirm->setEchoMode(QLineEdit::Password);
    m_registerBtn = new QPushButton("Create Account");
    regForm->addRow("Username:", m_regUser);
    regForm->addRow("Password:", m_regPass);
    regForm->addRow("Confirm:",  m_regConfirm);
    regForm->addRow(m_registerBtn);
    tabs->addTab(regWidget, "Register");

    root->addWidget(tabs);

    m_statusLabel = new QLabel;
    m_statusLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_statusLabel);

    connect(m_loginBtn,    &QPushButton::clicked, this, &LoginView::onLoginClicked);
    connect(m_registerBtn, &QPushButton::clicked, this, &LoginView::onRegisterClicked);
}

void LoginView::onConnected() {
    showStatus("Connected to server.", false);
}

void LoginView::onDisconnected() {
    showStatus("Disconnected. Reconnecting...", true);
    m_net->connectToServer("127.0.0.1", 9000);
}

void LoginView::onLoginClicked() {
    nlohmann::json req;
    req["type"]     = MSG_LOGIN;
    req["username"] = m_loginUser->text().toStdString();
    req["password"] = m_loginPass->text().toStdString();
    m_net->sendRequest(req);
}

void LoginView::onRegisterClicked() {
    if (m_regPass->text() != m_regConfirm->text()) {
        showStatus("Passwords do not match.", true);
        return;
    }
    nlohmann::json req;
    req["type"]     = MSG_REGISTER;
    req["username"] = m_regUser->text().toStdString();
    req["password"] = m_regPass->text().toStdString();
    m_net->sendRequest(req);
}

void LoginView::onMessageReceived(const nlohmann::json& msg) {
    std::string type = msg.value("type", "");

    if (type == MSG_AUTH_OK) {
        int     id       = msg["user"]["id"].get<int>();
        QString username = QString::fromStdString(msg["user"]["username"].get<std::string>());
        QString role     = QString::fromStdString(msg["user"]["role"].get<std::string>());
        emit loginSuccess(id, username, role);

    } else if (type == MSG_ERROR) {
        showStatus(QString::fromStdString(msg.value("message","Unknown error")), true);
    }
}

void LoginView::onNetworkError(const QString& error) {
    showStatus("Network error: " + error, true);
}

void LoginView::showStatus(const QString& text, bool isError) {
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(isError ? "color:red;" : "color:green;");
}

} // namespace bus

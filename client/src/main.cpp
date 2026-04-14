#include "network/NetworkManager.h"
#include "views/LoginView.h"
#include "views/UserDashboard.h"
#include "views/AdminDashboard.h"
#include <QApplication>
#include <QMainWindow>
#include <QStackedWidget>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("AUC Bus Subscription System");

    bus::NetworkManager netManager;

    QMainWindow window;
    window.setMinimumSize(800, 600);
    auto* stack = new QStackedWidget;
    window.setCentralWidget(stack);

    auto* loginView = new bus::LoginView(&netManager);
    stack->addWidget(loginView);

    QObject::connect(loginView, &bus::LoginView::loginSuccess,
        [&](int userId, const QString& username, const QString& role) {
            QWidget* dashboard = nullptr;
            if (role == "admin") {
                auto* admin = new bus::AdminDashboard(&netManager, userId, username);
                QObject::connect(admin, &bus::AdminDashboard::logoutRequested,
                                 [&]{ stack->setCurrentIndex(0); });
                admin->refresh();
                dashboard = admin;
            } else {
                auto* user = new bus::UserDashboard(&netManager, userId, username);
                QObject::connect(user, &bus::UserDashboard::logoutRequested,
                                 [&]{ stack->setCurrentIndex(0); });
                user->refresh();
                dashboard = user;
            }
            int idx = stack->addWidget(dashboard);
            stack->setCurrentIndex(idx);
            window.setWindowTitle("AUC Bus - " + username);
        });

    window.show();
    return app.exec();
}

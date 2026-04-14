#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <nlohmann/json.hpp>

namespace bus {

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(QObject* parent = nullptr);
    void connectToServer(const QString& host, quint16 port);
    void sendRequest(const nlohmann::json& request);
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void messageReceived(const nlohmann::json& msg);
    void networkError(const QString& error);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError err);

private:
    void tryProcessBuffer();
    QTcpSocket* m_socket;
    QByteArray  m_buffer;
    QString     m_host;
    quint16     m_port = 9000;
};

} // namespace bus

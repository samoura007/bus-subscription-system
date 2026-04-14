#include "NetworkManager.h"
#include <QAbstractSocket>

namespace bus {

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent), m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected,
            this, &NetworkManager::onConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &NetworkManager::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &NetworkManager::onReadyRead);
    connect(m_socket, &QAbstractSocket::errorOccurred,
            this, &NetworkManager::onError);
}

void NetworkManager::connectToServer(const QString& host, quint16 port) {
    m_host = host;
    m_port = port;
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        m_socket->waitForDisconnected(1000);
    }
    m_socket->connectToHost(host, port);
}

bool NetworkManager::isConnected() const {
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkManager::sendRequest(const nlohmann::json& request) {
    // If still connecting, wait up to 3 seconds
    if (m_socket->state() == QAbstractSocket::ConnectingState) {
        m_socket->waitForConnected(3000);
    }
    if (!isConnected()) {
        emit networkError("Not connected to server. Please wait and try again.");
        return;
    }

    std::string body = request.dump();
    uint32_t len = static_cast<uint32_t>(body.size());

    QByteArray frame;
    frame.append(static_cast<char>((len >> 24) & 0xFF));
    frame.append(static_cast<char>((len >> 16) & 0xFF));
    frame.append(static_cast<char>((len >>  8) & 0xFF));
    frame.append(static_cast<char>((len >>  0) & 0xFF));
    frame.append(body.c_str(), static_cast<qsizetype>(body.size()));

    m_socket->write(frame);
    m_socket->flush();
}

void NetworkManager::onConnected() {
    emit connected();
}

void NetworkManager::onDisconnected() {
    m_buffer.clear();
    emit disconnected();
}

void NetworkManager::onError(QAbstractSocket::SocketError /*err*/) {
    emit networkError(m_socket->errorString());
}

void NetworkManager::onReadyRead() {
    m_buffer.append(m_socket->readAll());
    tryProcessBuffer();
}

void NetworkManager::tryProcessBuffer() {
    const int HEADER = 4;
    while (m_buffer.size() >= HEADER) {
        uint32_t bodyLen = 0;
        for (int i = 0; i < HEADER; i++)
            bodyLen = (bodyLen << 8) | static_cast<unsigned char>(m_buffer[i]);

        if (static_cast<int>(m_buffer.size()) < HEADER + static_cast<int>(bodyLen))
            break; // wait for more data

        std::string raw(m_buffer.constData() + HEADER, bodyLen);
        m_buffer.remove(0, HEADER + static_cast<int>(bodyLen));

        try {
            emit messageReceived(nlohmann::json::parse(raw));
        } catch (...) {
            emit networkError("Received malformed JSON from server");
        }
    }
}

} // namespace bus

#include "Session.h"
#include "Messages.h"
#include <iostream>

namespace bus {

Session::Session(tcp::socket socket, BusinessLogic& logic)
    : m_socket(std::move(socket)), m_logic(logic)
{
    m_headerBuf.fill(0);
}

void Session::start() {
    std::cout << "[Server] Client connected: "
              << m_socket.remote_endpoint() << "\n";
    readHeader();
}

void Session::readHeader() {
    auto self = shared_from_this();
    boost::asio::async_read(
        m_socket,
        boost::asio::buffer(m_headerBuf),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (ec) {
                if (ec != boost::asio::error::eof)
                    std::cerr << "[Server] Header read error: " << ec.message() << "\n";
                return;
            }
            uint32_t len = 0;
            for (int i = 0; i < 4; i++)
                len = (len << 8) | static_cast<unsigned char>(m_headerBuf[i]);
            if (len == 0 || len > 1000000) {
                nlohmann::json err;
                err["type"] = MSG_ERROR;
                err["message"] = "Invalid message length";
                sendMessage(err);
                return;
            }
            readBody(len);
        });
}

void Session::readBody(uint32_t bodyLen) {
    m_bodyBuf.resize(bodyLen);
    auto self = shared_from_this();
    boost::asio::async_read(
        m_socket,
        boost::asio::buffer(m_bodyBuf),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (ec) {
                std::cerr << "[Server] Body read error: " << ec.message() << "\n";
                return;
            }
            processMessage(std::string(m_bodyBuf.begin(), m_bodyBuf.end()));
        });
}

void Session::processMessage(const std::string& raw) {
    nlohmann::json req;
    try {
        req = nlohmann::json::parse(raw);
    } catch (...) {
        nlohmann::json err;
        err["type"] = MSG_ERROR;
        err["message"] = "Malformed JSON";
        sendMessage(err);
        readHeader();
        return;
    }
    m_logic.handleMessage(this, m_userId, req);
    readHeader();
}

void Session::sendMessage(const nlohmann::json& msg) {
    std::string body = msg.dump();
    uint32_t len     = static_cast<uint32_t>(body.size());

    // frame is kept alive by the shared_ptr captured in the lambda
    auto frame = std::make_shared<std::vector<char>>(4 + body.size());
    (*frame)[0] = static_cast<char>((len >> 24) & 0xFF);
    (*frame)[1] = static_cast<char>((len >> 16) & 0xFF);
    (*frame)[2] = static_cast<char>((len >>  8) & 0xFF);
    (*frame)[3] = static_cast<char>((len >>  0) & 0xFF);
    std::copy(body.begin(), body.end(), frame->begin() + 4);

    auto self = shared_from_this();
    boost::asio::async_write(
        m_socket,
        boost::asio::buffer(*frame),
        [self, frame](boost::system::error_code ec, std::size_t) {
            if (ec) std::cerr << "[Server] Write error: " << ec.message() << "\n";
        });
}

} // namespace bus

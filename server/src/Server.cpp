#include "Server.h"
#include "Session.h"
#include <iostream>

namespace bus {

Server::Server(boost::asio::io_context& io,
               unsigned short port,
               DataStore& store)
    : m_acceptor(io, tcp::endpoint(tcp::v4(), port))
    , m_store(store)
    , m_logic(store)
{
    std::cout << "[Server] Listening on port " << port << "\n";
    acceptNext();
}

void Server::acceptNext() {
    m_acceptor.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto session = std::make_shared<Session>(
                    std::move(socket), m_logic);
                session->start();
            } else {
                std::cerr << "[Server] Accept error: " << ec.message() << "\n";
            }
            acceptNext();
        });
}

} // namespace bus

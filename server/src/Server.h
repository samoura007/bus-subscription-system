#pragma once
#include "logic/BusinessLogic.h"
#include <boost/asio.hpp>

namespace bus {

using boost::asio::ip::tcp;

class Server {
public:
    Server(boost::asio::io_context& io,
           unsigned short port,
           DataStore& store);

private:
    void acceptNext();
    tcp::acceptor m_acceptor;
    DataStore&    m_store;
    BusinessLogic m_logic;
};

} // namespace bus

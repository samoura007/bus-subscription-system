#pragma once
#include "INetworkClient.h"
#include "logic/BusinessLogic.h"
#include <boost/asio.hpp>
#include <memory>
#include <array>

namespace bus {

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session>,
                public INetworkClient {
public:
    Session(tcp::socket socket, BusinessLogic& logic);
    void start();
    void sendMessage(const nlohmann::json& msg) override;

private:
    void readHeader();
    void readBody(uint32_t bodyLen);
    void processMessage(const std::string& raw);

    tcp::socket          m_socket;
    BusinessLogic&       m_logic;
    int                  m_userId = 0;
    static const size_t  HEADER_SIZE = 4;
    std::array<char, 4>  m_headerBuf;
    std::vector<char>    m_bodyBuf;
};

} // namespace bus

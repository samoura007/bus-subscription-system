#pragma once
#include <nlohmann/json.hpp>

namespace bus {

class INetworkClient {
public:
    virtual ~INetworkClient() = default;
    virtual void sendMessage(const nlohmann::json& msg) = 0;
};

} //namespace bus

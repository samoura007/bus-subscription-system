#pragma once
#include <nlohmann/json.hpp>

namespace bus {

// Abstract interface so GoogleMock can mock the network in unit tests.
class INetworkClient {
public:
    virtual ~INetworkClient() = default;
    virtual void sendMessage(const nlohmann::json& msg) = 0;
};

} // namespace bus

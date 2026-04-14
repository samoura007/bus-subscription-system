#pragma once
#include "DataStore.h"
#include "INetworkClient.h"
#include <nlohmann/json.hpp>
#include <string>

namespace bus {

class BusinessLogic {
public:
    explicit BusinessLogic(DataStore& store);

    void handleMessage(INetworkClient* client,
                       int& sessionUserId,
                       const nlohmann::json& request);

    static std::string hashPassword(const std::string& plain);
    static std::string currentTimestamp();

private:
    DataStore& m_store;

    void handleRegister    (INetworkClient* c, int& uid, const nlohmann::json& req);
    void handleLogin       (INetworkClient* c, int& uid, const nlohmann::json& req);
    void handleGetRoutes   (INetworkClient* c);
    void handleAddRoute    (INetworkClient* c, int uid, const nlohmann::json& req);
    void handleDelRoute    (INetworkClient* c, int uid, const nlohmann::json& req);
    void handleSubscribe   (INetworkClient* c, int uid, const nlohmann::json& req);
    void handleUnsubscribe (INetworkClient* c, int uid, const nlohmann::json& req);
    void handleGetSubs     (INetworkClient* c, int uid);
    void handleRide        (INetworkClient* c, int uid, const nlohmann::json& req);
    void handleGetTickets  (INetworkClient* c, int uid);
    void handleSetFreetime (INetworkClient* c, int uid, const nlohmann::json& req);
    void handleGetFreetime (INetworkClient* c, int uid);
    void handleGetUsers    (INetworkClient* c, int uid);

    static nlohmann::json makeError(const std::string& msg);
    static nlohmann::json makeOk   (const std::string& msg = "");
};

} // namespace bus

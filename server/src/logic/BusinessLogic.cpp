#include "BusinessLogic.h"
#include "Messages.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

namespace bus {

BusinessLogic::BusinessLogic(DataStore& store) : m_store(store) {}

std::string BusinessLogic::hashPassword(const std::string& plain) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(plain.c_str()), plain.size(), hash);
    std::ostringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::string BusinessLogic::currentTimestamp() {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

nlohmann::json BusinessLogic::makeOk(const std::string& msg) {
    nlohmann::json j;
    j["type"] = MSG_OK;
    j["message"] = msg;
    return j;
}

nlohmann::json BusinessLogic::makeError(const std::string& msg) {
    nlohmann::json j;
    j["type"] = MSG_ERROR;
    j["message"] = msg;
    return j;
}

void BusinessLogic::handleMessage(INetworkClient* client, int& sessionUserId, const nlohmann::json& req) {
    std::string type = req.value("type", "");

    if (type == MSG_REGISTER)             handleRegister(client, sessionUserId, req);
    else if (type == MSG_LOGIN)           handleLogin   (client, sessionUserId, req);
    else if (type == MSG_GET_ROUTES)      handleGetRoutes(client);
    else if (sessionUserId == 0)          client->sendMessage(makeError("Unauthorized"));
    else if (type == MSG_ADD_ROUTE)       handleAddRoute(client, sessionUserId, req);
    else if (type == MSG_DEL_ROUTE)       handleDelRoute(client, sessionUserId, req);
    else if (type == MSG_SUBSCRIBE)       handleSubscribe(client, sessionUserId, req);
    else if (type == MSG_UNSUBSCRIBE)     handleUnsubscribe(client, sessionUserId, req);
    else if (type == MSG_GET_SUBS)        handleGetSubs (client, sessionUserId);
    else if (type == MSG_RIDE)            handleRide    (client, sessionUserId, req);
    else if (type == MSG_GET_TICKETS)     handleGetTickets(client, sessionUserId);
    else if (type == MSG_UPDATE_SCHEDULE) handleUpdateSchedule(client, sessionUserId, req);
    else if (type == MSG_GET_USERS)       handleGetUsers(client, sessionUserId);
    else if (type == MSG_GET_USER_DETAIL) handleGetUserDetail(client, sessionUserId, req);
    else if (type == MSG_SET_VOTES)       handleSetVotes(client, sessionUserId, req);
    else if (type == MSG_GET_VOTES)       handleGetVotes(client, sessionUserId);
    else if (type == MSG_GET_DEMAND)      handleGetDemand(client, sessionUserId);
    else                                  client->sendMessage(makeError("Unknown message type: " + type));
}

void BusinessLogic::handleRegister(INetworkClient* client, int& uid, const nlohmann::json& req) {
    if (!req.contains("username") || !req.contains("password")) {
        client->sendMessage(makeError("Missing fields"));
        return;
    }
    std::string username = req["username"].get<std::string>();
    std::string password = req["password"].get<std::string>();
    if (username.empty() || password.size() < 6) {
        client->sendMessage(makeError("Invalid data"));
        return;
    }
    if (!m_store.addUser(username, hashPassword(password), ROLE_USER)) {
        client->sendMessage(makeError("Taken"));
        return;
    }
    auto u = m_store.findUserByUsername(username);
    uid = u->id;
    nlohmann::json reply;
    reply["type"] = MSG_AUTH_OK;
    reply["user"] = u->toPublicJson();
    client->sendMessage(reply);
}

void BusinessLogic::handleLogin(INetworkClient* client, int& uid, const nlohmann::json& req) {
    std::string username = req.value("username", "");
    std::string password = req.value("password", "");
    auto u = m_store.findUserByUsername(username);
    if (!u || u->passwordHash != hashPassword(password)) {
        client->sendMessage(makeError("Invalid credentials"));
        return;
    }
    uid = u->id;
    nlohmann::json reply;
    reply["type"] = MSG_AUTH_OK;
    reply["user"] = u->toPublicJson();
    client->sendMessage(reply);
}

void BusinessLogic::handleGetRoutes(INetworkClient* client) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& r : m_store.getAllRoutes()) arr.push_back(r.toJson());
    nlohmann::json reply;
    reply["type"] = MSG_ROUTES_LIST;
    reply["routes"] = arr;
    client->sendMessage(reply);
}

void BusinessLogic::handleAddRoute(INetworkClient* client, int uid, const nlohmann::json& req) {
    auto u = m_store.findUserById(uid);
    if (!u || u->role != ROLE_ADMIN) {
        client->sendMessage(makeError("Unauthorized"));
        return;
    }
    m_store.addRoute(req.value("name", ""), req.value("stops", ""), req.value("schedule", ""));
    nlohmann::json reply; reply["type"] = MSG_ROUTE_OK; client->sendMessage(reply);
}

void BusinessLogic::handleDelRoute(INetworkClient* client, int uid, const nlohmann::json& req) {
    auto u = m_store.findUserById(uid);
    if (!u || u->role != ROLE_ADMIN) { client->sendMessage(makeError("Unauthorized")); return; }
    if (m_store.deleteRoute(req.value("routeId", 0))) client->sendMessage(makeOk("Deleted"));
    else client->sendMessage(makeError("Failed to delete"));
}

void BusinessLogic::handleSubscribe(INetworkClient* client, int uid, const nlohmann::json& req) {
    if (m_store.subscribe(uid, req.value("routeId", 0))) client->sendMessage(makeOk("Subscribed"));
    else client->sendMessage(makeError("Already subscribed"));
}

void BusinessLogic::handleUnsubscribe(INetworkClient* client, int uid, const nlohmann::json& req) {
    if (m_store.unsubscribe(uid, req.value("routeId", 0))) client->sendMessage(makeOk("Unsubscribed"));
    else client->sendMessage(makeError("Not subscribed"));
}

void BusinessLogic::handleGetSubs(INetworkClient* client, int uid) {
    nlohmann::json arr = nlohmann::json::array();
    for (int rid : m_store.getSubscribedRouteIds(uid)) arr.push_back(rid);
    nlohmann::json reply; reply["type"] = MSG_SUBS_LIST; reply["subs"] = arr;
    client->sendMessage(reply);
}

void BusinessLogic::handleRide(INetworkClient* client, int uid, const nlohmann::json& req) {
    int rid = req.value("routeId", 0);
    nlohmann::json reply;
    reply["type"] = MSG_OK;
    if (m_store.isSubscribed(uid, rid)) {
        reply["message"] = "Ride recorded. Have a safe trip!";
    } else {
        int tid = m_store.issueTicket(uid, rid, currentTimestamp(), true, 50);
        reply["ticketIssued"] = true;
        reply["ticketId"] = tid;
        reply["message"] = "No subscription. Ticket #" + std::to_string(tid) + " issued.";
    }
    client->sendMessage(reply);
}

void BusinessLogic::handleGetTickets(INetworkClient* client, int uid) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& t : m_store.getTicketsForUser(uid)) arr.push_back(t.toJson());
    nlohmann::json reply;
    reply["type"] = MSG_TICKETS;
    reply["tickets"] = arr;
    client->sendMessage(reply);
}

void BusinessLogic::handleUpdateSchedule(INetworkClient* client, int uid, const nlohmann::json& req) {
    auto u = m_store.findUserById(uid);
    if (!u || u->role != ROLE_ADMIN) {
        client->sendMessage(makeError("Unauthorized"));
        return;
    }
    int routeId = req.value("routeId", 0);
    std::string schedule = req.value("schedule", "[]");
    m_store.updateRouteSchedule(routeId, schedule);
    client->sendMessage(makeOk("Schedule updated"));
}

void BusinessLogic::handleGetUsers(INetworkClient* client, int uid) {
    auto u = m_store.findUserById(uid);
    if (!u || u->role != ROLE_ADMIN) { client->sendMessage(makeError("Unauthorized")); return; }
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& user : m_store.getAllUsers()) arr.push_back(user.toPublicJson());
    nlohmann::json reply; reply["type"] = MSG_USERS_LIST; reply["users"] = arr;
    client->sendMessage(reply);
}

void BusinessLogic::handleGetUserDetail(INetworkClient* client, int uid, const nlohmann::json& req) {
    auto u = m_store.findUserById(uid);
    if (!u || u->role != ROLE_ADMIN) { client->sendMessage(makeError("Unauthorized")); return; }

    int targetId = req.value("userId", 0);
    auto target = m_store.findUserById(targetId);
    if (!target) { client->sendMessage(makeError("User not found")); return; }

    nlohmann::json reply;
    reply["type"] = MSG_USER_DETAIL;
    reply["user"] = target->toPublicJson();

    nlohmann::json subsArr = nlohmann::json::array();
    for (int rid : m_store.getSubscribedRouteIds(targetId)) {
        if (auto route = m_store.findRouteById(rid)) subsArr.push_back(route->toJson());
    }
    reply["subscriptions"] = subsArr;

    nlohmann::json ticksArr = nlohmann::json::array();
    for (const auto& t : m_store.getTicketsForUser(targetId)) ticksArr.push_back(t.toJson());
    reply["tickets"] = ticksArr;

    client->sendMessage(reply);
}

void BusinessLogic::handleSetVotes(INetworkClient* client, int uid, const nlohmann::json& req) {
    std::vector<RouteVote> votes;
    for (const auto& j : req["votes"]) {
        RouteVote v;
        v.routeId = j.value("routeId", 0);
        v.day = j.value("day", "");
        v.timeSlot = j.value("timeSlot", "");
        v.destination = j.value("destination", "");
        votes.push_back(v);
    }
    m_store.setVotes(uid, votes);
    client->sendMessage(makeOk("Requests updated."));
}

void BusinessLogic::handleGetVotes(INetworkClient* client, int uid) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& v : m_store.getVotes(uid)) {
        arr.push_back(v.toJson());
    }
    nlohmann::json reply;
    reply["type"] = MSG_VOTES_LIST;
    reply["votes"] = arr;
    client->sendMessage(reply);
}

void BusinessLogic::handleGetDemand(INetworkClient* client, int uid) {
    auto u = m_store.findUserById(uid);
    if (!u || u->role != ROLE_ADMIN) {
        client->sendMessage(makeError("Unauthorized"));
        return;
    }
    nlohmann::json reply;
    reply["type"] = MSG_DEMAND_LIST;
    reply["demand"] = m_store.getAggregatedDemand();
    client->sendMessage(reply);
}

} // namespace bus

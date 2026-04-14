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
    SHA256(reinterpret_cast<const unsigned char*>(plain.c_str()),
           plain.size(), hash);
    std::ostringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::string BusinessLogic::currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

nlohmann::json BusinessLogic::makeError(const std::string& msg) {
    nlohmann::json j;
    j["type"]    = MSG_ERROR;
    j["message"] = msg;
    return j;
}

nlohmann::json BusinessLogic::makeOk(const std::string& msg) {
    nlohmann::json j;
    j["type"] = MSG_OK;
    if (!msg.empty()) j["message"] = msg;
    return j;
}

void BusinessLogic::handleMessage(INetworkClient* client,
                                  int& sessionUserId,
                                  const nlohmann::json& req) {
    if (!req.contains("type")) {
        client->sendMessage(makeError("Missing type field"));
        return;
    }
    std::string type = req["type"].get<std::string>();

    if (type == MSG_REGISTER) { handleRegister(client, sessionUserId, req); return; }
    if (type == MSG_LOGIN)    { handleLogin   (client, sessionUserId, req); return; }

    if (sessionUserId == 0) {
        client->sendMessage(makeError("Not authenticated"));
        return;
    }

    if      (type == MSG_GET_ROUTES)   handleGetRoutes  (client);
    else if (type == MSG_ADD_ROUTE)    handleAddRoute   (client, sessionUserId, req);
    else if (type == MSG_DEL_ROUTE)    handleDelRoute   (client, sessionUserId, req);
    else if (type == MSG_SUBSCRIBE)    handleSubscribe  (client, sessionUserId, req);
    else if (type == MSG_UNSUBSCRIBE)  handleUnsubscribe(client, sessionUserId, req);
    else if (type == MSG_GET_SUBS)     handleGetSubs    (client, sessionUserId);
    else if (type == MSG_RIDE)         handleRide       (client, sessionUserId, req);
    else if (type == MSG_GET_TICKETS)  handleGetTickets (client, sessionUserId);
    else if (type == MSG_SET_FREETIME) handleSetFreetime(client, sessionUserId, req);
    else if (type == MSG_GET_FREETIME) handleGetFreetime(client, sessionUserId);
    else if (type == MSG_GET_USERS)    handleGetUsers   (client, sessionUserId);
    else client->sendMessage(makeError("Unknown message type: " + type));
}

void BusinessLogic::handleRegister(INetworkClient* client, int& uid,
                                   const nlohmann::json& req) {
    if (!req.contains("username") || !req.contains("password")) {
        client->sendMessage(makeError("Missing username or password")); return;
    }
    std::string username = req["username"].get<std::string>();
    std::string password = req["password"].get<std::string>();
    if (username.empty()) {
        client->sendMessage(makeError("Username cannot be empty")); return;
    }
    if (password.size() < 6) {
        client->sendMessage(makeError("Password must be at least 6 characters")); return;
    }
    if (!m_store.addUser(username, hashPassword(password), ROLE_USER)) {
        client->sendMessage(makeError("Username already taken")); return;
    }
    auto u = m_store.findUserByUsername(username);
    uid = u->id;
    nlohmann::json reply;
    reply["type"] = MSG_AUTH_OK;
    reply["user"] = u->toPublicJson();
    client->sendMessage(reply);
}

void BusinessLogic::handleLogin(INetworkClient* client, int& uid,
                                const nlohmann::json& req) {
    if (!req.contains("username") || !req.contains("password")) {
        client->sendMessage(makeError("Missing username or password")); return;
    }
    std::string username = req["username"].get<std::string>();
    std::string password = req["password"].get<std::string>();
    auto u = m_store.findUserByUsername(username);
    if (!u || u->passwordHash != hashPassword(password)) {
        client->sendMessage(makeError("Invalid username or password")); return;
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
    reply["type"]   = MSG_ROUTES_LIST;
    reply["routes"] = arr;
    client->sendMessage(reply);
}

void BusinessLogic::handleAddRoute(INetworkClient* client, int uid,
                                   const nlohmann::json& req) {
    auto u = m_store.findUserById(uid);
    if (!u || u->role != ROLE_ADMIN) {
        client->sendMessage(makeError("Admins only")); return;
    }
    if (!req.contains("name")) {
        client->sendMessage(makeError("Route name required")); return;
    }
    int id = m_store.addRoute(req["name"].get<std::string>(),
                              req.value("stops",""),
                              req.value("schedule",""));
    nlohmann::json reply;
    reply["type"]    = MSG_ROUTE_OK;
    reply["routeId"] = id;
    client->sendMessage(reply);
}

void BusinessLogic::handleDelRoute(INetworkClient* client, int uid,
                                   const nlohmann::json& req) {
    auto u = m_store.findUserById(uid);
    if (!u || u->role != ROLE_ADMIN) {
        client->sendMessage(makeError("Admins only")); return;
    }
    if (!m_store.deleteRoute(req.value("routeId",-1))) {
        client->sendMessage(makeError("Route not found")); return;
    }
    client->sendMessage(makeOk("Route deleted"));
}

void BusinessLogic::handleSubscribe(INetworkClient* client, int uid,
                                    const nlohmann::json& req) {
    int rid = req.value("routeId",-1);
    if (!m_store.findRouteById(rid)) {
        client->sendMessage(makeError("Route not found")); return;
    }
    if (!m_store.subscribe(uid, rid)) {
        client->sendMessage(makeError("Already subscribed")); return;
    }
    client->sendMessage(makeOk("Subscribed"));
}

void BusinessLogic::handleUnsubscribe(INetworkClient* client, int uid,
                                      const nlohmann::json& req) {
    if (!m_store.unsubscribe(uid, req.value("routeId",-1))) {
        client->sendMessage(makeError("Not subscribed")); return;
    }
    client->sendMessage(makeOk("Unsubscribed"));
}

void BusinessLogic::handleGetSubs(INetworkClient* client, int uid) {
    nlohmann::json arr = nlohmann::json::array();
    for (int rid : m_store.getSubscribedRouteIds(uid)) {
        auto r = m_store.findRouteById(rid);
        if (r) arr.push_back(r->toJson());
    }
    nlohmann::json reply;
    reply["type"]          = MSG_SUBS_LIST;
    reply["subscriptions"] = arr;
    client->sendMessage(reply);
}

void BusinessLogic::handleRide(INetworkClient* client, int uid,
                               const nlohmann::json& req) {
    int rid = req.value("routeId",-1);
    if (!m_store.findRouteById(rid)) {
        client->sendMessage(makeError("Route not found")); return;
    }
    nlohmann::json reply;
    reply["type"]    = MSG_TICKETS;
    reply["routeId"] = rid;
    if (m_store.isSubscribed(uid, rid)) {
        reply["ticketIssued"] = false;
        reply["message"]      = "Ride recorded. Valid subscription.";
    } else {
        int tid = m_store.issueTicket(uid, rid, currentTimestamp());
        reply["ticketIssued"] = true;
        reply["ticketId"]     = tid;
        reply["message"]      = "No subscription. Ticket #" + std::to_string(tid) + " issued.";
    }
    client->sendMessage(reply);
}

void BusinessLogic::handleGetTickets(INetworkClient* client, int uid) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& t : m_store.getTicketsForUser(uid)) arr.push_back(t.toJson());
    nlohmann::json reply;
    reply["type"]    = MSG_TICKETS;
    reply["tickets"] = arr;
    client->sendMessage(reply);
}

void BusinessLogic::handleSetFreetime(INetworkClient* client, int uid,
                                      const nlohmann::json& req) {
    if (!req.contains("slots") || !req["slots"].is_array()) {
        client->sendMessage(makeError("Expected 'slots' array")); return;
    }
    std::vector<FreeSlot> slots;
    for (const auto& s : req["slots"]) {
        FreeSlot fs;
        fs.userId    = uid;
        fs.day       = s.value("day","");
        fs.startTime = s.value("startTime","");
        fs.endTime   = s.value("endTime","");
        if (fs.day.empty()) {
            client->sendMessage(makeError("Slot missing day")); return;
        }
        slots.push_back(fs);
    }
    m_store.setFreeSlots(uid, slots);
    client->sendMessage(makeOk("Free time saved"));
}

void BusinessLogic::handleGetFreetime(INetworkClient* client, int uid) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& fs : m_store.getFreeSlots(uid)) arr.push_back(fs.toJson());
    nlohmann::json reply;
    reply["type"]  = MSG_FREETIME;
    reply["slots"] = arr;
    client->sendMessage(reply);
}

void BusinessLogic::handleGetUsers(INetworkClient* client, int uid) {
    auto u = m_store.findUserById(uid);
    if (!u || u->role != ROLE_ADMIN) {
        client->sendMessage(makeError("Admins only")); return;
    }
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& usr : m_store.getAllUsers()) arr.push_back(usr.toPublicJson());
    nlohmann::json reply;
    reply["type"]  = MSG_USERS_LIST;
    reply["users"] = arr;
    client->sendMessage(reply);
}

} // namespace bus

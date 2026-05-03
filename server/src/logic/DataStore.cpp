#include "DataStore.h"
#include <algorithm>
#include <nlohmann/json.hpp>

namespace bus {

DataStore::DataStore() {
    // Default admin user
    addUser("admin", "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918", "admin");

    // Pass empty string as schedule to trigger default JSON generation
    addRoute("AUC -> Nasr City", "AUC Gate 1, Abbas El-Akkad, Nasr City", "");
    addRoute("AUC -> Maadi", "AUC Gate 2, Road 9, Maadi Metro", "");
    addRoute("AUC -> Heliopolis", "AUC Gate 1, Airport Rd, Heliopolis", "");
}

std::string DataStore::generateDefaultSchedule(const std::string& routeName) {
    std::string dest = routeName;
    auto pos = dest.find("->");
    if (pos != std::string::npos) {
        dest = dest.substr(pos + 2);
        dest.erase(0, dest.find_first_not_of(" \t")); // Trim leading spaces
    } else {
        auto p = dest.find("AUC");
        if (p != std::string::npos) dest.erase(p, 3);
        dest.erase(0, dest.find_first_not_of(" \t"));
        if (dest.empty()) dest = "Other Stop";
    }
    
    nlohmann::json arr = nlohmann::json::array();
    nlohmann::json t1; t1["time"] = "07:00 AM"; t1["destination"] = "University";
    nlohmann::json t2; t2["time"] = "04:00 PM"; t2["destination"] = dest;
    arr.push_back(t1); arr.push_back(t2);
    return arr.dump();
}

bool DataStore::addUser(const std::string& username, const std::string& passwordHash, const std::string& role) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& u : m_users) {
        if (u.username == username) return false;
    }
    User u;
    u.id = m_nextUserId++;
    u.username = username;
    u.passwordHash = passwordHash;
    u.role = role;
    m_users.push_back(u);
    return true;
}

std::optional<User> DataStore::findUserByUsername(const std::string& username) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& u : m_users) {
        if (u.username == username) return u;
    }
    return std::nullopt;
}

std::optional<User> DataStore::findUserById(int id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& u : m_users) {
        if (u.id == id) return u;
    }
    return std::nullopt;
}

std::vector<User> DataStore::getAllUsers() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_users;
}

int DataStore::addRoute(const std::string& name, const std::string& stops, const std::string& schedule) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Route r;
    r.id = m_nextRouteId++;
    r.name = name;
    r.stops = stops;
    r.schedule = schedule.empty() ? generateDefaultSchedule(name) : schedule;
    m_routes.push_back(r);
    return r.id;
}

void DataStore::updateRouteSchedule(int routeId, const std::string& schedule) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& r : m_routes) {
        if (r.id == routeId) {
            r.schedule = schedule;
            break;
        }
    }
}

bool DataStore::deleteRoute(int routeId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::remove_if(m_routes.begin(), m_routes.end(), [routeId](const Route& r){ return r.id == routeId; });
    if (it != m_routes.end()) {
        m_routes.erase(it, m_routes.end());
        m_subscriptions.erase(
            std::remove_if(m_subscriptions.begin(), m_subscriptions.end(), [routeId](auto& p){ return p.second == routeId; }),
            m_subscriptions.end()
        );
        return true;
    }
    return false;
}

std::vector<Route> DataStore::getAllRoutes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_routes;
}

std::optional<Route> DataStore::findRouteById(int routeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& r : m_routes) {
        if (r.id == routeId) return r;
    }
    return std::nullopt;
}

bool DataStore::subscribe(int userId, int routeId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& p : m_subscriptions) {
        if (p.first == userId && p.second == routeId) return false;
    }
    m_subscriptions.push_back({userId, routeId});
    return true;
}

bool DataStore::unsubscribe(int userId, int routeId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::remove_if(m_subscriptions.begin(), m_subscriptions.end(), [&](const auto& p){
        return p.first == userId && p.second == routeId;
    });
    if (it != m_subscriptions.end()) {
        m_subscriptions.erase(it, m_subscriptions.end());
        return true;
    }
    return false;
}

bool DataStore::isSubscribed(int userId, int routeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& p : m_subscriptions) {
        if (p.first == userId && p.second == routeId) return true;
    }
    return false;
}

std::vector<int> DataStore::getSubscribedRouteIds(int userId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<int> res;
    for (const auto& p : m_subscriptions) {
        if (p.first == userId) res.push_back(p.second);
    }
    return res;
}

int DataStore::issueTicket(int userId, int routeId, const std::string& timestamp, bool charged, int price) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Ticket t;
    t.id = m_nextTicketId++;
    t.userId = userId;
    t.routeId = routeId;
    t.issuedAt = timestamp;
    t.charged = charged;
    t.price = price;
    m_tickets.push_back(t);
    return t.id;
}

std::vector<Ticket> DataStore::getTicketsForUser(int userId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Ticket> result;
    for (const auto& t : m_tickets) {
        if (t.userId == userId) result.push_back(t);
    }
    return result;
}

void DataStore::setVotes(int userId, const std::vector<RouteVote>& votes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_votes[userId] = votes;
}

std::vector<RouteVote> DataStore::getVotes(int userId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_votes.find(userId);
    if (it != m_votes.end()) return it->second;
    return {};
}

nlohmann::json DataStore::getAggregatedDemand() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::map<std::string, int> counts;
    for (const auto& pair : m_votes) {
        for (const auto& v : pair.second) {
            std::string key = std::to_string(v.routeId) + "|" + v.day + "|" + v.timeSlot + "|" + v.destination;
            counts[key]++;
        }
    }
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& c : counts) {
        nlohmann::json j;
        j["key"] = c.first;
        j["count"] = c.second;
        arr.push_back(j);
    }
    return arr;
}

} // namespace bus

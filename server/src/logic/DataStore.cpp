#include "DataStore.h"
#include <map>
#include <tuple>

namespace bus {

DataStore::DataStore() { seedData(); }

void DataStore::seedData() {
    addUser("admin",
            "240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9",
            "admin");
    addRoute("AUC -> Nasr City",
             "AUC Gate 1, Abbas El-Akkad, Nasr City",
             "7:00, 9:00, 13:00, 17:00");
    addRoute("AUC -> Maadi",
             "AUC Gate 2, Road 9, Maadi Metro",
             "7:30, 9:30, 14:00, 18:00");
    addRoute("AUC -> Heliopolis",
             "AUC Gate 1, Airport Rd, Heliopolis",
             "8:00, 10:00, 15:00, 19:00");
}

bool DataStore::addUser(const std::string& username, const std::string& passwordHash, const std::string& role) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& u : m_users) {
        if (u.username == username) return false;
    }
    User u;
    u.id           = m_nextUserId++;
    u.username     = username;
    u.passwordHash = passwordHash;
    u.role         = role;
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
    r.id       = m_nextRouteId++;
    r.name     = name;
    r.stops    = stops;
    r.schedule = schedule;
    m_routes.push_back(r);
    return r.id;
}

bool DataStore::deleteRoute(int routeId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_routes.begin(); it != m_routes.end(); ++it) {
        if (it->id == routeId) {
            m_routes.erase(it);
            return true;
        }
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
    for (const auto& s : m_subscriptions) {
        if (s.first == userId && s.second == routeId) return false;
    }
    m_subscriptions.push_back({userId, routeId});
    return true;
}

bool DataStore::unsubscribe(int userId, int routeId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_subscriptions.begin(); it != m_subscriptions.end(); ++it) {
        if (it->first == userId && it->second == routeId) {
            m_subscriptions.erase(it);
            return true;
        }
    }
    return false;
}

bool DataStore::isSubscribed(int userId, int routeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& s : m_subscriptions) {
        if (s.first == userId && s.second == routeId) return true;
    }
    return false;
}

std::vector<int> DataStore::getSubscribedRouteIds(int userId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<int> ids;
    for (const auto& s : m_subscriptions) {
        if (s.first == userId) ids.push_back(s.second);
    }
    return ids;
}

int DataStore::issueTicket(int userId, int routeId, const std::string& timestamp, bool charged, int price) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Ticket t;
    t.id       = m_nextTicketId++;
    t.userId   = userId;
    t.routeId  = routeId;
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
    m_votes.erase(
        std::remove_if(m_votes.begin(), m_votes.end(),
                       [userId](const RouteVote& v){ return v.userId == userId; }),
        m_votes.end());
    for (const auto& v : votes) {
        m_votes.push_back(v);
    }
}

std::vector<RouteVote> DataStore::getVotes(int userId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<RouteVote> result;
    for (const auto& v : m_votes) {
        if (v.userId == userId) result.push_back(v);
    }
    return result;
}

nlohmann::json DataStore::getAggregatedDemand() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::map<std::tuple<int, std::string, std::string, std::string>, int> counts;
    for (const auto& v : m_votes) {
        counts[{v.routeId, v.day, v.timeSlot, v.direction}]++;
    }

    std::map<int, std::pair<std::tuple<std::string, std::string, std::string>, int>> max_per_route;
    for (const auto& kv : counts) {
        int rid = std::get<0>(kv.first);
        int count = kv.second;
        if (max_per_route.find(rid) == max_per_route.end() || count > max_per_route[rid].second) {
            max_per_route[rid] = {{std::get<1>(kv.first), std::get<2>(kv.first), std::get<3>(kv.first)}, count};
        }
    }

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& kv : max_per_route) {
        int count = kv.second.second;
        std::string busType;
        if (count <= 47) busType = "Mid-size Coach (36-47 passengers)";
        else if (count <= 56) busType = "Standard Motorcoach (47-56 passengers)";
        else if (count <= 60) busType = "Double-Axle Motorcoach (50-60 passengers)";
        else busType = "High-Capacity Coach (56-90 passengers)";

std::optional<Route> routeOpt = std::nullopt;
        for (const auto& r : m_routes) { if (r.id == kv.first) { routeOpt = r; break; } }
        std::string rname = routeOpt ? routeOpt->name : "Unknown";

        nlohmann::json j;
        j["routeId"]   = kv.first;
        j["routeName"] = rname;
        j["day"]       = std::get<0>(kv.second.first);
        j["timeSlot"]  = std::get<1>(kv.second.first);
        j["direction"] = std::get<2>(kv.second.first);
        j["votes"]     = count;
        j["busType"]   = busType;
        arr.push_back(j);
    }
    return arr;
}

void DataStore::fulfillDispatch(int routeId, const std::string& day, const std::string& timeSlot, const std::string& direction) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_votes.erase(
        std::remove_if(m_votes.begin(), m_votes.end(), [&](const RouteVote& v) {
            return v.routeId == routeId && v.day == day && v.timeSlot == timeSlot && v.direction == direction;
        }),
        m_votes.end());
}

} // namespace bus

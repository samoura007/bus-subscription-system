#include "DataStore.h"
#include <map>

namespace bus {

DataStore::DataStore() { seedData(); }

void DataStore::seedData() {
    // Admin password is "admin123" (SHA-256 hash)
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

bool DataStore::addUser(const std::string& username,
                        const std::string& passwordHash,
                        const std::string& role) {
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

int DataStore::addRoute(const std::string& name,
                        const std::string& stops,
                        const std::string& schedule) {
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

int DataStore::issueTicket(int userId, int routeId, const std::string& timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Ticket t;
    t.id       = m_nextTicketId++;
    t.userId   = userId;
    t.routeId  = routeId;
    t.issuedAt = timestamp;
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

void DataStore::setFreeSlots(int userId, const std::vector<FreeSlot>& slots) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_freeSlots.erase(
        std::remove_if(m_freeSlots.begin(), m_freeSlots.end(),
                       [userId](const FreeSlot& fs){ return fs.userId == userId; }),
        m_freeSlots.end());
    for (const auto& s : slots) {
        m_freeSlots.push_back(s);
    }
}

std::vector<FreeSlot> DataStore::getFreeSlots(int userId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<FreeSlot> result;
    for (const auto& fs : m_freeSlots) {
        if (fs.userId == userId) result.push_back(fs);
    }
    return result;
}

std::vector<std::pair<std::string, int>> DataStore::getAvailability(int routeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<int> subs;
    for (const auto& s : m_subscriptions) {
        if (s.second == routeId) subs.push_back(s.first);
    }
    std::map<std::string, int> counts;
    for (const auto& fs : m_freeSlots) {
        bool found = false;
        for (int uid : subs) { if (uid == fs.userId) { found = true; break; } }
        if (found) {
            std::string key = fs.day + " " + fs.startTime + "-" + fs.endTime;
            counts[key]++;
        }
    }
    return std::vector<std::pair<std::string,int>>(counts.begin(), counts.end());
}

} // namespace bus

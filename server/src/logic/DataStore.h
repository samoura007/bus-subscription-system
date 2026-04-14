#pragma once
#include "../models/Models.h"
#include <vector>
#include <optional>
#include <mutex>
#include <algorithm>

namespace bus {

class DataStore {
public:
    DataStore();

    bool addUser(const std::string& username,
                 const std::string& passwordHash,
                 const std::string& role);
    std::optional<User> findUserByUsername(const std::string& username) const;
    std::optional<User> findUserById(int id) const;
    std::vector<User> getAllUsers() const;

    int  addRoute(const std::string& name,
                  const std::string& stops,
                  const std::string& schedule);
    bool deleteRoute(int routeId);
    std::vector<Route> getAllRoutes() const;
    std::optional<Route> findRouteById(int routeId) const;

    bool subscribe(int userId, int routeId);
    bool unsubscribe(int userId, int routeId);
    bool isSubscribed(int userId, int routeId) const;
    std::vector<int> getSubscribedRouteIds(int userId) const;

    int issueTicket(int userId, int routeId, const std::string& timestamp, bool charged, int price);
    std::vector<Ticket> getTicketsForUser(int userId) const;

    void setFreeSlots(int userId, const std::vector<FreeSlot>& slots);
    std::vector<FreeSlot> getFreeSlots(int userId) const;
    std::vector<std::pair<std::string, int>> getAvailability(int routeId) const;

private:
    mutable std::mutex m_mutex;
    std::vector<User>   m_users;
    std::vector<Route>  m_routes;
    std::vector<std::pair<int,int>> m_subscriptions;
    std::vector<Ticket> m_tickets;
    std::vector<FreeSlot> m_freeSlots;
    int m_nextUserId   = 1;
    int m_nextRouteId  = 1;
    int m_nextTicketId = 1;
    void seedData();
};

} // namespace bus

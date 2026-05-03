#pragma once
#include "../models/Models.h"
#include <vector>
#include <mutex>
#include <optional>
#include <string>
#include <map>

namespace bus {

class DataStore {
public:
    DataStore();

    bool addUser(const std::string& username, const std::string& passwordHash, const std::string& role);
    std::optional<User> findUserByUsername(const std::string& username) const;
    std::optional<User> findUserById(int id) const;
    std::vector<User> getAllUsers() const;

    int  addRoute(const std::string& name,
                  const std::string& stops,
                  const std::string& schedule);
    bool deleteRoute(int routeId);
    std::vector<Route> getAllRoutes() const;
    std::optional<Route> findRouteById(int routeId) const;
    void updateRouteSchedule(int routeId, const std::string& schedule);

    bool subscribe(int userId, int routeId);
    bool unsubscribe(int userId, int routeId);
    bool isSubscribed(int userId, int routeId) const;
    std::vector<int> getSubscribedRouteIds(int userId) const;

    int issueTicket(int userId, int routeId, const std::string& timestamp, bool charged, int price);
    std::vector<Ticket> getTicketsForUser(int userId) const;

    // Time Slot Requests
    void setVotes(int userId, const std::vector<RouteVote>& votes);
    std::vector<RouteVote> getVotes(int userId) const;
    nlohmann::json getAggregatedDemand() const;

private:
    std::string generateDefaultSchedule(const std::string& routeName);

    mutable std::mutex m_mutex;
    std::vector<User>   m_users;
    std::vector<Route>  m_routes;
    std::vector<std::pair<int,int>> m_subscriptions;
    std::vector<Ticket> m_tickets;
    std::map<int, std::vector<RouteVote>> m_votes;

    int m_nextUserId = 1;
    int m_nextRouteId = 1;
    int m_nextTicketId = 1;
};

} // namespace bus

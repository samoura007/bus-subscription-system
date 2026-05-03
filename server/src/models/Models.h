#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace bus {

struct User {
    int id = 0;
    std::string username;
    std::string passwordHash;
    std::string role;

    nlohmann::json toPublicJson() const {
        nlohmann::json j;
        j["id"] = id;
        j["username"] = username;
        j["role"] = role;
        return j;
    }
};

struct Route {
    int id = 0;
    std::string name;
    std::string stops;
    std::string schedule; 

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["id"] = id;
        j["name"] = name;
        j["stops"] = stops;
        j["schedule"] = schedule;
        return j;
    }
};

struct Ticket {
    int id = 0;
    int userId = 0;
    int routeId = 0;
    std::string issuedAt;
    bool charged = false;
    int price = 0;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["id"] = id;
        j["userId"] = userId;
        j["routeId"] = routeId;
        j["issuedAt"] = issuedAt;
        j["charged"] = charged;
        j["price"] = price;
        return j;
    }
};

struct RouteVote {
    int routeId = 0;
    std::string day;
    std::string timeSlot;
    std::string destination; // Replaced direction with destination

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["routeId"] = routeId;
        j["day"] = day;
        j["timeSlot"] = timeSlot;
        j["destination"] = destination;
        return j;
    }
};

} // namespace bus

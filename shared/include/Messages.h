#pragma once

namespace bus {

inline constexpr const char* MSG_REGISTER = "REGISTER";
inline constexpr const char* MSG_LOGIN = "LOGIN";
inline constexpr const char* MSG_AUTH_OK = "AUTH_OK";

// Route Management
inline constexpr const char* MSG_GET_ROUTES = "GET_ROUTES";
inline constexpr const char* MSG_ROUTES_LIST = "ROUTES_LIST";
inline constexpr const char* MSG_ADD_ROUTE = "ADD_ROUTE";
inline constexpr const char* MSG_DEL_ROUTE = "DEL_ROUTE";
inline constexpr const char* MSG_ROUTE_OK = "ROUTE_OK";

// Subscriptions
inline constexpr const char* MSG_SUBSCRIBE = "SUBSCRIBE";
inline constexpr const char* MSG_UNSUBSCRIBE = "UNSUBSCRIBE";
inline constexpr const char* MSG_GET_SUBS = "GET_SUBS";
inline constexpr const char* MSG_SUBS_LIST = "SUBS_LIST";

// Rides & Tickets
inline constexpr const char* MSG_RIDE = "RIDE";
inline constexpr const char* MSG_GET_TICKETS = "GET_TICKETS";
inline constexpr const char* MSG_TICKETS = "TICKETS";

// Schedule Management
inline constexpr const char* MSG_UPDATE_SCHEDULE = "UPDATE_SCHEDULE";

// Time Slot Requests (Demand/Voting)
inline constexpr const char* MSG_SET_VOTES = "SET_VOTES";
inline constexpr const char* MSG_GET_VOTES = "GET_VOTES";
inline constexpr const char* MSG_VOTES_LIST = "VOTES_LIST";
inline constexpr const char* MSG_GET_DEMAND = "GET_DEMAND";
inline constexpr const char* MSG_DEMAND_LIST = "DEMAND_LIST";

// Admin
inline constexpr const char* MSG_GET_USERS = "GET_USERS";
inline constexpr const char* MSG_USERS_LIST = "USERS_LIST";
inline constexpr const char* MSG_GET_USER_DETAIL = "get_user_detail";
inline constexpr const char* MSG_USER_DETAIL = "user_detail";

// Generic
inline constexpr const char* MSG_OK = "OK";
inline constexpr const char* MSG_ERROR = "ERROR";

// Roles
inline constexpr const char* ROLE_USER = "user";
inline constexpr const char* ROLE_ADMIN = "admin";

} // namespace bus

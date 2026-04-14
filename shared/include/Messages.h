#pragma once

namespace bus {

// Auth
inline constexpr const char* MSG_REGISTER  = "REGISTER";
inline constexpr const char* MSG_LOGIN     = "LOGIN";
inline constexpr const char* MSG_AUTH_OK   = "AUTH_OK";
inline constexpr const char* MSG_AUTH_FAIL = "AUTH_FAIL";

// Routes
inline constexpr const char* MSG_GET_ROUTES  = "GET_ROUTES";
inline constexpr const char* MSG_ROUTES_LIST = "ROUTES_LIST";
inline constexpr const char* MSG_ADD_ROUTE   = "ADD_ROUTE";
inline constexpr const char* MSG_DEL_ROUTE   = "DEL_ROUTE";
inline constexpr const char* MSG_ROUTE_OK    = "ROUTE_OK";

// Subscriptions
inline constexpr const char* MSG_SUBSCRIBE   = "SUBSCRIBE";
inline constexpr const char* MSG_UNSUBSCRIBE = "UNSUBSCRIBE";
inline constexpr const char* MSG_GET_SUBS    = "GET_SUBS";
inline constexpr const char* MSG_SUBS_LIST   = "SUBS_LIST";

// Rides & Tickets
inline constexpr const char* MSG_RIDE        = "RIDE";
inline constexpr const char* MSG_GET_TICKETS = "GET_TICKETS";
inline constexpr const char* MSG_TICKETS     = "TICKETS";

// Free Time
inline constexpr const char* MSG_SET_FREETIME = "SET_FREETIME";
inline constexpr const char* MSG_GET_FREETIME = "GET_FREETIME";
inline constexpr const char* MSG_FREETIME     = "FREETIME";

// Admin
inline constexpr const char* MSG_GET_USERS  = "GET_USERS";
inline constexpr const char* MSG_USERS_LIST = "USERS_LIST";

// Generic
inline constexpr const char* MSG_OK    = "OK";
inline constexpr const char* MSG_ERROR = "ERROR";

// Roles
inline constexpr const char* ROLE_USER  = "user";
inline constexpr const char* ROLE_ADMIN = "admin";

} // namespace bus

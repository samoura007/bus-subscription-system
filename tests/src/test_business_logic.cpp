#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "INetworkClient.h"
#include "logic/BusinessLogic.h"
#include "logic/DataStore.h"

using namespace bus;
using json = nlohmann::json;
using ::testing::_;
using ::testing::Invoke;

// MockNetworkClient uses GoogleMock to record calls to sendMessage
class MockNetworkClient : public INetworkClient {
public:
    MOCK_METHOD(void, sendMessage, (const json& msg), (override));
};

// Test fixture: creates a DataStore and BusinessLogic for each test
class BusLogicTest : public ::testing::Test {
protected:
    DataStore     store;
    BusinessLogic logic{store};
    MockNetworkClient mock;
    int userId = 0;

    // Helper: register a user and store the assigned userId
    void registerUser(const std::string& name, const std::string& pass) {
        json req;
        req["type"]     = "REGISTER";
        req["username"] = name;
        req["password"] = pass;
        EXPECT_CALL(mock, sendMessage(_)).WillOnce(Invoke([this](const json& r){
            EXPECT_EQ(r["type"], "AUTH_OK");
            userId = r["user"]["id"].get<int>();
        }));
        logic.handleMessage(&mock, userId, req);
    }
};

// ---- Authentication tests ----

TEST_F(BusLogicTest, RegisterNewUser_Succeeds) {
    json req;
    req["type"]     = "REGISTER";
    req["username"] = "alice";
    req["password"] = "secure99";
    EXPECT_CALL(mock, sendMessage(_)).WillOnce(Invoke([](const json& r){
        EXPECT_EQ(r["type"], "AUTH_OK");
        EXPECT_EQ(r["user"]["role"], "user");
    }));
    logic.handleMessage(&mock, userId, req);
}

TEST_F(BusLogicTest, RegisterDuplicateUsername_Fails) {
    registerUser("alice", "pass123");
    json req;
    req["type"]     = "REGISTER";
    req["username"] = "alice";
    req["password"] = "other123";
    EXPECT_CALL(mock, sendMessage(_)).WillOnce(Invoke([](const json& r){
        EXPECT_EQ(r["type"], "ERROR");
    }));
    logic.handleMessage(&mock, userId, req);
}

TEST_F(BusLogicTest, RegisterShortPassword_Fails) {
    json req;
    req["type"]     = "REGISTER";
    req["username"] = "bob";
    req["password"] = "abc";
    EXPECT_CALL(mock, sendMessage(_)).WillOnce(Invoke([](const json& r){
        EXPECT_EQ(r["type"], "ERROR");
    }));
    logic.handleMessage(&mock, userId, req);
}

TEST_F(BusLogicTest, LoginCorrectCredentials_Succeeds) {
    registerUser("charlie", "mypassword");
    json req;
    req["type"]     = "LOGIN";
    req["username"] = "charlie";
    req["password"] = "mypassword";
    EXPECT_CALL(mock, sendMessage(_)).WillOnce(Invoke([](const json& r){
        EXPECT_EQ(r["type"], "AUTH_OK");
    }));
    logic.handleMessage(&mock, userId, req);
}

TEST_F(BusLogicTest, LoginWrongPassword_Fails) {
    registerUser("dave", "correct123");
    json req;
    req["type"]     = "LOGIN";
    req["username"] = "dave";
    req["password"] = "wrongpass";
    EXPECT_CALL(mock, sendMessage(_)).WillOnce(Invoke([](const json& r){
        EXPECT_EQ(r["type"], "ERROR");
    }));
    logic.handleMessage(&mock, userId, req);
}

// ---- Subscription tests ----

TEST_F(BusLogicTest, SubscribeToExistingRoute_Succeeds) {
    registerUser("tester", "pass12");
    int rid = store.addRoute("Route A", "Stop1, Stop2", "7:00");
    json req;
    req["type"]    = "SUBSCRIBE";
    req["routeId"] = rid;
    EXPECT_CALL(mock, sendMessage(_)).WillOnce(Invoke([](const json& r){
        EXPECT_EQ(r["type"], "OK");
    }));
    logic.handleMessage(&mock, userId, req);
    EXPECT_TRUE(store.isSubscribed(userId, rid));
}

TEST_F(BusLogicTest, SubscribeTwiceToSameRoute_Fails) {
    registerUser("tester2", "pass12");
    int rid = store.addRoute("Route B", "", "");
    store.subscribe(userId, rid);
    json req;
    req["type"]    = "SUBSCRIBE";
    req["routeId"] = rid;
    EXPECT_CALL(mock, sendMessage(_)).WillOnce(Invoke([](const json& r){
        EXPECT_EQ(r["type"], "ERROR");
    }));
    logic.handleMessage(&mock, userId, req);
}

TEST_F(BusLogicTest, SubscribeToNonExistentRoute_Fails) {
    registerUser("tester3", "pass12");
    json req;
    req["type"]    = "SUBSCRIBE";
    req["routeId"] = 9999;
    EXPECT_CALL(mock, sendMessage(_)).WillOnce(Invoke([](const json& r){
        EXPECT_EQ(r["type"], "ERROR");
    }));
    logic.handleMessage(&mock, userId, req);
}

// ---- Ticket tests ----

TEST_F(BusLogicTest, RideWithSubscription_NoTicketIssued) {
    registerUser("rider1", "pass12");
    int rid = store.addRoute("Route C", "", "");
    store.subscribe(userId, rid);
    json req;
    req["type"]    = "RIDE";
    req["routeId"] = rid;
    EXPECT_CALL(mock, sendMessage(_)).WillOnce(Invoke([](const json& r){
        EXPECT_FALSE(r.value("ticketIssued", true));
    }));
    logic.handleMessage(&mock, userId, req);
}

TEST_F(BusLogicTest, RideWithoutSubscription_TicketIssued) {
    registerUser("rider2", "pass12");
    int rid = store.addRoute("Route D", "", "");
    json req;
    req["type"]    = "RIDE";
    req["routeId"] = rid;
    EXPECT_CALL(mock, sendMessage(_)).WillOnce(Invoke([](const json& r){
        EXPECT_TRUE(r.value("ticketIssued", false));
        EXPECT_GT(r.value("ticketId", 0), 0);
    }));
    logic.handleMessage(&mock, userId, req);
}

TEST_F(BusLogicTest, MultipleRidesWithoutSubscription_TicketCountIncreases) {
    registerUser("rider3", "pass12");
    int rid = store.addRoute("Route E", "", "");
    json req;
    req["type"]    = "RIDE";
    req["routeId"] = rid;
    EXPECT_CALL(mock, sendMessage(_)).Times(2)
        .WillRepeatedly(Invoke([](const json& r){
            EXPECT_TRUE(r.value("ticketIssued", false));
        }));
    logic.handleMessage(&mock, userId, req);
    logic.handleMessage(&mock, userId, req);
    EXPECT_EQ(store.getTicketsForUser(userId).size(), static_cast<size_t>(2));
}

// ---- Admin tests ----

TEST_F(BusLogicTest, AdminCanAddRoute) {
    int adminId = 1; // seeded admin user
    json req;
    req["type"]     = "ADD_ROUTE";
    req["name"]     = "New Route";
    req["stops"]    = "A, B";
    req["schedule"] = "9:00";
    EXPECT_CALL(mock, sendMessage(_)).WillOnce(Invoke([](const json& r){
        EXPECT_EQ(r["type"], "ROUTE_OK");
    }));
    logic.handleMessage(&mock, adminId, req);
}

TEST_F(BusLogicTest, NonAdminCannotAddRoute) {
    registerUser("normal", "pass12");
    json req;
    req["type"] = "ADD_ROUTE";
    req["name"] = "Hack Route";
    EXPECT_CALL(mock, sendMessage(_)).WillOnce(Invoke([](const json& r){
        EXPECT_EQ(r["type"], "ERROR");
    }));
    logic.handleMessage(&mock, userId, req);
}

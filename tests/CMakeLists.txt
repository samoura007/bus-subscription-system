enable_testing()

add_executable(bus_tests
    src/test_business_logic.cpp
    ${CMAKE_SOURCE_DIR}/server/src/logic/DataStore.cpp
    ${CMAKE_SOURCE_DIR}/server/src/logic/BusinessLogic.cpp
)

target_include_directories(bus_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/shared/include
    ${CMAKE_SOURCE_DIR}/server/src
)

target_link_libraries(bus_tests PRIVATE
    bus_shared
    gtest
    gmock
    gmock_main
    OpenSSL::Crypto
    pthread
)

include(GoogleTest)
gtest_discover_tests(bus_tests)

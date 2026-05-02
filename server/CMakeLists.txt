add_executable(bus_server
    src/main.cpp
    src/Server.cpp
    src/Session.cpp
    src/logic/DataStore.cpp
    src/logic/BusinessLogic.cpp
)
target_include_directories(bus_server PRIVATE
    ${CMAKE_SOURCE_DIR}/shared/include
    src
)
target_link_libraries(bus_server PRIVATE
    bus_shared
    Boost::system
    OpenSSL::Crypto
)

if(WIN32)
    target_link_libraries(bus_server PRIVATE ws2_32 mswsock)
endif()

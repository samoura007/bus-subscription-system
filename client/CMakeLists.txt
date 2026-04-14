set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(bus_client
    src/main.cpp
    src/network/NetworkManager.cpp
    src/views/LoginView.cpp
    src/views/UserDashboard.cpp
    src/views/AdminDashboard.cpp
)

target_include_directories(bus_client PRIVATE
    ${CMAKE_SOURCE_DIR}/shared/include
    src
)

target_link_libraries(bus_client PRIVATE
    bus_shared
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

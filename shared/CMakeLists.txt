add_library(bus_shared INTERFACE)
target_include_directories(bus_shared INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(bus_shared INTERFACE nlohmann_json::nlohmann_json)

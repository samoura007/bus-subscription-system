cmake_minimum_required(VERSION 3.16)
project(BusSubscriptionSystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
qt_standard_project_setup()

find_package(Boost 1.74 REQUIRED COMPONENTS system)
find_package(nlohmann_json 3.10 REQUIRED)
find_package(OpenSSL REQUIRED)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_subdirectory(shared)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(tests)

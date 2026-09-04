cmake_minimum_required(VERSION 3.16)

project(unix_socket_rpc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)

# RPC Library
add_library(rpc
    src/protocol.cpp
    src/rpc_client.cpp
    src/rpc_server.cpp
)

target_include_directories(rpc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rpc
    PUBLIC
        nlohmann_json::nlohmann_json
)


# Server Example
add_executable(server server.cpp
)

target_link_libraries(server
    PRIVATE
        rpc
)


# Client Example
add_executable(client client.cpp
)

target_link_libraries(client
    PRIVATE
        rpc
)
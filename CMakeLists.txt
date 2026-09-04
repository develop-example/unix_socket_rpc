cmake_minimum_required(VERSION 3.16)

project(unix_socket_rpc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server
    server.cpp
)

add_executable(client
    client.cpp
)
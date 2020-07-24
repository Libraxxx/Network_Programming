cmake_minimum_required(VERSION 3.16)
project(ClionPRO)

set(CMAKE_CXX_STANDARD 14)
#link_libraries(ws2_32)
add_executable(client  Client.cpp)
add_executable(server  Server.cpp)
add_executable(client1  Client.cpp)
add_executable(mian  main.cpp)

find_package(Threads REQUIRED)
target_link_libraries(client Threads::Threads)
target_link_libraries(client1 Threads::Threads)
target_link_libraries(server Threads::Threads)


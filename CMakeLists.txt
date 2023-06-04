cmake_minimum_required(VERSION 3.25)
project(cs176a_proj05 C)

set(CMAKE_C_STANDARD 11)

add_library(server server.c server.h)
add_library(client client.c client.h)

add_library(server_tcp server_tcp.c server_tcp.h)
add_library(client_tcp client_tcp.c client_tcp.h)

add_library(util util.c util.h)

add_library(log third_party/log.c third_party/log.h)

add_executable(hangman_server hangman_server.c)
target_link_libraries(hangman_server server server_tcp util log)

add_executable(hangman_client hangman_client.c)
target_link_libraries(hangman_client client client_tcp util log)

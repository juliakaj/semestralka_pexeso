cmake_minimum_required(VERSION 3.18)
project(semestralka_pexeso C)
set(CMAKE_C_STANDARD 99)

find_package(Threads REQUIRED)

add_executable(server server.c pexeso.h)
target_link_libraries(server Threads::Threads)

add_executable(client client.c pexeso.h)
target_link_libraries(client Threads::Threads)

cmake_minimum_required(VERSION 3.11)
project(thread_pool)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-Werror -Wfatal-errors -Wall")

add_library(queue queue.h queue.c)

add_library(thread_pool thread_pool.h thread_pool.c)
target_link_libraries(thread_pool queue pthread)

add_subdirectory(test)
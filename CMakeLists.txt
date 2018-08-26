cmake_minimum_required(VERSION 3.11)
project(thread_pool)
set(CMAKE_C_STANDARD 99)

add_library(queue queue.h queue.c)
add_executable(test_queue test_queue.c)
target_link_libraries(test_queue queue)

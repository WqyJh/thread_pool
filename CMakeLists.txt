cmake_minimum_required(VERSION 3.11)
project(thread_pool)
set(CMAKE_C_STANDARD 99)

add_library(queue queue.h queue.c)
add_executable(test_queue test_queue.c)
target_link_libraries(test_queue queue)

add_library(thread_pool thread_pool.h thread_pool.c)
target_link_libraries(thread_pool queue pthread)

add_executable(test_tls test_tls.c)
target_link_libraries(test_tls thread_pool)

add_executable(test_pool test_pool.c)
target_link_libraries(test_pool thread_pool)

add_executable(test_tp_self test_tp_self.c)
target_link_libraries(test_tp_self thread_pool)


include_directories(..)

add_executable(test_queue test_queue.c)
target_link_libraries(test_queue queue)

add_executable(test_tls test_tls.c)
target_link_libraries(test_tls thread_pool)

add_executable(test_pool test_pool.c)
target_link_libraries(test_pool thread_pool)

add_executable(test_tp_self test_tp_self.c)
target_link_libraries(test_tp_self thread_pool)

add_executable(test_atomic test_atomic.c)
target_link_libraries(test_atomic thread_pool)

add_executable(practice practice.c)
target_link_libraries(practice pthread)

add_custom_target(test
        COMMAND test_queue
        COMMAND test_tls
        COMMAND test_pool
        COMMAND test_tp_self
        COMMAND test_atomic
        COMMAND practice)


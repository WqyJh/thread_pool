cmake_minimum_required(VERSION 2.8.8)
project(thread_pool)
# generating build/compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_FLAGS "-Wall -Werror -Wfatal-errors -Wextra")

set(THREAED_POOL_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

include_directories(${THREAED_POOL_INCLUDE_DIR})

aux_source_directory(./src SRCS)

add_library(thread_pool SHARED ${SRCS})

# Associate header files
set_target_properties(thread_pool PROPERTIES VERSION 1.0 SOVERSION 1)

target_link_libraries(thread_pool pthread)

# make install
install(TARGETS thread_pool
    LIBRARY DESTINATION lib ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
DESTINATION include ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h")

add_subdirectory(test)
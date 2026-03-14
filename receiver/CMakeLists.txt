cmake_minimum_required(VERSION 3.16)

set(RECEIVER_SOURCES
    src/main.cpp
    src/udp_receiver.cpp
)

add_executable(aerotrack_receiver ${RECEIVER_SOURCES})

target_include_directories(aerotrack_receiver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(aerotrack_receiver PRIVATE
    -Wall -Wextra -O2
)

target_link_libraries(aerotrack_receiver PRIVATE pthread)

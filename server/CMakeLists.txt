cmake_minimum_required(VERSION 3.16)

add_executable(aerotrack_server
    src/main.cpp
    src/api_server.cpp
    src/simulation_engine.cpp
    src/range_doppler_processor.cpp
)

target_include_directories(aerotrack_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(aerotrack_server PRIVATE
    -Wall -Wextra -O2
)

target_link_libraries(aerotrack_server PRIVATE aerotrack_core pthread)

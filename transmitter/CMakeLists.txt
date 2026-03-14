cmake_minimum_required(VERSION 3.16)

set(TRANSMITTER_SOURCES
    src/main.cpp
    src/signal_generator.cpp
    src/target_simulator.cpp
    src/noise_generator.cpp
    src/udp_transmitter.cpp
    src/packet_framer.cpp
)

add_executable(aerotrack_transmitter ${TRANSMITTER_SOURCES})

target_include_directories(aerotrack_transmitter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(aerotrack_transmitter PRIVATE
    -Wall -Wextra -O2
)

# Link math library and pthread
target_link_libraries(aerotrack_transmitter PRIVATE m pthread)

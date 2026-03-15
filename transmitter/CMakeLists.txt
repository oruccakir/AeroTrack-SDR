cmake_minimum_required(VERSION 3.16)

# --- Static library: tüm core bileşenler ---
add_library(aerotrack_core STATIC
    src/signal_generator.cpp
    src/target_simulator.cpp
    src/noise_generator.cpp
    src/udp_transmitter.cpp
    src/packet_framer.cpp
)

target_include_directories(aerotrack_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(aerotrack_core PRIVATE
    -Wall -Wextra -O2
)

target_link_libraries(aerotrack_core PRIVATE m pthread)

# --- CLI executable ---
add_executable(aerotrack_transmitter src/main.cpp)

target_compile_options(aerotrack_transmitter PRIVATE
    -Wall -Wextra -O2
)

target_link_libraries(aerotrack_transmitter PRIVATE aerotrack_core)

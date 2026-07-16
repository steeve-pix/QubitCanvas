cmake_minimum_required(VERSION 3.20)

project(QuantumComputer VERSION 0.1.0 LANGUAGES CXX)

add_executable(quantum_sim
        src/main.cpp

        src/math/Complex.cpp
)

target_compile_features(quantum_sim PRIVATE cxx_std_20)

target_include_directories(quantum_sim PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

if (MSVC)
    target_compile_options(quantum_sim PRIVATE /W4 /permissive-)
else ()
    target_compile_options(quantum_sim PRIVATE -Wall -Wextra -Wpedantic)
endif ()
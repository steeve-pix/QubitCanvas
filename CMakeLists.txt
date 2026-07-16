cmake_minimum_required(VERSION 3.20)

project(QuantumComputer VERSION 0.1.0 LANGUAGES CXX)

add_executable(quantumComputer src/main.cpp)

target_compile_features(quantumComputer PRIVATE cxx_std_20)

if (MSVC)
    target_compile_options(quantumComputer PRIVATE /W4 /permissive-)
else ()
    target_compile_options(quantumComputer PRIVATE -Wall -Wextra -Wpedantic)
endif ()
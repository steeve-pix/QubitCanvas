cmake_minimum_required(VERSION 3.20)

project(QuantumComputer VERSION 0.1.0 LANGUAGES CXX)

add_executable(quantumComputer src/main.cpp)

target_compile_features(quantumComputer PRIVATE cxx_std_20)
cmake_minimum_required(VERSION 3.20)

project(QuantumComputer VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

add_library(quantum_sim_core
        src/math/Complex.cpp
)

target_include_directories(quantum_sim_core PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

target_compile_features(quantum_sim_core PUBLIC cxx_std_20)

add_executable(quantum_sim
        src/main.cpp
)

target_link_libraries(quantum_sim PRIVATE
        quantum_sim_core
)

add_executable(complex_tests
        tests/ComplexTests.cpp
)

target_link_libraries(complex_tests PRIVATE quantum_sim_core)

add_test(
        NAME complex_tests
        COMMAND complex_tests
)

if (MSVC)
    target_compile_options(quantum_sim_core PRIVATE /W4 /permissive-)
    target_compile_options(quantum_sim PRIVATE /W4 /permissive-)
    target_compile_options(complex_tests PRIVATE /W4 /permissive-)
else ()
    target_compile_options(quantum_sim_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(quantum_sim PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(complex_tests PRIVATE -Wall -Wextra -Wpedantic)
endif ()
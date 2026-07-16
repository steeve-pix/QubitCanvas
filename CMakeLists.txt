cmake_minimum_required(VERSION 3.20)

project(QuantumComputer VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

add_executable(quantum_sim
        src/main.cpp
        src/math/Complex.cpp
)

target_compile_features(quantum_sim PRIVATE cxx_std_20)

target_include_directories(quantum_sim PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
)

add_executable(complex_tests
        tests/ComplexTests.cpp
        src/math/Complex.cpp
)

target_compile_features(complex_tests PRIVATE cxx_std_20)

target_include_directories(complex_tests PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
)

add_test(
        NAME complex_tests
        COMMAND complex_tests
)

if (MSVC)
    target_compile_options(quantum_sim PRIVATE /W4 /permissive-)
    target_compile_options(complex_tests PRIVATE /W4 /permissive-)
else ()
    target_compile_options(quantum_sim PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(complex_tests PRIVATE -Wall -Wextra -Wpedantic)
endif ()
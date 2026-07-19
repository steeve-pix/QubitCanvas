cmake_minimum_required(VERSION 3.20)

project(QuantumComputer VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

function(enable_project_warnings target_name)
    if (MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    else ()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif ()
endfunction()

# Core simulator library
add_library(quantum_sim_core
        src/math/Complex.cpp
        src/math/ComplexVector.cpp
        src/math/ComplexMatrix.cpp

        src/gates/QuantumGates.cpp

        src/quantum/Qubit.cpp
        src/quantum/QuantumRegister.cpp

        src/circuit/QuantumCircuit.cpp
)

target_include_directories(quantum_sim_core PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_compile_features(quantum_sim_core PUBLIC cxx_std_20)

enable_project_warnings(quantum_sim_core)

# Main simulator application
add_executable(quantum_sim
        src/main.cpp
)

target_link_libraries(quantum_sim PRIVATE quantum_sim_core)

enable_project_warnings(quantum_sim)

# Automated tests
if (BUILD_TESTING)
    add_executable(quantum_sim_tests
            tests/QuantumSimTests.cpp
    )

    target_link_libraries(quantum_sim_tests PRIVATE
            quantum_sim_core
    )

    enable_project_warnings(quantum_sim_tests)

    add_test(
            NAME quantum_sim_tests
            COMMAND quantum_sim_tests
    )
endif ()
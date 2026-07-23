cmake_minimum_required(VERSION 3.20)

project(QubitCanvas VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

function(enable_project_warnings target_name)
    if (MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    else ()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif ()
endfunction()

# Prevent GLFW from building unnecessary extras.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/glfw)

find_package(OpenGL REQUIRED)

# Dear ImGui
add_library(imgui STATIC
        third_party/imgui/imgui.cpp
        third_party/imgui/imgui_demo.cpp
        third_party/imgui/imgui_draw.cpp
        third_party/imgui/imgui_tables.cpp
        third_party/imgui/imgui_widgets.cpp

        third_party/imgui/backends/imgui_impl_glfw.cpp
        third_party/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(
        imgui
        PUBLIC
        third_party/imgui
        third_party/imgui/backends
)

target_link_libraries(
        imgui
        PUBLIC
        glfw
        OpenGL::GL
)

# Core simulator library
add_library(qubit_canvas_core
        src/math/Complex.cpp
        src/math/ComplexVector.cpp
        src/math/ComplexMatrix.cpp

        src/gates/QuantumGates.cpp

        src/quantum/Qubit.cpp
        src/quantum/QuantumRegister.cpp

        src/circuit/QuantumCircuit.cpp

        src/visualization/ConsoleVisualizer.cpp

        src/algorithms/QuantumAlgorithms.cpp

        src/debug/InteractiveCircuitDebugger.cpp
        src/debug/DebuggerSession.cpp
)

target_include_directories(qubit_canvas_core PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_compile_features(qubit_canvas_core PUBLIC cxx_std_20)

enable_project_warnings(qubit_canvas_core)

# Graphical interface library
add_library(qubit_canvas_gui STATIC
        src/gui/GuiApplication.cpp
        src/gui/panels/InspectorPanel.cpp
        src/gui/rendering/CircuitRenderer.cpp
        src/gui/rendering/BlochSphereRenderer.cpp
)

target_include_directories(qubit_canvas_gui PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_compile_features(qubit_canvas_gui PUBLIC cxx_std_20)

target_link_libraries(
        qubit_canvas_gui
        PUBLIC qubit_canvas_core
        PUBLIC imgui
)

enable_project_warnings(qubit_canvas_gui)

# Main application
add_executable(qubit_canvas
        src/main.cpp
)

add_custom_command(
        TARGET qubit_canvas
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:qubit_canvas>/assets"
)

target_link_libraries(
        qubit_canvas
        PRIVATE
        qubit_canvas_core
        qubit_canvas_gui
)

enable_project_warnings(qubit_canvas)

# Automated tests
if (BUILD_TESTING)
    add_executable(qubit_canvas_tests
            tests/QuantumSimTests.cpp
    )

    target_link_libraries(
            qubit_canvas_tests
            PRIVATE qubit_canvas_core
    )

    enable_project_warnings(qubit_canvas_tests)

    add_test(
            NAME qubit_canvas_tests
            COMMAND qubit_canvas_tests
    )
endif ()
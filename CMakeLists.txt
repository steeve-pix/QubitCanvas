cmake_minimum_required(VERSION 3.20)

project(
        QubitCanvas
        VERSION 0.1.0
        DESCRIPTION "Interactive quantum circuit editor and density-volume visualizer"
        HOMEPAGE_URL "https://github.com/steeve-pix/QubitCanvas"
        LANGUAGES C CXX
)

include(CTest)
include(GNUInstallDirs)

option(
        QUBITCANVAS_STATIC_MSVC_RUNTIME
        "Statically link the Visual C++ runtime for portable Windows builds"
        ON
)

option(
        QUBITCANVAS_STATIC_MINGW_RUNTIME
        "Statically link the MinGW runtime for portable Windows builds"
        ON
)

option(
        QUBITCANVAS_WINDOWS_GUI
        "Build the Windows application without an attached console window"
        ON
)

if (MSVC AND QUBITCANVAS_STATIC_MSVC_RUNTIME)
    set(
            CMAKE_MSVC_RUNTIME_LIBRARY
            "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif ()

if (APPLE)
    # OpenGL remains available on macOS but is marked deprecated by the SDK.
    # QubitCanvas intentionally targets the supported OpenGL 3.3 Core surface.
    add_compile_definitions(GL_SILENCE_DEPRECATION)
endif ()

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
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

if (UNIX AND NOT APPLE)
    # Clean Linux runners do not always provide wayland-scanner. The X11
    # backend has a smaller, predictable dependency set for release packages.
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
endif ()

add_subdirectory(third_party/glfw)

find_package(OpenGL REQUIRED)

# GLAD is the single application-level OpenGL function loader.
add_library(glad STATIC
        third_party/glad/src/gl.c
)

target_include_directories(
        glad
        PUBLIC
        third_party/glad/include
)

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
        src/analysis/StateMetrics.cpp
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
        src/project/OpenQasmFile.cpp
        src/project/ProjectFile.cpp
        src/project/ProjectWorkspace.cpp
        src/project/SubcircuitLibrary.cpp
)

target_include_directories(qubit_canvas_core PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_compile_features(qubit_canvas_core PUBLIC cxx_std_20)

enable_project_warnings(qubit_canvas_core)

# Graphical interface library
add_library(qubit_canvas_gui STATIC
        src/gui/ExportFile.cpp
        src/gui/GateNotation.cpp
        src/gui/NativeFileDialog.cpp
        src/gui/QuantumNotation.cpp
        src/gui/SimulationHistoryWorker.cpp
        src/gui/GuiApplication.cpp
        src/gui/panels/InspectorPanel.cpp
        src/gui/rendering/CircuitRenderer.cpp
        src/gui/rendering/BlochSphereRenderer.cpp
        src/gui/rendering/DensityVolumeMath.cpp
        src/gui/rendering/DensityVolumeModel.cpp
        src/gui/rendering/DensityVolumeColorMap.cpp
        src/gui/rendering/DensityVolumeScene.cpp
        src/gui/rendering/DensityVolumeCameraController.cpp
        src/gui/rendering/DensityVolumeRenderer.cpp
        src/gui/panels/GateLibraryPanel.cpp
)

target_include_directories(qubit_canvas_gui PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_compile_features(qubit_canvas_gui PUBLIC cxx_std_20)

target_link_libraries(
        qubit_canvas_gui
        PUBLIC
        qubit_canvas_core
        imgui
        glad
)

if (WIN32)
    target_link_libraries(qubit_canvas_gui PUBLIC comdlg32)
endif ()

enable_project_warnings(qubit_canvas_gui)

# Main application
add_executable(qubit_canvas
        src/main.cpp
)

if (WIN32 AND QUBITCANVAS_WINDOWS_GUI)
    set_target_properties(
            qubit_canvas
            PROPERTIES WIN32_EXECUTABLE TRUE
    )

    if (MSVC)
        target_link_options(
                qubit_canvas
                PRIVATE /ENTRY:mainCRTStartup
        )
    endif ()
endif ()

if (APPLE)
    set_target_properties(
            qubit_canvas
            PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.steeve-pix.QubitCanvas"
            MACOSX_BUNDLE_BUNDLE_NAME "QubitCanvas"
            MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif ()

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

if (MINGW AND QUBITCANVAS_STATIC_MINGW_RUNTIME)
    target_link_options(
            qubit_canvas
            PRIVATE -static -static-libgcc -static-libstdc++
    )
endif ()

# Automated tests
if (BUILD_TESTING)
    add_executable(qubit_canvas_tests
            tests/QuantumSimTests.cpp
    )

    target_link_libraries(
            qubit_canvas_tests
            PRIVATE qubit_canvas_gui
    )

    enable_project_warnings(qubit_canvas_tests)

    add_test(
            NAME qubit_canvas_tests
            COMMAND qubit_canvas_tests
    )
endif ()

# Installation and release archives
install(
        TARGETS qubit_canvas
        RUNTIME DESTINATION .
        BUNDLE DESTINATION .
)

if (APPLE)
    # main() resolves runtime data beside the executable. For a macOS bundle,
    # that executable lives in Contents/MacOS.
    install(
            DIRECTORY assets
            DESTINATION "qubit_canvas.app/Contents/MacOS"
    )
else ()
    install(DIRECTORY assets DESTINATION .)
endif ()

install(FILES README.md 0xthyz.cpp DESTINATION .)

set(CPACK_PACKAGE_NAME "QubitCanvas")
set(CPACK_PACKAGE_VENDOR "steeve-pix")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_GENERATOR "ZIP;TGZ")
set(CPACK_MONOLITHIC_INSTALL ON)

include(CPack)

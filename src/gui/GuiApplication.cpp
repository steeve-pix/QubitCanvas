#include "quantum_sim/gui/GuiApplication.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <cmath>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

namespace quantum_sim::gui {
    GuiApplication::GuiApplication(const circuit::QuantumCircuit &circuit,
                                   const quantum::QuantumRegister &initialState)
        : circuit_{circuit}, session_{circuit, initialState} {
    }

    void GuiApplication::run() {
        if (glfwInit() != GLFW_TRUE) {
            throw std::runtime_error{"Failed to initialize GLFW."};
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow *window = glfwCreateWindow(1280, 720, "QubitCanvas", nullptr, nullptr);

        if (window == nullptr) {
            glfwTerminate();
            throw std::runtime_error{"Failed to create the QubitCanvas window."};
        }

        glfwMakeContextCurrent(window);

        // Synchronize drawing with the monitor refresh rate.
        glfwSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();

        jetBrainsMonoFont_ =
                io.Fonts->AddFontFromFileTTF(
                    "assets/fonts/JetBrainsMono-Regular.ttf",
                    16.0F
                );

        jetBrainsMonoHeadingFont_ =
                io.Fonts->AddFontFromFileTTF(
                    "assets/fonts/JetBrainsMono-Regular.ttf",
                    20.0F
                );

        if (jetBrainsMonoFont_ == nullptr) {
            throw std::runtime_error{
                "Failed to load JetBrains Mono regular font."
            };
        }

        if (jetBrainsMonoHeadingFont_ == nullptr) {
            throw std::runtime_error{
                "Failed to load JetBrains Mono heading font."
            };
        }

        io.FontDefault = jetBrainsMonoFont_;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);

        ImGui_ImplOpenGL3_Init("#version 330");

        while (glfwWindowShouldClose(window) == GLFW_FALSE) {
            // Read operating-system events:
            // keyboard, mouse, resizing and closing.
            glfwPollEvents();

            // Begin a new Dear ImGui frame.
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            const debug::DebuggerSnapshot snapshot =
                    session_.snapshot();

            ImGui::Begin("Circuit");
            circuitRenderer_.draw(circuit_, snapshot);

            const auto selectedInstructionIndex =
                    circuitRenderer_.selectedInstructionIndex();
            ImGui::End();

            // Description of interface for this frame

            ImGui::Text("Quantum Circuit Debugger");

            ImGui::Separator();

            ImGui::Begin("Inspector");
            inspectorPanel_.draw(session_, snapshot, circuit_, selectedInstructionIndex, jetBrainsMonoHeadingFont_);
            ImGui::End();

            ImGui::Begin("Gate Library");

            ImGui::Text("Drag-and-drop gates comming soon.");

            ImGui::End();

            // Interface -> drawable geometry.
            ImGui::Render();

            int framebufferWidth{};
            int framebufferHeight{};

            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

            glViewport(0, 0, framebufferWidth, framebufferHeight);

            // Clear the previous frame.
            glClearColor(0.08F, 0.09F, 0.12F, 1.0F);

            glClear(GL_COLOR_BUFFER_BIT);

            // Draw Dear ImGui's generated geometry.
            ImGui_ImplOpenGL3_RenderDrawData(
                ImGui::GetDrawData()
            );

            // Present the completed frame.
            glfwSwapBuffers(window);
        }
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

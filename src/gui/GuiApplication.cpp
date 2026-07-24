#include "quantum_sim/gui/GuiApplication.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <cmath>
#include <utility>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "quantum_sim/gates/QuantumGates.hpp"

namespace quantum_sim::gui {
    GuiApplication::GuiApplication(circuit::QuantumCircuit &circuit,
                                   const quantum::QuantumRegister &initialState)
        : circuit_{circuit}, initialState_{initialState}, session_{circuit_, initialState_} {
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

            applyQueuedCircuitEdits();

            const debug::DebuggerSnapshot snapshot =
                    session_.snapshot();

            ImGui::Begin("Circuit");
            if (pendingGate_.has_value()) {
                if (
                    pendingGate_.value() == "CX" &&
                    circuitRenderer_.hasPendingControlQubit()
                ) {
                    ImGui::TextColored(
                        ImVec4{0.35F, 0.80F, 1.0F, 1.0F},
                        "Placement mode: CX — choose target qubit"
                    );
                } else if (pendingGate_.value() == "CX") {
                    ImGui::TextColored(
                        ImVec4{0.35F, 0.80F, 1.0F, 1.0F},
                        "Placement mode: CX — choose control qubit"
                    );
                } else {
                    ImGui::TextColored(
                        ImVec4{0.35F, 0.80F, 1.0F, 1.0F},
                        "Placement mode: %s — choose target qubit",
                        pendingGate_->c_str()
                    );
                }

                ImGui::SameLine();

                if (ImGui::SmallButton("Cancel")) {
                    pendingGate_.reset();
                    circuitRenderer_.cancelPlacement();
                }
            }

            if (ImGui::CollapsingHeader("Developer")) {
                ImGui::Checkbox(
                    "Show history debug info",
                    &showHistoryDebugInfo_
                );
            }

            const bool canUndo =
                    !undoHistory_.empty();

            const bool canRedo =
                    !redoHistory_.empty();

            const bool redoShortcutPressed =
                    canRedo &&
                    !io.WantTextInput &&
                    io.KeyCtrl &&
                    ImGui::IsKeyPressed(ImGuiKey_Y) ||
                    (
                        io.KeyShift &&
                        ImGui::IsKeyPressed(ImGuiKey_Z)
                    );

            const bool undoShortcutPressed =
                    canUndo &&
                    !io.WantTextInput &&
                    io.KeyCtrl &&
                    ImGui::IsKeyPressed(ImGuiKey_Z);

            if (!canUndo) {
                ImGui::BeginDisabled();
            }

            const bool undoButtonPressed =
                    ImGui::Button(
                        "Undo last gate  [Ctrl+Z]"
                    );

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Restore the previous circuit edit. Shortcut: Ctrl+Z"
                );
            }

            if (!canUndo) {
                ImGui::EndDisabled();
            }

            if (
                undoButtonPressed ||
                undoShortcutPressed
            ) {
                undoLastCircuitEdit();
            }

            ImGui::SameLine();

            if (!canRedo) {
                ImGui::BeginDisabled();
            }

            const bool redoButtonPressed =
                    ImGui::Button("Redo [Ctrl+Y]");

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Reapply the most recently undone edit.\n"
                    "Shortcuts: Ctrl+Y or Ctrl+Shift+Z"
                );
            }

            if (!canRedo) {
                ImGui::EndDisabled();
            }

            if (redoButtonPressed || redoShortcutPressed) {
                redoLastCircuitEdit();
            }


            if (showHistoryDebugInfo_) {
                ImGui::TextDisabled(
                    "Undo: %zu   Redo: %zu",
                    undoHistory_.size(),
                    redoHistory_.size()
                );
            }

            circuitRenderer_.draw(circuit_, snapshot, pendingGate_);

            const auto singleQubitPlacement =
                    circuitRenderer_.consumeCompletedSingleQubitPlacement();

            if (singleQubitPlacement.has_value()) {
                queuedSingleQubitPlacement_ =
                        std::move(singleQubitPlacement);

                pendingGate_.reset();
            }

            const auto controlledPlacement =
                    circuitRenderer_.consumeCompletedControlledPlacement();

            if (controlledPlacement.has_value()) {
                queuedControlledPlacement_ =
                        controlledPlacement;

                pendingGate_.reset();
            }

            const auto selectedInstructionIndex =
                    circuitRenderer_.selectedInstructionIndex();

            ImGui::End();

            // Description of interface for this frame

            ImGui::Text("Quantum Circuit Debugger");

            ImGui::Separator();

            ImGui::Begin("Inspector");
            const bool jumpToInstruction = inspectorPanel_.draw(session_, snapshot, circuit_, selectedInstructionIndex,
                                                                jetBrainsMonoHeadingFont_);
            if (jumpToInstruction) {
                circuitRenderer_.clearSelection();
            }

            ImGui::End();

            ImGui::Begin("Gate Library");

            gateLibraryPanel_.draw();

            const std::optional<std::string> selectedGate =
                    gateLibraryPanel_.consumeSelectedGate();

            if (selectedGate.has_value()) {
                pendingGate_ = selectedGate.value();
            }

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

    math::ComplexMatrix GuiApplication::createSingleQubitGateMatrix(const std::string &gateName) const {
        if (gateName == "H") {
            return gates::hadamardGate();
        }

        if (gateName == "X") {
            return gates::xGate();
        }

        if (gateName == "Y") {
            return gates::yGate();
        }

        if (gateName == "Z") {
            return gates::zGate();
        }

        throw std::invalid_argument("Unsupported single-qubit gate " + gateName);
    }

    void GuiApplication::applyQueuedCircuitEdits() {
        if (queuedSingleQubitPlacement_.has_value()) {
            const std::string &gateName =
                    queuedSingleQubitPlacement_->gateName;

            const std::size_t targetQubit =
                    queuedSingleQubitPlacement_->targetQubit;

            recordCircuitForUndo();

            circuit_.addSingleQubitGate(
                gateName,
                createSingleQubitGateMatrix(gateName),
                targetQubit
            );

            queuedSingleQubitPlacement_.reset();

            rebuildDebuggerAfterCircuitEdit();
        } else if (queuedControlledPlacement_.has_value()) {
            const std::size_t controlQubit =
                    queuedControlledPlacement_->controlQubit;

            const std::size_t targetQubit =
                    queuedControlledPlacement_->targetQubit;

            recordCircuitForUndo();

            circuit_.addControlledGate(
                "CX",
                gates::cxGate(
                    circuit_.qubitCount(),
                    controlQubit,
                    targetQubit
                ),
                controlQubit,
                targetQubit
            );

            queuedControlledPlacement_.reset();

            rebuildDebuggerAfterCircuitEdit();
        }
    }

    void GuiApplication::undoLastCircuitEdit() {
        if (undoHistory_.empty()) {
            return;
        }

        redoHistory_.push_back(
            circuit_
        );

        circuit_ =
                std::move(
                    undoHistory_.back()
                );

        undoHistory_.pop_back();

        rebuildDebuggerAfterCircuitEdit();

        pendingGate_.reset();
        circuitRenderer_.cancelPlacement();
    }

    void GuiApplication::redoLastCircuitEdit() {
        if (redoHistory_.empty()) {
            return;
        }

        undoHistory_.push_back(
            circuit_
        );

        circuit_ =
                std::move(
                    redoHistory_.back()
                );

        redoHistory_.pop_back();

        rebuildDebuggerAfterCircuitEdit();

        pendingGate_.reset();
        circuitRenderer_.cancelPlacement();
    }

    void GuiApplication::rebuildDebuggerAfterCircuitEdit() {
        session_.rebuild(circuit_, initialState_);
        circuitRenderer_.clearSelection();
    }

    void GuiApplication::recordCircuitForUndo() {
        undoHistory_.push_back(circuit_);
        redoHistory_.clear();
    }
}

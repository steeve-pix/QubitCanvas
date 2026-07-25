#include "quantum_sim/gui/GuiApplication.hpp"
#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"

#define GLFW_INCLUDE_NONE
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <utility>
#include <vector>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "quantum_sim/gates/QuantumGates.hpp"

namespace quantum_sim::gui {
    GuiApplication::GuiApplication(circuit::QuantumCircuit &circuit,
                                   const quantum::QuantumRegister &initialState)
        : circuit_{circuit}, initialState_{initialState}, session_{circuit_, initialState_} {
        settleDebuggerPreview();
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

        const int loadedOpenGlVersion =
                gladLoadGL(
                    [](const char *functionName) -> GLADapiproc {
                        return reinterpret_cast<GLADapiproc>(
                            glfwGetProcAddress(functionName)
                        );
                    }
                );

        if (loadedOpenGlVersion == 0) {
            glfwDestroyWindow(window);
            glfwTerminate();
            throw std::runtime_error{"Failed to initialize GLAD for the QubitCanvas OpenGL context."};
        }

        if (
            GLAD_VERSION_MAJOR(loadedOpenGlVersion) < 3 ||
            (
                GLAD_VERSION_MAJOR(loadedOpenGlVersion) == 3 &&
                GLAD_VERSION_MINOR(loadedOpenGlVersion) < 3
            )
        ) {
            glfwDestroyWindow(window);
            glfwTerminate();
            throw std::runtime_error{"QubitCanvas requires OpenGL 3.3 Core or newer."};
        }

        try {
            qaveCubeRenderer_.initialize();
        } catch (...) {
            glfwDestroyWindow(window);
            glfwTerminate();
            throw;
        }

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

        configureStyle();

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

            // Keep the regular JetBrains Mono face active for the complete UI pass.
            pushApplicationFont();

            applyQueuedCircuitEdits();

            debug::DebuggerSnapshot snapshot =
                    session_.snapshot();

            // Playback can mutate the session, so refresh the snapshot afterward.
            applyPlayback(session_, snapshot);
            snapshot = session_.snapshot();

            drawBackdrop();
            drawTopBar(session_, snapshot);

            const ImGuiViewport *viewport =
                    ImGui::GetMainViewport();

            const ImVec2 workPosition =
                    viewport->WorkPos;

            const ImVec2 workSize =
                    viewport->WorkSize;

            constexpr float topBarHeight = 58.0F;
            constexpr float bottomBarHeight = 28.0F;
            constexpr float gap = 10.0F;
            constexpr float leftPanelWidth = 354.0F;
            constexpr float rightPanelWidth = 390.0F;

            const float usableHeight =
                    std::max(
                        260.0F,
                        workSize.y - topBarHeight - bottomBarHeight - gap * 3.0F
                    );

            const float circuitPanelHeight =
                    usableHeight * 0.45F;

            const float qavePanelHeight =
                    usableHeight - circuitPanelHeight - gap;

            // Center circuit canvas gets all remaining width after fixed side panels.
            const float circuitPanelWidth =
                    std::max(
                        360.0F,
                        workSize.x - leftPanelWidth - rightPanelWidth - gap * 4.0F
                    );

            ImGui::SetNextWindowPos(
                ImVec2{
                    workPosition.x + leftPanelWidth + gap * 2.0F,
                    workPosition.y + topBarHeight + gap
                },
                ImGuiCond_Always
            );

            ImGui::SetNextWindowSize(
                ImVec2{
                    circuitPanelWidth,
                    circuitPanelHeight
                },
                ImGuiCond_Always
            );

            ImGui::Begin("Circuit");
            if (pendingGate_.has_value()) {
                const std::string &gateName =
                        pendingGate_.value();

                const bool hasFirstQubit =
                        circuitRenderer_.hasPendingControlQubit();

                const bool isSwapFamily =
                        gateName == "SWAP" ||
                        gateName == "iSWAP";

                if (isSwapFamily) {
                    ImGui::TextColored(
                        ImVec4{0.35F, 0.80F, 1.0F, 1.0F},
                        hasFirstQubit
                            ? "Placement mode: %s - choose second qubit"
                            : "Placement mode: %s - choose first qubit",
                        gateName.c_str()
                    );
                } else if (
                    gateName == "CX" ||
                    gateName == "CY" ||
                    gateName == "CZ"
                ) {
                    ImGui::TextColored(
                        ImVec4{0.35F, 0.80F, 1.0F, 1.0F},
                        hasFirstQubit
                            ? "Placement mode: %s - choose target qubit"
                            : "Placement mode: %s - choose control qubit",
                        gateName.c_str()
                    );
                } else {
                    ImGui::TextColored(
                        ImVec4{0.35F, 0.80F, 1.0F, 1.0F},
                        "Placement mode: %s - choose target qubit",
                        gateName.c_str()
                    );
                }

                if (pendingRotationAngleRadians_.has_value()) {
                    ImGui::SameLine();
                    ImGui::TextDisabled(
                        "%.3f rad",
                        pendingRotationAngleRadians_.value()
                    );
                }

                ImGui::SameLine();

                if (ImGui::SmallButton("Cancel")) {
                    pendingGate_.reset();
                    pendingRotationAngleRadians_.reset();
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
                    io.KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_Y) || (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)));

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

                queuedSingleQubitRotationAngleRadians_ =
                        pendingRotationAngleRadians_;

                pendingGate_.reset();
                pendingRotationAngleRadians_.reset();
            }

            const auto controlledPlacement =
                    circuitRenderer_.consumeCompletedControlledPlacement();

            if (controlledPlacement.has_value()) {
                queuedControlledPlacement_ =
                        controlledPlacement;

                pendingGate_.reset();
                pendingRotationAngleRadians_.reset();
            }

            const auto selectedInstructionIndex =
                    circuitRenderer_.selectedInstructionIndex();

            const bool canDeleteSelectedInstruction =
                    selectedInstructionIndex.has_value() &&
                    !pendingGate_.has_value();

            if (!canDeleteSelectedInstruction) {
                ImGui::BeginDisabled();
            }

            const bool deleteButtonPressed =
                    ImGui::Button(
                        "Delete selected gate  [Delete]"
                    );

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Remove the selected circuit instruction. Shortcut: Delete"
                );
            }

            const bool deleteShortcutPressed =
                    canDeleteSelectedInstruction &&
                    !io.WantTextInput &&
                    ImGui::IsKeyPressed(ImGuiKey_Delete);

            if (canDeleteSelectedInstruction && (deleteButtonPressed || deleteShortcutPressed)) {
                queuedInstructionDeletion_ =
                        selectedInstructionIndex.value();
            }

            if (!canDeleteSelectedInstruction) {
                ImGui::EndDisabled();
            }

            ImGui::End();

            drawQaveViewport(
                ImVec2{
                    workPosition.x + leftPanelWidth + gap * 2.0F,
                    workPosition.y + topBarHeight + gap * 2.0F + circuitPanelHeight
                },
                ImVec2{
                    circuitPanelWidth,
                    qavePanelHeight
                }
            );

            ImGui::SetNextWindowPos(
                ImVec2{
                    workPosition.x + leftPanelWidth + circuitPanelWidth + gap * 3.0F,
                    workPosition.y + topBarHeight + gap
                },
                ImGuiCond_Always
            );

            ImGui::SetNextWindowSize(
                ImVec2{
                    rightPanelWidth,
                    usableHeight
                },
                ImGuiCond_Always
            );

            ImGui::Begin("Inspector");
            const bool jumpToInstruction = inspectorPanel_.draw(session_, snapshot, circuit_, selectedInstructionIndex,
                                                                jetBrainsMonoHeadingFont_);
            if (jumpToInstruction) {
                circuitRenderer_.clearSelection();
            }

            ImGui::End();

            ImGui::SetNextWindowPos(
                ImVec2{
                    workPosition.x + gap,
                    workPosition.y + topBarHeight + gap
                },
                ImGuiCond_Always
            );

            ImGui::SetNextWindowSize(
                ImVec2{
                    leftPanelWidth,
                    usableHeight
                },
                ImGuiCond_Always
            );

            ImGui::Begin("Gate Library");

            drawAlgorithmScripts();

            ImGui::Spacing();
            ImGui::SeparatorText("Gates");

            gateLibraryPanel_.draw();

            const std::optional<std::string> selectedGate =
                    gateLibraryPanel_.consumeSelectedGate();

            if (selectedGate.has_value()) {
                const std::string &gateName =
                        selectedGate.value();

                pendingGate_ =
                        gateName;

                const bool rotationGate =
                        gateName == "Rx" ||
                        gateName == "Ry" ||
                        gateName == "Rz";

                pendingRotationAngleRadians_ =
                        rotationGate
                            ? std::optional<double>{
                                gateLibraryPanel_.rotationAngleRadians()
                            }
                            : std::nullopt;
            }

            ImGui::End();

            drawBottomStatus(snapshot);

            popApplicationFont();

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

        qaveCubeRenderer_.shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void GuiApplication::configureStyle() const {
        ImGui::StyleColorsDark();

        ImGuiStyle &style =
                ImGui::GetStyle();

        style.WindowRounding = 6.0F;
        style.ChildRounding = 6.0F;
        style.FrameRounding = 5.0F;
        style.PopupRounding = 6.0F;
        style.ScrollbarRounding = 5.0F;
        style.GrabRounding = 5.0F;
        style.TabRounding = 5.0F;
        style.WindowBorderSize = 1.0F;
        style.FrameBorderSize = 1.0F;
        style.WindowPadding = ImVec2{12.0F, 10.0F};
        style.FramePadding = ImVec2{10.0F, 7.0F};
        style.ItemSpacing = ImVec2{8.0F, 8.0F};

        ImVec4 *colors =
                style.Colors;

        colors[ImGuiCol_WindowBg] = ImVec4{0.035F, 0.050F, 0.075F, 0.92F};
        colors[ImGuiCol_ChildBg] = ImVec4{0.035F, 0.050F, 0.075F, 0.78F};
        colors[ImGuiCol_Border] = ImVec4{0.18F, 0.30F, 0.43F, 0.95F};
        colors[ImGuiCol_FrameBg] = ImVec4{0.045F, 0.070F, 0.105F, 0.95F};
        colors[ImGuiCol_FrameBgHovered] = ImVec4{0.070F, 0.145F, 0.205F, 1.0F};
        colors[ImGuiCol_FrameBgActive] = ImVec4{0.120F, 0.200F, 0.270F, 1.0F};
        colors[ImGuiCol_Button] = ImVec4{0.040F, 0.070F, 0.105F, 0.96F};
        colors[ImGuiCol_ButtonHovered] = ImVec4{0.075F, 0.160F, 0.220F, 1.0F};
        colors[ImGuiCol_ButtonActive] = ImVec4{0.130F, 0.230F, 0.295F, 1.0F};
        colors[ImGuiCol_Header] = ImVec4{0.080F, 0.150F, 0.220F, 0.88F};
        colors[ImGuiCol_HeaderHovered] = ImVec4{0.120F, 0.220F, 0.310F, 0.95F};
        colors[ImGuiCol_HeaderActive] = ImVec4{0.150F, 0.270F, 0.360F, 1.0F};
        colors[ImGuiCol_CheckMark] = ImVec4{1.0F, 0.74F, 0.16F, 1.0F};
        colors[ImGuiCol_SliderGrab] = ImVec4{0.34F, 0.74F, 1.0F, 1.0F};
        colors[ImGuiCol_SliderGrabActive] = ImVec4{1.0F, 0.78F, 0.22F, 1.0F};
        colors[ImGuiCol_TitleBg] = ImVec4{0.030F, 0.045F, 0.070F, 0.96F};
        colors[ImGuiCol_TitleBgActive] = ImVec4{0.045F, 0.075F, 0.110F, 0.98F};
    }

    void GuiApplication::pushApplicationFont() const {
        if (jetBrainsMonoFont_ != nullptr) {
            ImGui::PushFont(jetBrainsMonoFont_);
        }
    }

    void GuiApplication::popApplicationFont() const {
        if (jetBrainsMonoFont_ != nullptr) {
            ImGui::PopFont();
        }
    }

    void GuiApplication::drawBackdrop() const {
        const ImGuiViewport *viewport =
                ImGui::GetMainViewport();

        const ImVec2 minimum =
                viewport->Pos;

        const ImVec2 maximum{
            viewport->Pos.x + viewport->Size.x,
            viewport->Pos.y + viewport->Size.y
        };

        ImDrawList *drawList =
                ImGui::GetBackgroundDrawList();

        drawList->AddRectFilledMultiColor(
            minimum,
            maximum,
            IM_COL32(3, 6, 12, 255),
            IM_COL32(7, 17, 29, 255),
            IM_COL32(14, 5, 7, 255),
            IM_COL32(7, 8, 14, 255)
        );

        for (float y = minimum.y; y < maximum.y; y += 4.0F) {
            drawList->AddLine(
                ImVec2{minimum.x, y},
                ImVec2{maximum.x, y},
                IM_COL32(80, 120, 160, 12),
                1.0F
            );
        }
    }

    void GuiApplication::drawQaveViewport(
        const ImVec2 &position,
        const ImVec2 &size
    ) {
        ImGui::SetNextWindowPos(position, ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);

        ImGui::Begin(
            "QAVE 3D",
            nullptr,
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse
        );

        const ImVec2 available =
                ImGui::GetContentRegionAvail();

        const float footerHeight =
                ImGui::GetTextLineHeightWithSpacing();

        const ImVec2 imageSize{
            std::max(1.0F, available.x),
            std::max(1.0F, available.y - footerHeight)
        };

        const ImVec2 framebufferScale =
                ImGui::GetIO().DisplayFramebufferScale;

        const int framebufferWidth =
                std::max(
                    1,
                    static_cast<int>(
                        std::lround(
                            imageSize.x * std::max(framebufferScale.x, 1.0F)
                        )
                    )
                );

        const int framebufferHeight =
                std::max(
                    1,
                    static_cast<int>(
                        std::lround(
                            imageSize.y * std::max(framebufferScale.y, 1.0F)
                        )
                    )
                );

        qaveCubeRenderer_.render(framebufferWidth, framebufferHeight);

        const ImTextureRef qaveTexture{
            static_cast<ImTextureID>(qaveCubeRenderer_.colorTexture())
        };

        // OpenGL framebuffer textures have a bottom-left origin, so the image
        // UVs are vertically flipped for Dear ImGui's top-left coordinate space.
        ImGui::Image(
            qaveTexture,
            imageSize,
            ImVec2{0.0F, 1.0F},
            ImVec2{1.0F, 0.0F}
        );

        ImGui::TextDisabled(
            "OPENGL 3.3 CORE | VAO/VBO/EBO | 36 INDICES | DEPTH TEST"
        );

        ImGui::End();
    }


    void GuiApplication::settleDebuggerPreview() {
        if (
            !session_.hasSteps() ||
            session_.stepCount() < 4U
        ) {
            return;
        }

        session_.moveToStep(
            std::min(
                session_.stepCount() - 1U,
                session_.stepCount() * 2U / 5U
            )
        );
    }

    void GuiApplication::drawTopBar(
        debug::DebuggerSession &session,
        const debug::DebuggerSnapshot &snapshot
    ) {
        const ImGuiViewport *viewport =
                ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(
            ImVec2{
                viewport->WorkSize.x,
                56.0F
            },
            ImGuiCond_Always
        );

        ImGui::Begin(
            "QubitCanvasTopBar",
            nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoScrollbar
        );

        if (jetBrainsMonoHeadingFont_ != nullptr) {
            ImGui::PushFont(jetBrainsMonoHeadingFont_);
        }

        ImGui::TextUnformatted("rho(t)");

        if (jetBrainsMonoHeadingFont_ != nullptr) {
            ImGui::PopFont();
        }

        ImGui::SameLine();
        ImGui::TextDisabled(
            "step %zu/%zu",
            snapshot.stepCount == 0 ? 0 : snapshot.currentStepIndex + 1,
            snapshot.stepCount
        );

        ImGui::SameLine();
        ImGui::TextColored(
            ImVec4{1.0F, 0.76F, 0.18F, 1.0F},
            "%s",
            playbackPaused_ ? "settle" : "running"
        );

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 650.0F);

        if (ImGui::Button(
            canvasMode_ == CanvasMode::LayerStack
                ? "Layer Stack"
                : "Floor Field",
            ImVec2{142.0F, 0.0F}
        )) {
            // Toggle between the two main state-field renderings.
            canvasMode_ =
                    canvasMode_ == CanvasMode::LayerStack
                        ? CanvasMode::FloorField
                        : CanvasMode::LayerStack;
        }

        ImGui::SameLine();

        if (ImGui::Button(
            playbackPaused_ ? "Play" : "Pause",
            ImVec2{98.0F, 0.0F}
        )) {
            playbackPaused_ = !playbackPaused_;
            nextAutoStepAt_ = ImGui::GetTime() + 0.45;
        }

        ImGui::SameLine();

        if (!snapshot.canMoveNext) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Step", ImVec2{90.0F, 0.0F})) {
            session.moveNext();
        }

        if (!snapshot.canMoveNext) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (ImGui::Button("Restart", ImVec2{110.0F, 0.0F})) {
            session.restart();
            playbackPaused_ = true;
        }

        ImGui::SameLine();

        if (ImGui::Button("Sample", ImVec2{110.0F, 0.0F})) {
            sampleCurrentState(snapshot.afterState.get());
        }

        if (snapshot.stepCount > 0) {
            int scrubStep =
                    static_cast<int>(snapshot.currentStepIndex + 1);

            ImGui::SameLine();
            ImGui::SetNextItemWidth(150.0F);

            if (
                ImGui::SliderInt(
                    "##StepScrub",
                    &scrubStep,
                    1,
                    static_cast<int>(snapshot.stepCount)
                )
            ) {
                // Slider is one-based for humans; session indices are zero-based.
                session.moveToStep(
                    static_cast<std::size_t>(
                        std::max(1, scrubStep) - 1
                    )
                );
                playbackPaused_ = true;
            }
        }

        ImGui::End();
    }

    void GuiApplication::drawAlgorithmScripts() {
        ImGui::SeparatorText("Algorithm");

        ImGui::SetNextItemWidth(-1.0F);
        ImGui::SliderInt(
            "Register qubits",
            &presetQubitCount_,
            1,
            10
        );

        const float width =
                ImGui::GetContentRegionAvail().x;

        const float spacing =
                ImGui::GetStyle().ItemSpacing.x;

        const float buttonWidth =
                (width - spacing) * 0.5F;

        const auto scriptButton =
                [&](
                    const char *label,
                    CircuitPreset preset,
                    const char *description,
                    bool sameLine
                ) {
            if (sameLine) {
                ImGui::SameLine();
            }

            if (ImGui::Button(label, ImVec2{buttonWidth, 42.0F})) {
                loadPreset(preset);
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                ImGui::SetTooltip("%s", description);
            }
        };

        scriptButton(
            "Bell",
            CircuitPreset::Bell,
            "2 qubits - maximally entangled Bell pair",
            false
        );
        scriptButton(
            "GHZ",
            CircuitPreset::Ghz,
            "3 qubits - GHZ entanglement chain",
            true
        );
        scriptButton(
            "|+>^n",
            CircuitPreset::PlusRegister,
            "Selected register size - uniform superposition",
            false
        );
        scriptButton(
            "QFT",
            CircuitPreset::Qft,
            "Selected register size - Fourier phase history",
            true
        );
        scriptButton(
            "iQFT",
            CircuitPreset::InverseQft,
            "Selected register size - exact inverse QFT history",
            false
        );
        scriptButton(
            "Grover",
            CircuitPreset::Grover,
            "2 qubits - searches for marked state |11>",
            true
        );
        scriptButton(
            "Deutsch-J",
            CircuitPreset::DeutschJozsa,
            "3 qubits - balanced f(x) = x0 XOR x1",
            false
        );
        scriptButton(
            "Bernstein",
            CircuitPreset::BernsteinVazirani,
            "4 qubits - recovers hidden string 101",
            true
        );
        scriptButton(
            "Toffoli",
            CircuitPreset::Toffoli,
            "3 qubits - decomposed controlled-controlled X",
            false
        );
        scriptButton(
            "Kickback",
            CircuitPreset::Kickback,
            "2 qubits - exposes controlled phase kickback",
            true
        );
        scriptButton(
            "Teleport",
            CircuitPreset::Teleportation,
            "3 qubits - coherent state teleportation to q2",
            false
        );
        scriptButton(
            "Scramble",
            CircuitPreset::Scramble,
            "Selected register size - mixed-gate visualization stress test",
            true
        );

        ImGui::Spacing();
        ImGui::SeparatorText("P Coloring");

        if (ImGui::Button(
            canvasMode_ == CanvasMode::FloorField
                ? "Floor Field"
                : "Layer Stack",
            ImVec2{-1.0F, 0.0F}
        )) {
            canvasMode_ =
                    canvasMode_ == CanvasMode::FloorField
                        ? CanvasMode::LayerStack
                        : CanvasMode::FloorField;
        }

        ImGui::SetNextItemWidth(-1.0F);
        ImGui::SliderFloat(
            "Heat",
            &heatAmount_,
            0.35F,
            1.35F,
            "%.2f"
        );
    }

    void GuiApplication::applyPlayback(
        debug::DebuggerSession &session,
        const debug::DebuggerSnapshot &snapshot
    ) {
        if (
            playbackPaused_ ||
            snapshot.stepCount == 0
        ) {
            return;
        }

        const double now =
                ImGui::GetTime();

        if (now < nextAutoStepAt_) {
            return;
        }

        if (snapshot.canMoveNext) {
            session.moveNext();
        } else {
            session.restart();
        }

        nextAutoStepAt_ = now + 0.72;
    }

    void GuiApplication::loadPreset(CircuitPreset preset) {
        circuit_ =
                createPresetCircuit(preset);

        // Presets always start in the all-zero basis state for predictable demos.
        initialState_ =
                quantum::QuantumRegister::basisState(
                    circuit_.qubitCount(),
                    0
                );

        undoHistory_.clear();
        redoHistory_.clear();

        // Clear transient edit state so old placements do not leak into new circuits.
        pendingGate_.reset();
        pendingRotationAngleRadians_.reset();
        queuedControlledPlacement_.reset();
        queuedSingleQubitPlacement_.reset();
        queuedSingleQubitRotationAngleRadians_.reset();
        queuedInstructionDeletion_.reset();
        circuitRenderer_.cancelPlacement();
        circuitRenderer_.clearSelection();
        playbackPaused_ = true;

        rebuildDebuggerAfterCircuitEdit();
        settleDebuggerPreview();
    }

    circuit::QuantumCircuit GuiApplication::createPresetCircuit(CircuitPreset preset) const {
        const std::size_t qubitCount =
                static_cast<std::size_t>(
                    std::clamp(presetQubitCount_, 1, 10)
                );

        if (preset == CircuitPreset::Bell) {
            return algorithms::bellStateCircuit();
        }

        if (preset == CircuitPreset::Ghz) {
            return algorithms::ghzStateCircuit();
        }

        if (preset == CircuitPreset::PlusRegister) {
            return algorithms::equalSuperpositionCircuit(qubitCount);
        }

        if (preset == CircuitPreset::Qft) {
            return algorithms::qftCircuit(qubitCount);
        }

        if (preset == CircuitPreset::InverseQft) {
            return algorithms::inverseQftCircuit(qubitCount);
        }

        if (preset == CircuitPreset::Grover) {
            return algorithms::groverSearchCircuit();
        }

        if (preset == CircuitPreset::DeutschJozsa) {
            return algorithms::deutschJozsaCircuit();
        }

        if (preset == CircuitPreset::BernsteinVazirani) {
            return algorithms::bernsteinVaziraniCircuit(3, 0b101);
        }

        if (preset == CircuitPreset::Toffoli) {
            return algorithms::toffoliDemoCircuit();
        }

        if (preset == CircuitPreset::Kickback) {
            return algorithms::phaseKickbackCircuit();
        }

        if (preset == CircuitPreset::Teleportation) {
            return algorithms::teleportationCircuit();
        }

        if (preset == CircuitPreset::Scramble) {
            return algorithms::scrambleCircuit(qubitCount);
        }

        throw std::invalid_argument{
            "Unsupported circuit preset."
        };
    }

    void GuiApplication::sampleCurrentState(const quantum::QuantumRegister &state) {
        quantum::QuantumRegister sampledState =
                state;

        // Copy before measuring so sampling does not mutate the debugger state.
        const std::size_t measuredIndex =
                sampledState.measure(randomEngine_);

        lastSampleLabel_ =
                state.basisStateLabel(measuredIndex);
    }

    void GuiApplication::drawBottomStatus(const debug::DebuggerSnapshot &snapshot) const {
        const ImGuiViewport *viewport =
                ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(
            ImVec2{
                viewport->WorkPos.x,
                viewport->WorkPos.y + viewport->WorkSize.y - 28.0F
            },
            ImGuiCond_Always
        );

        ImGui::SetNextWindowSize(
            ImVec2{
                viewport->WorkSize.x,
                28.0F
            },
            ImGuiCond_Always
        );

        ImGui::Begin(
            "QubitCanvasBottomBar",
            nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoScrollbar
        );

        ImGui::TextDisabled("| outcome");
        ImGui::SameLine();
        ImGui::TextColored(
            ImVec4{0.48F, 0.74F, 1.0F, 1.0F},
            "%s",
            lastSampleLabel_.c_str()
        );

        ImGui::SameLine();
        ImGui::TextDisabled("| states %zu", snapshot.afterState.get().stateCount());

        ImGui::SameLine();
        ImGui::TextDisabled(
            "| %.0f FPS",
            ImGui::GetIO().Framerate
        );

        ImGui::SameLine();
        ImGui::TextDisabled(
            "| %s",
            canvasMode_ == CanvasMode::FloorField
                ? "floor field"
                : "layer stack"
        );

        ImGui::End();
    }

    math::ComplexMatrix GuiApplication::createSingleQubitGateMatrix(
        const std::string &gateName,
        const std::optional<double> angleRadians
    ) const {
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

        if (gateName == "S") {
            return gates::sGate();
        }

        if (gateName == "T") {
            return gates::tGate();
        }

        if (
            gateName == "Rx" ||
            gateName == "Ry" ||
            gateName == "Rz"
        ) {
            if (!angleRadians.has_value()) {
                throw std::invalid_argument(
                    gateName + " requires an angle in radians."
                );
            }

            if (gateName == "Rx") {
                return gates::rxGate(angleRadians.value());
            }

            if (gateName == "Ry") {
                return gates::ryGate(angleRadians.value());
            }

            return gates::rzGate(angleRadians.value());
        }

        throw std::invalid_argument("Unsupported single-qubit gate " + gateName);
    }

    void GuiApplication::applyQueuedCircuitEdits() {
        if (queuedInstructionDeletion_.has_value()) {
            const std::size_t instructionIndex =
                    queuedInstructionDeletion_.value();

            recordCircuitForUndo();

            // Remove first, then rebuild only if the index was valid.
            const bool removed =
                    circuit_.removeInstruction(instructionIndex);

            queuedInstructionDeletion_.reset();

            if (removed) {
                rebuildDebuggerAfterCircuitEdit();
                circuitRenderer_.cancelPlacement();
            }

            return;
        }

        if (queuedSingleQubitPlacement_.has_value()) {
            const std::string &gateName =
                    queuedSingleQubitPlacement_->gateName;

            const std::size_t targetQubit =
                    queuedSingleQubitPlacement_->targetQubit;

            const std::optional<double> angleRadians =
                    queuedSingleQubitRotationAngleRadians_;

            recordCircuitForUndo();

            // Placement stores the insertion slot picked by the circuit renderer.
            const std::size_t instructionIndex =
                    queuedSingleQubitPlacement_->instructionIndex;

            circuit_.insertSingleQubitGate(
                instructionIndex,
                gateName,
                createSingleQubitGateMatrix(gateName, angleRadians),
                targetQubit,
                angleRadians
            );

            queuedSingleQubitPlacement_.reset();
            queuedSingleQubitRotationAngleRadians_.reset();

            rebuildDebuggerAfterCircuitEdit();
        } else if (queuedControlledPlacement_.has_value()) {
            const std::string &gateName =
                    queuedControlledPlacement_->gateName;
            const std::size_t controlQubit =
                    queuedControlledPlacement_->controlQubit;

            const std::size_t targetQubit =
                    queuedControlledPlacement_->targetQubit;

            const std::size_t instructionIndex =
                    queuedControlledPlacement_->instructionIndex;

            recordCircuitForUndo();

            // Controlled, CZ, SWAP, and iSWAP all enter the circuit as full-register matrices.
            circuit_.insertControlledGate(
                instructionIndex,
                gateName,
                createControlledGateMatrix(
                    gateName,
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

        // Move the latest undo snapshot back into the active circuit.
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

        // Symmetric with undo: active state returns to undo history first.
        circuit_ =
                std::move(
                    redoHistory_.back()
                );

        redoHistory_.pop_back();

        rebuildDebuggerAfterCircuitEdit();

        pendingGate_.reset();
        circuitRenderer_.cancelPlacement();
    }

    math::ComplexMatrix GuiApplication::createControlledGateMatrix(const std::string &gateName,
                                                                   std::size_t controlQubit,
                                                                   std::size_t targetQubit) const {
        if (gateName == "CX") {
            return gates::cxGate(
                circuit_.qubitCount(),
                controlQubit,
                targetQubit
            );
        }

        if (gateName == "CY") {
            return gates::cyGate(
                circuit_.qubitCount(),
                controlQubit,
                targetQubit
            );
        }

        if (gateName == "CZ") {
            return gates::czGate(
                circuit_.qubitCount(),
                controlQubit,
                targetQubit
            );
        }

        if (gateName == "SWAP") {
            return gates::swapGate(
                circuit_.qubitCount(),
                controlQubit,
                targetQubit
            );
        }

        if (gateName == "iSWAP") {
            return gates::iSwapGate(
                circuit_.qubitCount(),
                controlQubit,
                targetQubit);
        }

        throw std::invalid_argument(
            "Unsupported controlled gate: " + gateName
        );
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

#include "quantum_sim/gui/GuiApplication.hpp"
#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <numbers>
#include <sstream>
#include <utility>
#include <vector>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "quantum_sim/gates/QuantumGates.hpp"

namespace quantum_sim::gui {
    namespace {
        /**
         * Screen-space point and depth produced by the light-weight volume camera.
         */
        struct ProjectedVolumePoint {
            ImVec2 screen{};
            float depth{};
        };

        /**
         * Strongest amplitude bucket used by density-matrix render cells.
         */
        struct DensityBinSample {
            std::size_t firstState{};
            std::size_t lastState{};
            std::size_t strongestState{};
            double probability{};
            double phase{};
        };

        /**
         * Draw payload for one matrix cell in the 3D layer stack.
         */
        struct DensityLayerCell {
            float depth{};
            ImVec2 center{};
            float size{};
            ImU32 color{};
            ImU32 glowColor{};
            float glowRadius{};
        };

        /**
         * Clamps a floating color channel before packing it into ImGui's RGBA format.
         */
        [[nodiscard]] int colorChannel(float value) {
            return static_cast<int>(
                std::clamp(value, 0.0F, 255.0F)
            );
        }

        /**
         * Picks a matrix resolution that preserves small states and buckets huge ones.
         */
        [[nodiscard]] std::size_t densityMatrixDimension(std::size_t stateCount, std::size_t maximumDimension) {
            if (stateCount == 0U) {
                return 1U;
            }

            return std::clamp<std::size_t>(
                stateCount,
                2U,
                maximumDimension
            );
        }

        /**
         * Builds one row/column bucket for a density-matrix visualization.
         */
        [[nodiscard]] DensityBinSample densityBin(
            const quantum::QuantumRegister &state,
            const std::size_t binIndex,
            const std::size_t binCount
        ) {
            const std::size_t stateCount =
                    state.stateCount();

            const std::size_t firstState =
                    std::min(
                        stateCount - 1U,
                        binIndex * stateCount / binCount
                    );

            const std::size_t lastState =
                    std::min(
                        stateCount,
                        std::max<std::size_t>(
                            firstState + 1U,
                            ((binIndex + 1U) * stateCount + binCount - 1U) /
                            binCount
                        )
                    );

            std::size_t strongestState =
                    firstState;

            double probability =
                    0.0;

            double phase =
                    0.0;

            for (std::size_t stateIndex = firstState; stateIndex < lastState; ++stateIndex) {
                const double candidateProbability =
                        state.probability(stateIndex);

                if (candidateProbability >= probability) {
                    strongestState =
                            stateIndex;

                    probability =
                            candidateProbability;

                    const auto &amplitude =
                            state.amplitude(stateIndex);

                    phase =
                            std::atan2(
                                amplitude.imaginary(),
                                amplitude.real()
                            );
                }
            }

            return DensityBinSample{
                firstState,
                lastState,
                strongestState,
                probability,
                phase
            };
        }

        /**
         * Precomputes density-matrix buckets for one quantum state.
         */
        [[nodiscard]] std::vector<DensityBinSample> densityBins(
            const quantum::QuantumRegister &state,
            const std::size_t binCount
        ) {
            std::vector<DensityBinSample> bins;
            bins.reserve(binCount);

            for (std::size_t bin = 0; bin < binCount; ++bin) {
                bins.push_back(
                    densityBin(state, bin, binCount)
                );
            }

            return bins;
        }

        /**
         * Converts one density-matrix cell into the purple/magenta/amber QAVE palette.
         */
        [[nodiscard]] ImU32 densityMatrixColor(
            const double magnitude,
            const double phase,
            const float alphaScale = 1.0F
        ) {
            const float intensity =
                    std::clamp(
                        static_cast<float>(std::pow(magnitude, 0.45)),
                        0.0F,
                        1.0F
                    );

            const float phaseT =
                    static_cast<float>(
                        (phase + std::numbers::pi) /
                        (2.0 * std::numbers::pi)
                    );

            const float amber =
                    std::clamp(
                        (intensity - 0.72F) / 0.28F,
                        0.0F,
                        1.0F
                    );

            const int red =
                    colorChannel(30.0F + intensity * 176.0F + amber * 72.0F);

            const int green =
                    colorChannel(13.0F + intensity * 42.0F + amber * 168.0F);

            const int blue =
                    colorChannel(
                        72.0F + (1.0F - intensity) * 84.0F +
                        std::cos(phaseT * 6.28318F) * 28.0F -
                        amber * 48.0F
                    );

            const int alpha =
                    colorChannel((72.0F + intensity * 176.0F) * alphaScale);

            return IM_COL32(red, green, blue, alpha);
        }

        /**
         * Projects one 3D point through a tiny orbit camera into ImGui coordinates.
         */
        [[nodiscard]] ProjectedVolumePoint projectVolumePoint(
            float x,
            float y,
            float z,
            float yaw,
            float pitch,
            float scale,
            const ImVec2 &origin
        ) {
            const float yawCosine =
                    std::cos(yaw);

            const float yawSine =
                    std::sin(yaw);

            const float pitchCosine =
                    std::cos(pitch);

            const float pitchSine =
                    std::sin(pitch);

            // Yaw spins the slab around its vertical axis.
            const float yawedX =
                    x * yawCosine - z * yawSine;

            const float yawedZ =
                    x * yawSine + z * yawCosine;

            // Pitch tilts the slab so stacked amplitudes read as a volume.
            const float pitchedY =
                    y * pitchCosine - yawedZ * pitchSine;

            const float depth =
                    y * pitchSine + yawedZ * pitchCosine;

            return ProjectedVolumePoint{
                ImVec2{
                    origin.x + yawedX * scale,
                    origin.y - pitchedY * scale
                },
                depth
            };
        }
    }

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

            drawBackdrop(session_, snapshot);
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
                    usableHeight * 0.45F
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

            // Description of interface for this frame

            ImGui::Text("Quantum Circuit Debugger");

            ImGui::Separator();

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
                pendingGate_ = selectedGate.value();
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

    void GuiApplication::drawBackdrop(
        const debug::DebuggerSession &session,
        const debug::DebuggerSnapshot &snapshot
    ) {
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
            // Scanlines add a stable instrument-panel texture behind the 3D field.
            drawList->AddLine(
                ImVec2{minimum.x, y},
                ImVec2{maximum.x, y},
                IM_COL32(80, 120, 160, 12),
                1.0F
            );
        }

        constexpr float topBarHeight = 58.0F;
        constexpr float bottomBarHeight = 28.0F;
        constexpr float gap = 10.0F;
        constexpr float leftPanelWidth = 354.0F;
        constexpr float rightPanelWidth = 390.0F;

        const ImVec2 workPosition =
                viewport->WorkPos;

        const ImVec2 workSize =
                viewport->WorkSize;

        const float usableHeight =
                std::max(
                    260.0F,
                    workSize.y - topBarHeight - bottomBarHeight - gap * 3.0F
                );

        const float centerPanelWidth =
                std::max(
                    360.0F,
                    workSize.x - leftPanelWidth - rightPanelWidth - gap * 4.0F
                );

        const ImVec2 renderMinimum{
            workPosition.x + leftPanelWidth + gap * 2.0F,
            workPosition.y + topBarHeight + gap
        };

        const ImVec2 renderMaximum{
            workPosition.x + leftPanelWidth + gap * 2.0F + centerPanelWidth,
            workPosition.y + topBarHeight + usableHeight
        };

        if (
            renderMaximum.x > renderMinimum.x + 64.0F &&
            renderMaximum.y > renderMinimum.y + 64.0F
        ) {
            handleRenderCameraInput(renderMinimum, renderMaximum);
        }

        const ImVec2 visualMinimum{
            workPosition.x + leftPanelWidth + gap * 2.0F - 96.0F,
            workPosition.y + topBarHeight + gap
        };

        const ImVec2 visualMaximum{
            workPosition.x + leftPanelWidth + gap * 2.0F + centerPanelWidth + 72.0F,
            workPosition.y + topBarHeight + usableHeight
        };

        const ImVec2 stageMinimum{
            visualMinimum.x,
            visualMinimum.y
        };

        const ImVec2 stageMaximum{
            visualMaximum.x,
            visualMaximum.y
        };

        drawList->AddRectFilledMultiColor(
            stageMinimum,
            stageMaximum,
            IM_COL32(1, 2, 5, 246),
            IM_COL32(2, 4, 8, 242),
            IM_COL32(34, 6, 8, 232),
            IM_COL32(8, 2, 7, 238)
        );

        for (float y = stageMinimum.y; y < stageMaximum.y; y += 3.0F) {
            drawList->AddLine(
                ImVec2{stageMinimum.x, y},
                ImVec2{stageMaximum.x, y},
                IM_COL32(92, 58, 84, 14),
                1.0F
            );
        }

        const quantum::QuantumRegister &state =
                snapshot.afterState.get();

        const std::size_t stateCount =
                state.stateCount();

        if (stateCount == 0) {
            return;
        }

        const bool layerStack =
                canvasMode_ == CanvasMode::LayerStack;

        const std::size_t sourceLayerCount =
                session.hasSteps()
                    ? session.stepCount()
                    : 1U;

        const std::size_t layerCount =
                layerStack
                    ? std::clamp<std::size_t>(
                        std::max<std::size_t>(sourceLayerCount, 18U),
                        8U,
                        56U
                    )
                    : 1U;

        const std::size_t matrixDimension =
                std::max<std::size_t>(
                    4U,
                    densityMatrixDimension(stateCount, 16U)
                );

        const float renderWidth =
                std::max(
                    1.0F,
                    visualMaximum.x - visualMinimum.x
                );

        const float renderHeight =
                std::max(
                    1.0F,
                    visualMaximum.y - visualMinimum.y
                );

        const float scale =
                std::clamp(
                    std::min(renderWidth / 620.0F, renderHeight / 290.0F),
                    0.78F,
                    1.48F
                ) * renderZoom_;

        const ImVec2 origin{
            visualMinimum.x + renderWidth * 0.38F + renderPan_.x,
            stageMinimum.y + (stageMaximum.y - stageMinimum.y) * 0.53F + renderPan_.y
        };

        const float pitch =
                renderPitch_ + (layerStack ? 0.0F : 0.24F);

        const float layerSpacing =
                layerStack ? 4.6F : 8.8F;

        const float matrixSpacing =
                layerStack ? 5.2F : 7.6F;

        const float halfX =
                (static_cast<float>(layerCount) - 1.0F) * layerSpacing * 0.5F;

        const float halfMatrix =
                (static_cast<float>(matrixDimension) - 1.0F) * matrixSpacing * 0.5F;

        const std::array<ProjectedVolumePoint, 8> corners{
            projectVolumePoint(-halfX, -halfMatrix, -halfMatrix, renderYaw_, pitch, scale, origin),
            projectVolumePoint(halfX, -halfMatrix, -halfMatrix, renderYaw_, pitch, scale, origin),
            projectVolumePoint(halfX, halfMatrix, -halfMatrix, renderYaw_, pitch, scale, origin),
            projectVolumePoint(-halfX, halfMatrix, -halfMatrix, renderYaw_, pitch, scale, origin),
            projectVolumePoint(-halfX, -halfMatrix, halfMatrix, renderYaw_, pitch, scale, origin),
            projectVolumePoint(halfX, -halfMatrix, halfMatrix, renderYaw_, pitch, scale, origin),
            projectVolumePoint(halfX, halfMatrix, halfMatrix, renderYaw_, pitch, scale, origin),
            projectVolumePoint(-halfX, halfMatrix, halfMatrix, renderYaw_, pitch, scale, origin)
        };

        const std::array<std::pair<int, int>, 12> edges{
            std::pair<int, int>{0, 1},
            std::pair<int, int>{1, 2},
            std::pair<int, int>{2, 3},
            std::pair<int, int>{3, 0},
            std::pair<int, int>{4, 5},
            std::pair<int, int>{5, 6},
            std::pair<int, int>{6, 7},
            std::pair<int, int>{7, 4},
            std::pair<int, int>{0, 4},
            std::pair<int, int>{1, 5},
            std::pair<int, int>{2, 6},
            std::pair<int, int>{3, 7}
        };

        for (const auto &[firstCorner, secondCorner] : edges) {
            drawList->AddLine(
                corners[static_cast<std::size_t>(firstCorner)].screen,
                corners[static_cast<std::size_t>(secondCorner)].screen,
                IM_COL32(94, 118, 155, 28),
                1.0F
            );
        }

        const float time =
                static_cast<float>(ImGui::GetTime());

        const std::size_t currentLayer =
                std::min(
                    layerCount - 1U,
                    sourceLayerCount == 0U
                        ? 0U
                        : snapshot.currentStepIndex * layerCount / sourceLayerCount
                );

        const float currentLayerX =
                static_cast<float>(currentLayer) * layerSpacing - halfX;

        const ProjectedVolumePoint highlightA =
                projectVolumePoint(currentLayerX, -halfMatrix, -halfMatrix, renderYaw_, pitch, scale, origin);

        const ProjectedVolumePoint highlightB =
                projectVolumePoint(currentLayerX, halfMatrix, -halfMatrix, renderYaw_, pitch, scale, origin);

        const ProjectedVolumePoint highlightC =
                projectVolumePoint(currentLayerX, halfMatrix, halfMatrix, renderYaw_, pitch, scale, origin);

        const ProjectedVolumePoint highlightD =
                projectVolumePoint(currentLayerX, -halfMatrix, halfMatrix, renderYaw_, pitch, scale, origin);

        drawList->AddQuadFilled(
            highlightA.screen,
            highlightB.screen,
            highlightC.screen,
            highlightD.screen,
            IM_COL32(255, 177, 36, 38)
        );

        drawList->AddQuad(
            highlightA.screen,
            highlightB.screen,
            highlightC.screen,
            highlightD.screen,
            IM_COL32(255, 194, 67, 118),
            1.2F
        );

        std::vector<DensityLayerCell> cells;
        cells.reserve(layerCount * matrixDimension * matrixDimension);

        const float depthNormalizer =
                halfX + halfMatrix * 2.0F + 1.0F;

        for (std::size_t layer = 0; layer < layerCount; ++layer) {
            const std::size_t traceIndex =
                    session.hasSteps()
                        ? std::min(
                            sourceLayerCount - 1U,
                            layer * sourceLayerCount / layerCount
                        )
                        : 0U;

            const quantum::QuantumRegister &layerState =
                    session.hasSteps()
                        ? session.stepAt(traceIndex).state
                        : state;

            const std::vector<DensityBinSample> bins =
                    densityBins(layerState, matrixDimension);

            const float layerX =
                    static_cast<float>(layer) * layerSpacing - halfX;

            const float layerDistance =
                    std::abs(
                        static_cast<float>(layer) -
                        static_cast<float>(currentLayer)
                    );

            const float focusBoost =
                    std::clamp(
                        1.0F - layerDistance / 12.0F,
                        0.18F,
                        1.0F
                    );

            for (std::size_t row = 0; row < matrixDimension; ++row) {
                for (std::size_t column = 0; column < matrixDimension; ++column) {
                    const double rawMagnitude =
                            std::sqrt(
                                bins[row].probability *
                                bins[column].probability
                            );

                    const double normalizedMagnitude =
                            std::clamp(
                                rawMagnitude * std::sqrt(static_cast<double>(stateCount)),
                                0.0,
                                1.0
                            );

                    const double phase =
                            bins[row].phase - bins[column].phase;

                    const bool diagonal =
                            row == column;

                    const float shimmer =
                            0.92F + 0.08F * std::sin(
                                time * 0.9F +
                                static_cast<float>(layer) * 0.19F +
                                static_cast<float>(row + column) * 0.11F
                            );

                    const float alphaScale =
                            std::clamp(
                                (0.30F + focusBoost * 0.70F) * shimmer,
                                0.18F,
                                1.0F
                            );

                    const float x =
                            layerX;

                    const float y =
                            halfMatrix - static_cast<float>(row) * matrixSpacing;

                    const float z =
                            static_cast<float>(column) * matrixSpacing - halfMatrix;

                    const ProjectedVolumePoint projected =
                            projectVolumePoint(
                                x,
                                y,
                                z,
                                renderYaw_,
                                pitch,
                                scale,
                                origin
                            );

                    const float depthLight =
                            std::clamp(
                                (projected.depth + depthNormalizer) /
                                (depthNormalizer * 2.0F),
                                0.0F,
                                1.0F
                            );

                    const float cellSize =
                            std::max(
                                1.2F,
                                matrixSpacing * scale *
                                (diagonal ? 0.92F : 0.82F)
                            );

                    const float glowMagnitude =
                            std::clamp(
                                static_cast<float>(normalizedMagnitude - 0.76) / 0.24F,
                                0.0F,
                                1.0F
                            );

                    const ImU32 color =
                            normalizedMagnitude <= 0.002
                                ? IM_COL32(58, 22, 112, colorChannel(86.0F * alphaScale))
                                : densityMatrixColor(
                                    normalizedMagnitude,
                                    phase,
                                    alphaScale * (0.72F + depthLight * 0.42F)
                                );

                    cells.push_back(
                        DensityLayerCell{
                            projected.depth,
                            projected.screen,
                            cellSize,
                            color,
                            IM_COL32(255, 197, 91, colorChannel(58.0F * glowMagnitude * alphaScale)),
                            glowMagnitude > 0.0F ? cellSize * (1.5F + glowMagnitude) : 0.0F
                        }
                    );
                }
            }
        }

        std::sort(
            cells.begin(),
            cells.end(),
            [](const DensityLayerCell &left, const DensityLayerCell &right) {
                return left.depth < right.depth;
            }
        );

        for (const DensityLayerCell &cell : cells) {
            if (cell.glowRadius > 0.0F) {
                drawList->AddRectFilled(
                    ImVec2{
                        cell.center.x - cell.glowRadius,
                        cell.center.y - cell.glowRadius
                    },
                    ImVec2{
                        cell.center.x + cell.glowRadius,
                        cell.center.y + cell.glowRadius
                    },
                    cell.glowColor,
                    2.4F
                );
            }

            const ImVec2 cellMinimum{
                cell.center.x - cell.size * 0.5F,
                cell.center.y - cell.size * 0.5F
            };

            const ImVec2 cellMaximum{
                cell.center.x + cell.size * 0.5F,
                cell.center.y + cell.size * 0.5F
            };

            drawList->AddRectFilled(
                cellMinimum,
                cellMaximum,
                cell.color,
                0.8F
            );
        }

        const float overlayWidth =
                std::clamp(
                    renderWidth * 0.42F,
                    240.0F,
                    330.0F
                );

        const float overlayGrid =
                overlayWidth - 24.0F;

        const float overlayHeight =
                overlayGrid + 54.0F;

        const ImVec2 overlayMinimum{
            visualMaximum.x - overlayWidth - 18.0F,
            stageMaximum.y - overlayHeight - 52.0F
        };

        const ImVec2 overlayMaximum{
            overlayMinimum.x + overlayWidth,
            overlayMinimum.y + overlayHeight
        };

        drawList->AddRectFilled(
            overlayMinimum,
            overlayMaximum,
            IM_COL32(7, 12, 22, 232),
            7.0F
        );

        drawList->AddRect(
            overlayMinimum,
            overlayMaximum,
            IM_COL32(54, 98, 143, 190),
            7.0F,
            0,
            1.0F
        );

        const std::size_t overlayDimension =
                densityMatrixDimension(stateCount, 32U);

        const std::vector<DensityBinSample> overlayBins =
                densityBins(state, overlayDimension);

        const std::string overlayHeader =
                "P - 2D - LAYER " +
                std::to_string(snapshot.stepCount == 0 ? 0 : snapshot.currentStepIndex + 1U) +
                " - " +
                std::to_string(overlayDimension) +
                "x" +
                std::to_string(overlayDimension);

        drawList->AddText(
            ImVec2{overlayMinimum.x + 12.0F, overlayMinimum.y + 8.0F},
            IM_COL32(139, 160, 193, 255),
            overlayHeader.c_str()
        );

        const ImVec2 overlayGridMinimum{
            overlayMinimum.x + 12.0F,
            overlayMinimum.y + 32.0F
        };

        const float overlayCell =
                overlayGrid / static_cast<float>(overlayDimension);

        for (std::size_t row = 0; row < overlayDimension; ++row) {
            for (std::size_t column = 0; column < overlayDimension; ++column) {
                const double magnitude =
                        std::clamp(
                            std::sqrt(
                                overlayBins[row].probability *
                                overlayBins[column].probability
                            ) * std::sqrt(static_cast<double>(stateCount)),
                            0.0,
                            1.0
                        );

                const double phase =
                        overlayBins[row].phase - overlayBins[column].phase;

                const ImVec2 cellMinimum{
                    overlayGridMinimum.x + static_cast<float>(column) * overlayCell,
                    overlayGridMinimum.y + static_cast<float>(row) * overlayCell
                };

                const ImVec2 cellMaximum{
                    overlayGridMinimum.x + static_cast<float>(column + 1U) * overlayCell - 0.7F,
                    overlayGridMinimum.y + static_cast<float>(row + 1U) * overlayCell - 0.7F
                };

                drawList->AddRectFilled(
                    cellMinimum,
                    cellMaximum,
                    magnitude <= 0.002
                        ? IM_COL32(30, 17, 69, 184)
                        : densityMatrixColor(magnitude, phase, 0.96F),
                    0.0F
                );
            }
        }

        for (std::size_t line = 0; line <= overlayDimension; ++line) {
            const float offset =
                    static_cast<float>(line) * overlayCell;

            drawList->AddLine(
                ImVec2{overlayGridMinimum.x + offset, overlayGridMinimum.y},
                ImVec2{overlayGridMinimum.x + offset, overlayGridMinimum.y + overlayGrid},
                IM_COL32(76, 96, 137, 42),
                1.0F
            );

            drawList->AddLine(
                ImVec2{overlayGridMinimum.x, overlayGridMinimum.y + offset},
                ImVec2{overlayGridMinimum.x + overlayGrid, overlayGridMinimum.y + offset},
                IM_COL32(76, 96, 137, 42),
                1.0F
            );
        }

        drawList->AddLine(
            overlayGridMinimum,
            ImVec2{
                overlayGridMinimum.x + overlayGrid,
                overlayGridMinimum.y + overlayGrid
            },
            IM_COL32(218, 148, 50, 100),
            1.1F
        );

        drawList->AddText(
            ImVec2{visualMinimum.x + 18.0F, stageMaximum.y - 36.0F},
            IM_COL32(255, 197, 53, 235),
            layerStack ? "layer stack:" : "floor field:"
        );

        drawList->AddText(
            ImVec2{visualMinimum.x + 128.0F, stageMaximum.y - 36.0F},
            IM_COL32(104, 121, 156, 210),
            layerStack
                ? "full history density slices - one rho matrix per circuit step"
                : "single rho matrix floor view"
        );

        drawList->AddText(
            ImVec2{visualMaximum.x - 168.0F, stageMaximum.y - 18.0F},
            IM_COL32(104, 121, 156, 210),
            "|P| low-high"
        );

        constexpr int legendSteps = 44;
        const float legendWidth = 82.0F;
        const ImVec2 legendMinimum{
            visualMaximum.x - 260.0F,
            stageMaximum.y - 16.0F
        };

        for (int step = 0; step < legendSteps; ++step) {
            const float t =
                    static_cast<float>(step) /
                    static_cast<float>(legendSteps - 1);

            drawList->AddRectFilled(
                ImVec2{
                    legendMinimum.x + legendWidth * t,
                    legendMinimum.y
                },
                ImVec2{
                    legendMinimum.x +
                    legendWidth * static_cast<float>(step + 1) /
                    static_cast<float>(legendSteps),
                    legendMinimum.y + 8.0F
                },
                densityMatrixColor(t * t, 0.0, 1.0F),
                0.0F
            );
        }
    }

    void GuiApplication::handleRenderCameraInput(const ImVec2 &minimum, const ImVec2 &maximum) {
        const ImVec2 size{
            maximum.x - minimum.x,
            maximum.y - minimum.y
        };

        if (
            size.x <= 0.0F ||
            size.y <= 0.0F
        ) {
            return;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
        ImGui::SetNextWindowPos(minimum, ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);

        ImGui::Begin(
            "QubitCanvasStateRenderSurface",
            nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoBringToFrontOnFocus
        );

        ImGui::SetCursorScreenPos(minimum);
        ImGui::InvisibleButton(
            "##StateVolumeCamera",
            size,
            ImGuiButtonFlags_MouseButtonLeft |
            ImGuiButtonFlags_MouseButtonMiddle |
            ImGuiButtonFlags_MouseButtonRight
        );

        const bool hovered =
                ImGui::IsItemHovered();

        const ImGuiIO &io =
                ImGui::GetIO();

        if (hovered) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        }

        const bool middleMouseDragging =
                ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0F);

        const bool altLeftMouseDragging =
                io.KeyAlt &&
                ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0F);

        const bool blenderOrbitDrag =
                ImGui::IsItemActive() &&
                !io.KeyShift &&
                !io.KeyCtrl &&
                (
                    middleMouseDragging ||
                    altLeftMouseDragging
                );

        const bool blenderPanDrag =
                ImGui::IsItemActive() &&
                io.KeyShift &&
                !io.KeyCtrl &&
                (
                    middleMouseDragging ||
                    altLeftMouseDragging
                );

        const bool blenderDollyDrag =
                ImGui::IsItemActive() &&
                io.KeyCtrl &&
                !io.KeyShift &&
                (
                    middleMouseDragging ||
                    altLeftMouseDragging
                );

        if (blenderOrbitDrag) {
            // Blender-style orbit: MMB drag rotates around the state volume.
            renderYaw_ +=
                    io.MouseDelta.x * 0.008F;

            renderPitch_ =
                    std::clamp(
                        renderPitch_ + io.MouseDelta.y * 0.006F,
                        -0.88F,
                        0.82F
                    );
        }

        if (blenderPanDrag) {
            // Blender-style pan: Shift+MMB moves the view in screen space.
            renderPan_.x +=
                    io.MouseDelta.x;

            renderPan_.y +=
                    io.MouseDelta.y;
        }

        if (blenderDollyDrag) {
            renderZoom_ =
                    std::clamp(
                        renderZoom_ * std::pow(1.012F, -io.MouseDelta.y),
                        0.48F,
                        2.35F
                    );
        }

        if (hovered && io.MouseWheel != 0.0F) {
            // Wheel zoom mirrors Blender's viewport dolly without needing a drag.
            renderZoom_ =
                    std::clamp(
                        renderZoom_ * std::pow(1.12F, io.MouseWheel),
                        0.48F,
                        2.35F
                    );
        }

        if (
            hovered &&
            (
                ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Middle) ||
                (
                    io.KeyAlt &&
                    ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)
                )
            )
        ) {
            resetRenderCamera();
        }

        if (hovered) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("MMB orbit  |  Shift+MMB pan  |  Ctrl+MMB zoom");
            ImGui::TextUnformatted("Alt+LMB mirrors MMB");
            ImGui::EndTooltip();
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void GuiApplication::resetRenderCamera() {
        renderYaw_ =
                -0.46F;

        renderPitch_ =
                0.24F;

        renderZoom_ =
                1.30F;

        renderPan_ =
                ImVec2{0.0F, 0.0F};
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
                [&](const char *label, CircuitPreset preset, bool sameLine) {
            if (sameLine) {
                ImGui::SameLine();
            }

            if (ImGui::Button(label, ImVec2{buttonWidth, 42.0F})) {
                loadPreset(preset);
            }
        };

        scriptButton("Bell", CircuitPreset::Bell, false);
        scriptButton("GHZ", CircuitPreset::Ghz, true);
        scriptButton("|+>^n", CircuitPreset::PlusRegister, false);
        scriptButton("QFT", CircuitPreset::Qft, true);
        scriptButton("Phase", CircuitPreset::PhaseLadder, false);
        scriptButton("Entangle", CircuitPreset::EntangleChain, true);
        scriptButton("Scramble", CircuitPreset::Scramble, false);

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
        queuedControlledPlacement_.reset();
        queuedSingleQubitPlacement_.reset();
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

        // The classic demos keep their canonical qubit counts.
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

        circuit::QuantumCircuit presetCircuit{qubitCount};

        if (preset == CircuitPreset::EntangleChain) {
            presetCircuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                0
            );

            for (std::size_t qubit = 1; qubit < qubitCount; ++qubit) {
                // Chain each qubit into the previous one to build GHZ-like correlation.
                presetCircuit.addControlledGate(
                    "CX",
                    gates::cxGate(qubitCount, qubit - 1, qubit),
                    qubit - 1,
                    qubit
                );
            }

            return presetCircuit;
        }

        for (std::size_t qubit = 0; qubit < qubitCount; ++qubit) {
            // Phase and scramble presets begin from broad superposition.
            presetCircuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                qubit
            );
        }

        if (preset == CircuitPreset::PhaseLadder) {
            for (std::size_t qubit = 0; qubit < qubitCount; ++qubit) {
                presetCircuit.addSingleQubitGate(
                    qubit % 2 == 0 ? "S" : "T",
                    qubit % 2 == 0 ? gates::sGate() : gates::tGate(),
                    qubit
                );
            }

            for (std::size_t qubit = 1; qubit < qubitCount; ++qubit) {
                presetCircuit.addControlledGate(
                    "CZ",
                    gates::czGate(qubitCount, qubit - 1, qubit),
                    qubit - 1,
                    qubit
                );
            }

            return presetCircuit;
        }

        for (std::size_t qubit = 0; qubit < qubitCount; ++qubit) {
            if (qubit % 3 == 0) {
                presetCircuit.addSingleQubitGate(
                    "X",
                    gates::xGate(),
                    qubit
                );
            }

            if (qubit % 3 == 1) {
                presetCircuit.addSingleQubitGate(
                    "S",
                    gates::sGate(),
                    qubit
                );
            }
        }

        for (std::size_t qubit = 0; qubit + 1 < qubitCount; ++qubit) {
            presetCircuit.addControlledGate(
                qubit % 2 == 0 ? "CX" : "CZ",
                qubit % 2 == 0
                    ? gates::cxGate(qubitCount, qubit, qubit + 1)
                    : gates::czGate(qubitCount, qubit, qubit + 1),
                qubit,
                qubit + 1
            );
        }

        if (qubitCount > 2) {
            presetCircuit.addControlledGate(
                "SWAP",
                gates::swapGate(qubitCount, 0, qubitCount - 1),
                0,
                qubitCount - 1
            );
        }

        return presetCircuit;
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

        if (gateName == "S") {
            return gates::sGate();
        }

        if (gateName == "T") {
            return gates::tGate();
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

            recordCircuitForUndo();

            // Placement stores the insertion slot picked by the circuit renderer.
            const std::size_t instructionIndex =
                    queuedSingleQubitPlacement_->instructionIndex;

            circuit_.insertSingleQubitGate(
                instructionIndex,
                gateName,
                createSingleQubitGateMatrix(gateName),
                targetQubit
            );

            queuedSingleQubitPlacement_.reset();

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

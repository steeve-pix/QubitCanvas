#include "quantum_sim/gui/GuiApplication.hpp"
#include "quantum_sim/gui/NativeFileDialog.hpp"
#include "quantum_sim/gui/QuantumNotation.hpp"
#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"
#include "quantum_sim/project/ProjectFile.hpp"

#define GLFW_INCLUDE_NONE
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <array>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <numbers>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "quantum_sim/gates/QuantumGates.hpp"

namespace {
    [[nodiscard]] bool usesGateParameters(
        const std::string_view gateName
    ) noexcept {
        return gateName == "P" ||
               gateName == "U" ||
               gateName == "Rx" ||
               gateName == "Ry" ||
               gateName == "Rz" ||
               gateName == "CP" ||
               gateName == "CRx" ||
               gateName == "CRy" ||
               gateName == "CRz" ||
               gateName == "RXX" ||
               gateName == "RYY" ||
               gateName == "RZZ" ||
               gateName == "fSim";
    }

    [[nodiscard]] bool isControlledGate(
        const std::string_view gateName
    ) noexcept {
        return gateName == "CX" ||
               gateName == "CY" ||
               gateName == "CZ" ||
               gateName == "CH" ||
               gateName == "CS" ||
               gateName == "CSdg" ||
               gateName == "CT" ||
               gateName == "CTdg" ||
               gateName == "CP" ||
               gateName == "CRx" ||
               gateName == "CRy" ||
               gateName == "CRz";
    }

    [[nodiscard]] bool isSymmetricTwoQubitGate(
        const std::string_view gateName
    ) noexcept {
        return gateName == "SWAP" ||
               gateName == "iSWAP" ||
               gateName == "sqrtSWAP" ||
               gateName == "DCX" ||
               gateName == "ECR" ||
               gateName == "fSim" ||
               gateName == "RXX" ||
               gateName == "RYY" ||
               gateName == "RZZ";
    }

    [[nodiscard]] bool isThreeQubitGate(
        const std::string_view gateName
    ) noexcept {
        return gateName == "CCX" ||
               gateName == "CSWAP";
    }

    [[nodiscard]] bool writeFramebufferPpm(
        const std::string &path,
        const int width,
        const int height,
        const std::vector<unsigned char> &pixels
    ) {
        std::ofstream output{
            path,
            std::ios::binary
        };

        if (!output) {
            return false;
        }

        output
                << "P6\n"
                << width
                << ' '
                << height
                << "\n255\n";

        const std::size_t rowByteCount =
                static_cast<std::size_t>(width) *
                3U;

        // OpenGL's framebuffer origin is lower-left; PPM viewers expect top-left.
        for (int row = height; row-- > 0;) {
            const std::size_t rowOffset =
                    static_cast<std::size_t>(row) *
                    rowByteCount;

            output.write(
                reinterpret_cast<const char *>(
                    pixels.data() +
                    rowOffset
                ),
                static_cast<std::streamsize>(
                    rowByteCount
                )
            );
        }

        return static_cast<bool>(output);
    }
}

namespace quantum_sim::gui {
    GuiApplication::GuiApplication(
        circuit::QuantumCircuit &circuit,
        const quantum::QuantumRegister &initialState,
        GuiLaunchOptions launchOptions
    )
        : circuit_{circuit},
          initialState_{initialState},
          session_{circuit_, initialState_},
          launchOptions_{std::move(launchOptions)} {
        presetQubitCount_ =
                static_cast<int>(
                    std::clamp(
                        circuit_.qubitCount(),
                        std::size_t{1},
                        std::size_t{10}
                    )
                );

        rebuildDensityVolume();

        if (launchOptions_.algorithmPage.has_value()) {
                algorithmPage_ =
                    std::min(
                        launchOptions_.algorithmPage.value(),
                        std::size_t{3}
                    );
        }

        if (launchOptions_.gatePage.has_value()) {
            gateLibraryPanel_.setPage(
                launchOptions_.gatePage.value()
            );
        }

        if (launchOptions_.armedGate.has_value()) {
            armGatePlacement(
                launchOptions_.armedGate.value()
            );
        }

        if (launchOptions_.startAtFinalStep) {
            session_.moveToStepNumber(
                session_.stepCount()
            );
        }

        if (launchOptions_.startInFloorField) {
            canvasMode_ = CanvasMode::FloorField;
        }

        isolateDensityLayer_ =
                launchOptions_.isolateDensityLayer;

        if (launchOptions_.comparisonDensityLayer.has_value()) {
            compareDensityLayers_ = true;
            comparisonDensityLayer_ =
                    launchOptions_.comparisonDensityLayer.value();
        }

        synchronizeDensityLayer(session_.snapshot());
    }

    void GuiApplication::run() {
        if (glfwInit() != GLFW_TRUE) {
            throw std::runtime_error{"Failed to initialize GLFW."};
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        if (launchOptions_.hiddenWindow) {
            glfwWindowHint(
                GLFW_VISIBLE,
                GLFW_FALSE
            );
        }

        const int initialWindowWidth =
                launchOptions_.capturePath.has_value()
                    ? 1920
                    : 1280;

        const int initialWindowHeight =
                launchOptions_.capturePath.has_value()
                    ? 1080
                    : 720;

        GLFWwindow *window = glfwCreateWindow(
            initialWindowWidth,
            initialWindowHeight,
            "QubitCanvas",
            nullptr,
            nullptr
        );

        if (window == nullptr) {
            glfwTerminate();
            throw std::runtime_error{"Failed to create the QubitCanvas window."};
        }

        // QubitCanvas uses three dense technical work areas; maximize the
        // initial window so display scaling cannot hide the inspector.
        if (!launchOptions_.hiddenWindow) {
            glfwMaximizeWindow(window);
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
            densityVolumeRenderer_.initialize();
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

        static constexpr ImWchar jetBrainsMonoGlyphRanges[]{
            0x0020, 0x00FF,
            0x03B8, 0x03B8,
            0x03BB, 0x03BB,
            0x03C0, 0x03C1,
            0x03C6, 0x03C6,
            0x03C8, 0x03C8,
            0x2020, 0x2020,
            0x207F, 0x207F,
            0x2190, 0x2192,
            0x2212, 0x2212,
            0x221A, 0x221A,
            0x2220, 0x2220,
            0x27E8, 0x27E9,
            0
        };

        jetBrainsMonoFont_ =
                io.Fonts->AddFontFromFileTTF(
                    "assets/fonts/JetBrainsMono-Regular.ttf",
                    16.0F,
                    nullptr,
                    jetBrainsMonoGlyphRanges
                );

        jetBrainsMonoHeadingFont_ =
                io.Fonts->AddFontFromFileTTF(
                    "assets/fonts/JetBrainsMono-Regular.ttf",
                    20.0F,
                    nullptr,
                    jetBrainsMonoGlyphRanges
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

        if (!launchOptions_.hiddenWindow) {
            const bool previousSessionWasUnclean =
                    projectWorkspace_.beginSession();

            projectWorkspaceSessionActive_ = true;
            recentProjectPaths_ =
                    projectWorkspace_.recentProjects();

            reusableSubcircuits_ =
                    subcircuitLibrary_.loadAll();

            recoveryPromptPending_ =
                    projectWorkspace_.recoveryAvailable();

            if (
                previousSessionWasUnclean &&
                !recoveryPromptPending_
            ) {
                projectStatusMessage_ =
                        "The previous session ended unexpectedly.";
            }

            nextAutosaveAt_ = ImGui::GetTime() + 8.0;
        }

        bool captureFailed = false;

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

            handleGlobalShortcuts();
            applyQueuedProjectOpen();
            applyQueuedPreset();
            applyQueuedCircuitEdits();
            adoptCompletedSimulationHistory();
            autosaveProjectIfDue();

            debug::DebuggerSnapshot snapshot =
                    session_.snapshot();

            // Playback can mutate the session, so refresh the snapshot afterward.
            applyPlayback(session_, snapshot);
            snapshot = session_.snapshot();
            synchronizeDensityLayer(snapshot);

            drawBackdrop();
            drawTopBar(session_, snapshot);
            drawRecoveryPrompt();
            snapshot = session_.snapshot();
            synchronizeDensityLayer(snapshot);

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

            const float activeRightPanelWidth =
                    circuitFocusMode_
                        ? 0.0F
                        : rightPanelWidth;

            const float usableHeight =
                    std::max(
                        260.0F,
                        workSize.y - topBarHeight - bottomBarHeight - gap * 3.0F
                    );

            const float splitPanelHeight =
                    (
                        usableHeight -
                        gap
                    ) *
                    0.5F;

            const float circuitPanelHeight =
                    circuitFocusMode_
                        ? usableHeight
                        : splitPanelHeight;

            const float densityVolumePanelHeight =
                    splitPanelHeight;

            // Center circuit canvas gets all remaining width after fixed side panels.
            const float circuitPanelWidth =
                    std::max(
                        360.0F,
                        workSize.x -
                        leftPanelWidth -
                        activeRightPanelWidth -
                        gap *
                        (
                            circuitFocusMode_
                                ? 3.0F
                                : 4.0F
                        )
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

                const std::size_t selectedOperandCount =
                        circuitRenderer_.placementOperandCount();

                const bool isSwapFamily =
                        isSymmetricTwoQubitGate(gateName);

                if (isThreeQubitGate(gateName)) {
                    const char *prompt =
                            selectedOperandCount == 0U
                                ? (
                                    gateName == "CCX"
                                        ? "choose first control"
                                        : "choose control"
                                )
                                : selectedOperandCount == 1U
                                    ? (
                                        gateName == "CCX"
                                            ? "choose second control"
                                            : "choose first swap qubit"
                                    )
                                    : (
                                        gateName == "CCX"
                                            ? "choose target"
                                            : "choose second swap qubit"
                                    );

                    ImGui::TextColored(
                        ImVec4{0.35F, 0.80F, 1.0F, 1.0F},
                        "Placement mode: %s - %s",
                        gateName.c_str(),
                        prompt
                    );
                } else if (isSwapFamily) {
                    ImGui::TextColored(
                        ImVec4{0.35F, 0.80F, 1.0F, 1.0F},
                        hasFirstQubit
                            ? "Placement mode: %s - choose second qubit"
                            : "Placement mode: %s - choose first qubit",
                        gateName.c_str()
                    );
                } else if (isControlledGate(gateName)) {
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

                if (pendingGateParameters_.has_value()) {
                    const std::string angleText =
                            notation::formatAngleMeasurement(
                                pendingGateParameters_->thetaRadians
                            );

                    ImGui::SameLine();
                    ImGui::TextDisabled(
                        "%s",
                        angleText.c_str()
                    );

                    if (gateName == "U") {
                        const std::string phiText =
                                notation::formatAngleMeasurement(
                                    pendingGateParameters_->phiRadians
                                );

                        const std::string lambdaText =
                                notation::formatAngleMeasurement(
                                    pendingGateParameters_->lambdaRadians
                                );

                        ImGui::SameLine();
                        ImGui::TextDisabled(
                            "\xCF\x86 %s  \xCE\xBB %s",
                            phiText.c_str(),
                            lambdaText.c_str()
                        );
                    } else if (gateName == "fSim") {
                        const std::string phiText =
                                notation::formatAngleMeasurement(
                                    pendingGateParameters_->phiRadians
                                );

                        ImGui::SameLine();
                        ImGui::TextDisabled(
                            "\xCF\x86 %s",
                            phiText.c_str()
                        );
                    }
                }

                ImGui::SameLine();

                if (ImGui::SmallButton("Cancel")) {
                    cancelGatePlacement();
                }
            }

            if (
                ImGui::Button(
                    circuitFocusMode_
                        ? "Show visualizers"
                        : "Focus editor"
                )
            ) {
                circuitFocusMode_ =
                        !circuitFocusMode_;
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    circuitFocusMode_
                        ? "Restore the Density Volume and Inspector panels."
                        : "Give the circuit the full center workspace."
                );
            }

            ImGui::SameLine();
            ImGui::Checkbox(
                "Follow edits",
                &followManualEdits_
            );

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Preview the state immediately after each manual edit.\n"
                    "Algorithms still open at step 0."
                );
            }

            ImGui::SameLine();

            if (ImGui::Button("-##CircuitZoom")) {
                circuitRenderer_.zoomOut();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Zoom out. Ctrl+wheel also zooms the circuit."
                );
            }

            ImGui::SameLine();

            if (ImGui::Button("Fit")) {
                circuitRenderer_.fitToView();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Fit the complete circuit timeline."
                );
            }

            ImGui::SameLine();

            if (ImGui::Button("+##CircuitZoom")) {
                circuitRenderer_.zoomIn();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Zoom in. Ctrl+wheel also zooms the circuit."
                );
            }

            ImGui::SameLine();
            ImGui::TextDisabled(
                circuitRenderer_.isFittingToView()
                    ? "AUTO"
                    : "%d%%",
                static_cast<int>(
                    std::lround(
                        circuitRenderer_.viewZoom() *
                        100.0F
                    )
                )
            );

            const bool canUndo =
                    !undoHistory_.empty();

            const bool canRedo =
                    !redoHistory_.empty();

            const auto toolbarSelectedInstructionIndex =
                    circuitRenderer_.selectedInstructionIndex();

            const std::vector<std::size_t> &toolbarSelectedInstructionIndices =
                    circuitRenderer_.selectedInstructionIndices();

            const bool canDeleteSelectedInstruction =
                    !toolbarSelectedInstructionIndices.empty() &&
                    !pendingGate_.has_value();

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
                        "Undo edit  [Ctrl+Z]"
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

            ImGui::SameLine();

            if (!canDeleteSelectedInstruction) {
                ImGui::BeginDisabled();
            }

            const std::string deleteLabel =
                    toolbarSelectedInstructionIndices.size() > 1U
                        ? "Delete selected (" +
                          std::to_string(
                              toolbarSelectedInstructionIndices.size()
                          ) +
                          ")  [Delete]"
                        : "Delete selected gate  [Delete]";

            const bool deleteButtonPressed =
                    ImGui::Button(deleteLabel.c_str());

            if (
                ImGui::IsItemHovered(
                    ImGuiHoveredFlags_AllowWhenDisabled
                )
            ) {
                ImGui::SetTooltip(
                    canDeleteSelectedInstruction
                        ? "Remove every selected circuit instruction in one undoable edit."
                        : "Select one or more circuit gates before deleting them."
                );
            }

            if (!canDeleteSelectedInstruction) {
                ImGui::EndDisabled();
            }

            const bool deleteShortcutPressed =
                    canDeleteSelectedInstruction &&
                    !io.WantTextInput &&
                    ImGui::IsKeyPressed(ImGuiKey_Delete);

            if (
                canDeleteSelectedInstruction &&
                (
                    deleteButtonPressed ||
                    deleteShortcutPressed
                )
            ) {
                queuedInstructionDeletions_ =
                        toolbarSelectedInstructionIndices;
            }

            ImGui::SameLine();

            const bool canClearCircuit =
                    circuit_.instructionCount() > 0U &&
                    !pendingGate_.has_value();

            if (!canClearCircuit) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("Clear circuit")) {
                queuedClearCircuit_ = true;
            }

            if (
                ImGui::IsItemHovered(
                    ImGuiHoveredFlags_AllowWhenDisabled
                )
            ) {
                ImGui::SetTooltip(
                    canClearCircuit
                        ? "Remove every gate in one undoable edit."
                        : "The circuit is already empty."
                );
            }

            if (!canClearCircuit) {
                ImGui::EndDisabled();
            }

            const bool canMoveSelectedLeft =
                    toolbarSelectedInstructionIndices.size() == 1U &&
                    toolbarSelectedInstructionIndex.has_value() &&
                    toolbarSelectedInstructionIndex.value() > 0U &&
                    !pendingGate_.has_value();

            const bool canMoveSelectedRight =
                    toolbarSelectedInstructionIndices.size() == 1U &&
                    toolbarSelectedInstructionIndex.has_value() &&
                    toolbarSelectedInstructionIndex.value() + 1U <
                        circuit_.instructionCount() &&
                    !pendingGate_.has_value();

            ImGui::SameLine();

            if (!canMoveSelectedLeft) {
                ImGui::BeginDisabled();
            }

            const bool moveLeftPressed =
                    ImGui::Button("<##MoveGateLeft");

            if (ImGui::IsItemHovered(
                ImGuiHoveredFlags_AllowWhenDisabled
            )) {
                ImGui::SetTooltip(
                    canMoveSelectedLeft
                        ? "Move the selected gate one step earlier."
                        : "Select a gate that is not already first."
                );
            }

            if (!canMoveSelectedLeft) {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();

            if (!canMoveSelectedRight) {
                ImGui::BeginDisabled();
            }

            const bool moveRightPressed =
                    ImGui::Button(">##MoveGateRight");

            if (ImGui::IsItemHovered(
                ImGuiHoveredFlags_AllowWhenDisabled
            )) {
                ImGui::SetTooltip(
                    canMoveSelectedRight
                        ? "Move the selected gate one step later."
                        : "Select a gate that is not already last."
                );
            }

            if (!canMoveSelectedRight) {
                ImGui::EndDisabled();
            }

            if (moveLeftPressed && canMoveSelectedLeft) {
                queuedInstructionMove_ =
                        InstructionMove{
                            toolbarSelectedInstructionIndex.value(),
                            toolbarSelectedInstructionIndex.value() - 1U
                        };
            }

            if (moveRightPressed && canMoveSelectedRight) {
                queuedInstructionMove_ =
                        InstructionMove{
                            toolbarSelectedInstructionIndex.value(),
                            toolbarSelectedInstructionIndex.value() + 1U
                        };
            }

            const bool clipboardShortcutAllowed =
                    !io.WantTextInput &&
                    io.KeyCtrl &&
                    !pendingGate_.has_value();

            const bool copyShortcutPressed =
                    clipboardShortcutAllowed &&
                    ImGui::IsKeyPressed(ImGuiKey_C);

            const bool pasteShortcutPressed =
                    clipboardShortcutAllowed &&
                    ImGui::IsKeyPressed(ImGuiKey_V);

            const bool duplicateShortcutPressed =
                    clipboardShortcutAllowed &&
                    ImGui::IsKeyPressed(ImGuiKey_D);

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled(
                "Selection %zu",
                toolbarSelectedInstructionIndices.size()
            );

            ImGui::SameLine();

            const bool canCopy =
                    !toolbarSelectedInstructionIndices.empty() &&
                    !pendingGate_.has_value();

            if (!canCopy) {
                ImGui::BeginDisabled();
            }

            const bool copyButtonPressed =
                    ImGui::SmallButton("Copy");

            if (ImGui::IsItemHovered(
                ImGuiHoveredFlags_AllowWhenDisabled
            )) {
                ImGui::SetTooltip(
                    canCopy
                        ? "Copy selected gates [Ctrl+C]"
                        : "Select one or more gates to copy."
                );
            }

            if (!canCopy) {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();

            const bool canPaste =
                    !instructionClipboard_.empty() &&
                    !pendingGate_.has_value();

            if (!canPaste) {
                ImGui::BeginDisabled();
            }

            const bool pasteButtonPressed =
                    ImGui::SmallButton("Paste");

            if (ImGui::IsItemHovered(
                ImGuiHoveredFlags_AllowWhenDisabled
            )) {
                ImGui::SetTooltip(
                    canPaste
                        ? "Paste after the current selection [Ctrl+V]"
                        : "Copy gates before pasting."
                );
            }

            if (!canPaste) {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();

            if (!canCopy) {
                ImGui::BeginDisabled();
            }

            const bool duplicateButtonPressed =
                    ImGui::SmallButton("Duplicate");

            if (ImGui::IsItemHovered(
                ImGuiHoveredFlags_AllowWhenDisabled
            )) {
                ImGui::SetTooltip(
                    canCopy
                        ? "Duplicate selected gates [Ctrl+D]"
                        : "Select one or more gates to duplicate."
                );
            }

            if (!canCopy) {
                ImGui::EndDisabled();
            }

            const auto copySelectionToClipboard =
                    [&]() {
                const std::vector<circuit::CircuitInstructionSnapshot> allInstructions =
                        circuit_.instructionSnapshots();

                instructionClipboard_.clear();
                instructionClipboard_.reserve(
                    toolbarSelectedInstructionIndices.size()
                );

                for (
                    const std::size_t instructionIndex :
                        toolbarSelectedInstructionIndices
                ) {
                    if (instructionIndex < allInstructions.size()) {
                        instructionClipboard_.push_back(
                            allInstructions[instructionIndex]
                        );
                    }
                }
            };

            if (
                canCopy &&
                (
                    copyButtonPressed ||
                    copyShortcutPressed
                )
            ) {
                copySelectionToClipboard();
            }

            if (
                canCopy &&
                (
                    duplicateButtonPressed ||
                    duplicateShortcutPressed
                )
            ) {
                copySelectionToClipboard();
                queuedClipboardInsertionIndex_ =
                        toolbarSelectedInstructionIndices.back() + 1U;
            } else if (
                canPaste &&
                (
                    pasteButtonPressed ||
                    pasteShortcutPressed
                )
            ) {
                queuedClipboardInsertionIndex_ =
                        toolbarSelectedInstructionIndices.empty()
                            ? circuit_.instructionCount()
                            : toolbarSelectedInstructionIndices.back() + 1U;
            }

            ImGui::SameLine();

            if (!canCopy) {
                ImGui::BeginDisabled();
            }

            if (ImGui::SmallButton("Save block")) {
                reusableSubcircuitName_.fill('\0');
                ImGui::OpenPopup("Save reusable block");
            }

            if (ImGui::IsItemHovered(
                ImGuiHoveredFlags_AllowWhenDisabled
            )) {
                ImGui::SetTooltip(
                    canCopy
                        ? "Keep the selected gates in the reusable block library."
                        : "Select one or more gates to save a reusable block."
                );
            }

            if (!canCopy) {
                ImGui::EndDisabled();
            }

            if (
                ImGui::BeginPopupModal(
                    "Save reusable block",
                    nullptr,
                    ImGuiWindowFlags_AlwaysAutoResize
                )
            ) {
                ImGui::TextUnformatted(
                    "Name the selected circuit block"
                );

                ImGui::SetNextItemWidth(320.0F);
                ImGui::InputText(
                    "##ReusableBlockName",
                    reusableSubcircuitName_.data(),
                    reusableSubcircuitName_.size()
                );

                const bool hasName =
                        reusableSubcircuitName_.front() != '\0';

                if (!hasName) {
                    ImGui::BeginDisabled();
                }

                if (ImGui::Button("Save", ImVec2{110.0F, 0.0F})) {
                    const auto allInstructions =
                            circuit_.instructionSnapshots();

                    std::vector<circuit::CircuitInstructionSnapshot>
                            selectedInstructions;

                    selectedInstructions.reserve(
                        toolbarSelectedInstructionIndices.size()
                    );

                    for (
                        const std::size_t instructionIndex :
                        toolbarSelectedInstructionIndices
                    ) {
                        if (instructionIndex < allInstructions.size()) {
                            selectedInstructions.push_back(
                                allInstructions[instructionIndex]
                            );
                        }
                    }

                    try {
                        subcircuitLibrary_.save(
                            reusableSubcircuitName_.data(),
                            circuit_.qubitCount(),
                            selectedInstructions
                        );

                        reusableSubcircuits_ =
                                subcircuitLibrary_.loadAll();

                        selectedReusableSubcircuit_ = 0U;
                        projectStatusMessage_ =
                                "Reusable block saved.";
                        ImGui::CloseCurrentPopup();
                    } catch (const std::exception &error) {
                        projectStatusMessage_ =
                                std::string{
                                    "Block save failed: "
                                } +
                                error.what();
                    }
                }

                if (!hasName) {
                    ImGui::EndDisabled();
                }

                ImGui::SameLine();

                if (ImGui::Button("Cancel", ImVec2{110.0F, 0.0F})) {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            std::optional<circuit::CircuitInstructionInfo>
                    editableAngleInstruction;

            if (
                toolbarSelectedInstructionIndices.size() == 1U &&
                toolbarSelectedInstructionIndex.has_value()
            ) {
                const auto instructionInfo =
                        circuit_.instructionInfo();

                const std::size_t selectedIndex =
                        toolbarSelectedInstructionIndex.value();

                if (selectedIndex < instructionInfo.size()) {
                    const auto &candidate =
                            instructionInfo[selectedIndex];

                    if (
                        candidate.angleRadians.has_value() &&
                        usesGateParameters(candidate.name) &&
                        candidate.name != "U" &&
                        candidate.name != "fSim"
                    ) {
                        editableAngleInstruction = candidate;
                    }
                }
            }

            if (
                editableAngleInstruction.has_value() &&
                toolbarSelectedInstructionIndex.has_value()
            ) {
                const std::size_t selectedIndex =
                        toolbarSelectedInstructionIndex.value();

                if (
                    inlineAngleInstructionIndex_ !=
                    selectedIndex
                ) {
                    inlineAngleInstructionIndex_ =
                            selectedIndex;

                    inlineAnglePiCoefficient_ =
                            static_cast<float>(
                                editableAngleInstruction->
                                    angleRadians.value() /
                                std::numbers::pi
                            );
                }

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("\xCE\xB8");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(230.0F);

                ImGui::SliderFloat(
                    "##InlineGateAngle",
                    &inlineAnglePiCoefficient_,
                    -2.0F,
                    2.0F,
                    "%.3f \xCF\x80",
                    ImGuiSliderFlags_AlwaysClamp
                );

                ImGui::SameLine();

                if (ImGui::SmallButton("Apply angle")) {
                    queuedInstructionAngleEdit_ =
                            InstructionAngleEdit{
                                selectedIndex,
                                static_cast<double>(
                                    inlineAnglePiCoefficient_
                                ) *
                                std::numbers::pi
                            };
                }

                ImGui::SameLine();

                const std::string angleMeasurement =
                        notation::formatAngleMeasurement(
                            static_cast<double>(
                                inlineAnglePiCoefficient_
                            ) *
                            std::numbers::pi
                        );

                ImGui::TextDisabled(
                    "%s",
                    angleMeasurement.c_str()
                );
            } else {
                inlineAngleInstructionIndex_.reset();
            }

            if (showHistoryDebugInfo_) {
                ImGui::TextDisabled(
                    "Undo: %zu   Redo: %zu",
                    undoHistory_.size(),
                    redoHistory_.size()
                );
            }

            if (snapshot.stepCount > 0U) {
                float requestedPosition =
                        static_cast<float>(
                            snapshot.currentStepNumber
                        ) /
                        static_cast<float>(
                            snapshot.stepCount
                        );

                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled(
                    "STEP %zu / %zu",
                    snapshot.currentStepNumber,
                    snapshot.stepCount
                );
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-1.0F);

                ImGui::PushStyleColor(
                    ImGuiCol_FrameBg,
                    ImVec4{0.035F, 0.070F, 0.105F, 1.0F}
                );
                ImGui::PushStyleColor(
                    ImGuiCol_FrameBgHovered,
                    ImVec4{0.055F, 0.120F, 0.165F, 1.0F}
                );
                ImGui::PushStyleColor(
                    ImGuiCol_SliderGrab,
                    ImVec4{0.32F, 0.72F, 0.94F, 1.0F}
                );
                ImGui::PushStyleColor(
                    ImGuiCol_SliderGrabActive,
                    ImVec4{0.48F, 0.82F, 1.0F, 1.0F}
                );

                const bool stepChanged =
                        ImGui::SliderFloat(
                            "##CircuitStepScrub",
                            &requestedPosition,
                            0.0F,
                            1.0F,
                            "",
                            ImGuiSliderFlags_AlwaysClamp |
                            ImGuiSliderFlags_NoInput
                        );

                ImGui::PopStyleColor(4);

                if (
                    stepChanged
                ) {
                    const std::size_t requestedStep =
                            std::min(
                                static_cast<std::size_t>(
                                    std::llround(
                                        requestedPosition *
                                        static_cast<float>(
                                            snapshot.stepCount
                                        )
                                    )
                                ),
                                snapshot.stepCount
                            );

                    playbackPaused_ = true;
                    session_.moveToStepNumber(
                        requestedStep
                    );
                    snapshot = session_.snapshot();
                    synchronizeDensityLayer(snapshot);
                    circuitRenderer_.requestFocusStep(
                        requestedStep
                    );
                }
            }

            circuitRenderer_.draw(circuit_, snapshot, pendingGate_);

            const auto requestedInstructionMove =
                    circuitRenderer_.consumeInstructionMoveRequest();

            if (requestedInstructionMove.has_value()) {
                queuedInstructionMove_ =
                        requestedInstructionMove;
            }

            const auto requestedStepJump =
                    circuitRenderer_.consumeStepJumpRequest();

            if (
                requestedStepJump.has_value() &&
                requestedStepJump.value() <= session_.stepCount()
            ) {
                playbackPaused_ = true;
                session_.moveToStepNumber(
                    requestedStepJump.value()
                );

                snapshot = session_.snapshot();
                synchronizeDensityLayer(snapshot);
            }

            const auto singleQubitPlacement =
                    circuitRenderer_.consumeCompletedSingleQubitPlacement();

            if (singleQubitPlacement.has_value()) {
                queuedSingleQubitPlacement_ =
                        std::move(singleQubitPlacement);

                queuedSingleQubitParameters_ =
                        pendingGateParameters_;
            }

            const auto controlledPlacement =
                    circuitRenderer_.consumeCompletedControlledPlacement();

            if (controlledPlacement.has_value()) {
                queuedControlledPlacement_ =
                        controlledPlacement;

                queuedTwoQubitParameters_ =
                        pendingGateParameters_;
            }

            const auto threeQubitPlacement =
                    circuitRenderer_.consumeCompletedThreeQubitPlacement();

            if (threeQubitPlacement.has_value()) {
                queuedThreeQubitPlacement_ =
                        threeQubitPlacement;
            }

            const auto selectedInstructionIndex =
                    circuitRenderer_.selectedInstructionIndex();

            if (
                selectedInstructionIndex !=
                lastInspectorSelection_
            ) {
                lastInspectorSelection_ =
                        selectedInstructionIndex;

                if (selectedInstructionIndex.has_value()) {
                    const std::vector<circuit::CircuitInstructionInfo> instructions =
                            circuit_.instructionInfo();

                    if (
                        selectedInstructionIndex.value() <
                        instructions.size()
                    ) {
                        const circuit::CircuitInstructionInfo &instruction =
                                instructions[
                                    selectedInstructionIndex.value()
                                ];

                        const std::optional<std::size_t> focusedQubit =
                                instruction.targetQubit.has_value()
                                    ? instruction.targetQubit
                                    : (
                                        instruction.tertiaryTargetQubit.has_value()
                                            ? instruction.tertiaryTargetQubit
                                            : instruction.secondaryTargetQubit.has_value()
                                            ? instruction.secondaryTargetQubit
                                            : instruction.controlQubit
                                    );

                        if (focusedQubit.has_value()) {
                            inspectorPanel_.focusQubit(
                                focusedQubit.value()
                            );
                        }
                    }
                }
            }

            ImGui::End();

            if (!circuitFocusMode_) {
                drawDensityVolumeViewport(
                    ImVec2{
                        workPosition.x + leftPanelWidth + gap * 2.0F,
                        workPosition.y + topBarHeight + gap * 2.0F + circuitPanelHeight
                    },
                    ImVec2{
                        circuitPanelWidth,
                        densityVolumePanelHeight
                    }
                );
            }

            if (!circuitFocusMode_) {
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
                inspectorPanel_.draw(
                    session_,
                    snapshot,
                    selectedInstructionIndex,
                    densityVolumeStack_,
                    selectedDensityLayer_,
                    jetBrainsMonoHeadingFont_
                );

                ImGui::End();
            }

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
                armGatePlacement(
                    selectedGate.value()
                );
            }

            drawReusableSubcircuits();

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

            ++renderedFrameCount_;

            if (
                launchOptions_.capturePath.has_value() &&
                renderedFrameCount_ >=
                    launchOptions_.captureAfterFrames
            ) {
                glPixelStorei(
                    GL_PACK_ALIGNMENT,
                    1
                );

                std::vector<unsigned char> pixels(
                    static_cast<std::size_t>(framebufferWidth) *
                    static_cast<std::size_t>(framebufferHeight) *
                    3U
                );

                glReadPixels(
                    0,
                    0,
                    framebufferWidth,
                    framebufferHeight,
                    GL_RGB,
                    GL_UNSIGNED_BYTE,
                    pixels.data()
                );

                if (
                    !writeFramebufferPpm(
                        launchOptions_.capturePath.value(),
                        framebufferWidth,
                        framebufferHeight,
                        pixels
                    )
                ) {
                    captureFailed = true;

                    glfwSetWindowShouldClose(
                        window,
                        GLFW_TRUE
                    );
                }

                glfwSetWindowShouldClose(
                    window,
                    GLFW_TRUE
                );
            }

            // Present the completed frame.
            glfwSwapBuffers(window);
        }

        if (projectWorkspaceSessionActive_) {
            nextAutosaveAt_ = 0.0;
            autosaveProjectIfDue();

            if (!circuitHasUnsavedEdits_) {
                projectWorkspace_.discardRecovery();
            }

            projectWorkspace_.endSession();
            projectWorkspaceSessionActive_ = false;
        }

        densityVolumeRenderer_.shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();

        if (captureFailed) {
            throw std::runtime_error{
                "Failed to write the requested framebuffer capture."
            };
        }
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

    void GuiApplication::handleGlobalShortcuts() {
        const ImGuiIO &io =
                ImGui::GetIO();

        if (io.WantTextInput) {
            return;
        }

        if (
            io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_O, false)
        ) {
            queuedProjectOpenPath_ =
                    NativeFileDialog::openProject();
            return;
        }

        if (
            io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_S, false)
        ) {
            saveProject();
            return;
        }

        if (
            ImGui::IsKeyPressed(ImGuiKey_Escape, false) &&
            (
                pendingGate_.has_value() ||
                circuitRenderer_.hasPendingControlQubit()
            )
        ) {
            cancelGatePlacement();
            return;
        }

        if (
            ImGui::IsKeyPressed(ImGuiKey_Space, false) &&
            !simulationHistoryWorker_.busy()
        ) {
            playbackPaused_ =
                    !playbackPaused_;

            nextAutoStepAt_ =
                    ImGui::GetTime() + 0.45;

            return;
        }

        if (
            io.KeyCtrl ||
            io.KeyAlt ||
            io.KeyShift
        ) {
            return;
        }

        constexpr struct {
            ImGuiKey key;
            const char *gateName;
        } gateShortcuts[]{
            {ImGuiKey_H, "H"},
            {ImGuiKey_X, "X"},
            {ImGuiKey_Y, "Y"},
            {ImGuiKey_Z, "Z"},
            {ImGuiKey_S, "S"},
            {ImGuiKey_T, "T"}
        };

        for (const auto &shortcut : gateShortcuts) {
            if (
                ImGui::IsKeyPressed(
                    shortcut.key,
                    false
                )
            ) {
                armGatePlacement(
                    shortcut.gateName
                );
                return;
            }
        }
    }

    void GuiApplication::armGatePlacement(
        std::string gateName
    ) {
        const bool changedGate =
                !pendingGate_.has_value() ||
                pendingGate_.value() != gateName;

        if (changedGate) {
            circuitRenderer_.cancelPlacement();
        }

        pendingGate_ =
                gateName;

        pendingGateParameters_ =
                usesGateParameters(gateName)
                    ? std::optional<GateParameters>{
                        gateLibraryPanel_.gateParameters()
                    }
                    : std::nullopt;

        gateLibraryPanel_.selectGate(
            std::move(gateName)
        );
    }

    void GuiApplication::cancelGatePlacement() noexcept {
        pendingGate_.reset();
        pendingGateParameters_.reset();
        gateLibraryPanel_.clearSelection();
        circuitRenderer_.cancelPlacement();
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

    void GuiApplication::drawDensityVolumeViewport(
        const ImVec2 &position,
        const ImVec2 &size
    ) {
        ImGui::SetNextWindowPos(position, ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);

        ImGui::Begin(
            "Density Volume 3D",
            nullptr,
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse
        );

        const std::size_t layerCount =
                densityVolumeStack_.layers.size();

        if (layerCount > 0U) {
            selectedDensityLayer_ =
                    std::min(
                        selectedDensityLayer_,
                        layerCount - 1U
                    );

            comparisonDensityLayer_ =
                    std::min(
                        comparisonDensityLayer_,
                        layerCount - 1U
                    );
        }

        bool viewOptionsChanged = false;

        if (canvasMode_ != CanvasMode::LayerStack) {
            ImGui::BeginDisabled();
        }

        viewOptionsChanged =
                ImGui::Checkbox(
                    "Isolate layer",
                    &isolateDensityLayer_
                ) ||
                viewOptionsChanged;

        if (canvasMode_ != CanvasMode::LayerStack) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        const bool canCompare =
                layerCount > 1U;

        if (!canCompare) {
            ImGui::BeginDisabled();
        }

        const bool comparisonToggled =
                ImGui::Checkbox(
                    "Compare",
                    &compareDensityLayers_
                );

        if (!canCompare) {
            ImGui::EndDisabled();
            compareDensityLayers_ = false;
        }

        if (
            comparisonToggled &&
            compareDensityLayers_
        ) {
            comparisonDensityLayer_ =
                    selectedDensityLayer_ > 0U
                        ? selectedDensityLayer_ - 1U
                        : std::min<std::size_t>(
                            1U,
                            layerCount - 1U
                        );
        }

        viewOptionsChanged =
                comparisonToggled ||
                viewOptionsChanged;

        if (
            compareDensityLayers_ &&
            canCompare
        ) {
            ImGui::SameLine();
            ImGui::TextDisabled("reference");
            ImGui::SameLine();

            if (
                ImGui::SmallButton("<##PreviousComparisonLayer") &&
                comparisonDensityLayer_ > 0U
            ) {
                --comparisonDensityLayer_;
                viewOptionsChanged = true;
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(150.0F);

            int comparisonLayerValue =
                    static_cast<int>(
                        comparisonDensityLayer_
                    );

            if (
                ImGui::SliderInt(
                    "##ComparisonLayer",
                    &comparisonLayerValue,
                    0,
                    static_cast<int>(layerCount - 1U),
                    "layer %d"
                )
            ) {
                comparisonDensityLayer_ =
                        static_cast<std::size_t>(
                            comparisonLayerValue
                        );

                viewOptionsChanged = true;
            }

            ImGui::SameLine();

            if (
                ImGui::SmallButton(">##NextComparisonLayer") &&
                comparisonDensityLayer_ + 1U < layerCount
            ) {
                ++comparisonDensityLayer_;
                viewOptionsChanged = true;
            }
        }

        if (viewOptionsChanged) {
            densityVolumeCameraFramePending_ = true;
        }

        const ImVec2 available =
                ImGui::GetContentRegionAvail();

        const float footerHeight =
                ImGui::GetTextLineHeightWithSpacing() * 2.0F;

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

        const density_volume::VisualizationMode visualizationMode =
                canvasMode_ == CanvasMode::FloorField
                    ? density_volume::VisualizationMode::FloorField
                    : density_volume::VisualizationMode::LayerStack;

        const density_volume::SceneViewOptions sceneViewOptions{
            .isolateSelectedLayer =
                isolateDensityLayer_ &&
                canvasMode_ == CanvasMode::LayerStack,
            .comparisonLayer =
                compareDensityLayers_ && canCompare
                    ? std::optional<std::size_t>{
                        comparisonDensityLayer_
                    }
                    : std::nullopt
        };

        densityVolumeRenderer_.updateScene(
            densityVolumeStack_,
            selectedDensityLayer_,
            visualizationMode,
            sceneViewOptions
        );

        const float viewportAspect =
                static_cast<float>(framebufferWidth) /
                static_cast<float>(framebufferHeight);

        if (
            densityVolumeCameraFramePending_ ||
            !densityVolumeCamera_.isFramed()
        ) {
            densityVolumeCamera_.frameScene(
                densityVolumeRenderer_.framingMinimum(),
                densityVolumeRenderer_.framingMaximum(),
                viewportAspect,
                visualizationMode ==
                    density_volume::VisualizationMode::FloorField
            );
            densityVolumeCameraFramePending_ = false;
        } else {
            // The complete planned history keeps untouched playback stable.
            // Reapplying its bounds also adapts the reset composition when the
            // viewport aspect changes. Manual input retains the live pose.
            densityVolumeCamera_.updateSceneBounds(
                densityVolumeRenderer_.framingMinimum(),
                densityVolumeRenderer_.framingMaximum(),
                viewportAspect,
                visualizationMode ==
                    density_volume::VisualizationMode::FloorField
            );
        }

        densityVolumeCamera_.update(ImGui::GetIO().DeltaTime);

        densityVolumeRenderer_.render(
            framebufferWidth,
            framebufferHeight,
            selectedDensityLayer_,
            densityVolumeHeatAmount_,
            densityVolumeCamera_
        );

        const ImTextureRef densityVolumeTexture{
            static_cast<ImTextureID>(densityVolumeRenderer_.colorTexture())
        };

        const ImVec2 imageOrigin =
                ImGui::GetCursorScreenPos();

        // OpenGL framebuffer textures have a bottom-left origin, so the image
        // UVs are vertically flipped for Dear ImGui's top-left coordinate space.
        ImGui::Image(
            densityVolumeTexture,
            imageSize,
            ImVec2{0.0F, 1.0F},
            ImVec2{1.0F, 0.0F}
        );

        // The invisible input surface keeps all camera gestures scoped to Density Volume.
        ImGui::SetCursorScreenPos(imageOrigin);
        ImGui::InvisibleButton(
            "##DensityVolumeViewportInput",
            imageSize,
            ImGuiButtonFlags_MouseButtonLeft |
            ImGuiButtonFlags_MouseButtonRight
        );

        ImGuiIO &io =
                ImGui::GetIO();

        const bool viewportHovered =
                ImGui::IsItemHovered();

        const bool viewportActive =
                ImGui::IsItemActive();

        if (
            viewportHovered &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left)
        ) {
            densityVolumePointerDragged_ = false;
        }

        if (
            viewportActive &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0F)
        ) {
            densityVolumePointerDragged_ = true;

            if (io.KeyShift) {
                densityVolumeCamera_.pan(io.MouseDelta.x, io.MouseDelta.y);
            } else {
                densityVolumeCamera_.orbit(io.MouseDelta.x, io.MouseDelta.y);
            }
        }

        if (
            viewportActive &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Right, 3.0F)
        ) {
            densityVolumePointerDragged_ = true;
            densityVolumeCamera_.pan(io.MouseDelta.x, io.MouseDelta.y);
        }

        if (viewportHovered && std::abs(io.MouseWheel) > 0.0F) {
            densityVolumeCamera_.zoom(io.MouseWheel);
        }

        if (
            viewportHovered &&
            !io.WantTextInput &&
            ImGui::IsKeyPressed(ImGuiKey_R)
        ) {
            densityVolumeCamera_.reset();
        }

        std::optional<density_volume::Selection> hoveredCell;

        if (viewportHovered) {
            const ImVec2 pointer =
                    ImGui::GetMousePos();

            const float localX =
                    std::clamp(
                        pointer.x - imageOrigin.x,
                        0.0F,
                        imageSize.x - 1.0F
                    );

            const float localY =
                    std::clamp(
                        pointer.y - imageOrigin.y,
                        0.0F,
                        imageSize.y - 1.0F
                    );

            const int pickX =
                    std::clamp(
                        static_cast<int>(
                            localX / imageSize.x *
                            static_cast<float>(framebufferWidth)
                        ),
                        0,
                        framebufferWidth - 1
                    );

            const int pickY =
                    std::clamp(
                        static_cast<int>(
                            (1.0F - localY / imageSize.y) *
                            static_cast<float>(framebufferHeight)
                        ),
                        0,
                        framebufferHeight - 1
                    );

            hoveredCell =
                    densityVolumeRenderer_.pick(pickX, pickY);
        }

        if (
            hoveredCell.has_value() &&
            hoveredCell->layer < densityVolumeStack_.layers.size()
        ) {
            const density_volume::DensityLayer &layer =
                    densityVolumeStack_.layers[hoveredCell->layer];

            const density_volume::DensityCell &cell =
                    layer.cellAt(
                        hoveredCell->row,
                        hoveredCell->column
                    );

            std::optional<density_volume::DensityCell> differenceCell;

            if (
                compareDensityLayers_ &&
                comparisonDensityLayer_ <
                    densityVolumeStack_.layers.size()
            ) {
                const density_volume::DensityLayer differenceLayer =
                        density_volume::DensityModel::difference(
                            layer,
                            densityVolumeStack_.layers[
                                comparisonDensityLayer_
                            ]
                        );

                differenceCell =
                        differenceLayer.cellAt(
                            hoveredCell->row,
                            hoveredCell->column
                        );
            }

            ImGui::BeginTooltip();
            ImGui::Text(
                "LAYER %zu / %zu",
                hoveredCell->layer,
                densityVolumeStack_.layers.size() - 1U
            );
            ImGui::Separator();
            ImGui::Text(
                "row %zu  %s",
                cell.row,
                layer.bins[cell.row].label.c_str()
            );
            ImGui::Text(
                "col %zu  %s",
                cell.column,
                layer.bins[cell.column].label.c_str()
            );
            const std::string magnitudeText =
                    notation::formatReal(cell.magnitude);

            const std::string intensityText =
                    notation::formatReal(cell.intensity);

            const std::string phaseText =
                    notation::formatRadians(cell.phaseRadians);

            const std::string realText =
                    notation::formatReal(cell.real);

            const std::string imaginaryText =
                    notation::formatReal(cell.imaginary);

            ImGui::Text(
                "|\xCF\x81|       %s",
                magnitudeText.c_str()
            );

            ImGui::Text(
                "intensity   %s",
                intensityText.c_str()
            );

            ImGui::Text(
                "phase       %s",
                phaseText.c_str()
            );

            ImGui::Text(
                "Re(\xCF\x81)     %s",
                realText.c_str()
            );

            ImGui::Text(
                "Im(\xCF\x81)     %s",
                imaginaryText.c_str()
            );

            if (differenceCell.has_value()) {
                ImGui::Separator();
                ImGui::Text(
                    "\xCE\x94 from layer %zu",
                    comparisonDensityLayer_
                );

                ImGui::Text(
                    "|\xCE\x94\xCF\x81|      %s",
                    notation::formatReal(
                        differenceCell->magnitude
                    ).c_str()
                );

                ImGui::Text(
                    "Re(\xCE\x94\xCF\x81)    %s",
                    notation::formatReal(
                        differenceCell->real
                    ).c_str()
                );

                ImGui::Text(
                    "Im(\xCE\x94\xCF\x81)    %s",
                    notation::formatReal(
                        differenceCell->imaginary
                    ).c_str()
                );
            }
            ImGui::EndTooltip();

            if (
                viewportHovered &&
                ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                !densityVolumePointerDragged_
            ) {
                selectDensityLayer(hoveredCell->layer);
            }
        }

        const std::size_t dimension =
                layerCount == 0U
                    ? 0U
                    : densityVolumeStack_.layers[selectedDensityLayer_].dimension;

        ImGui::TextDisabled(
            "%s | LAYER %zu/%zu | \xCF\x81 %zux%zu | fixed rounded voxels",
            compareDensityLayers_ && canCompare
                ? "\xCE\x94 DENSITY"
                : isolateDensityLayer_ &&
                  canvasMode_ == CanvasMode::LayerStack
                    ? "ISOLATED LAYER"
                    : canvasMode_ == CanvasMode::FloorField
                        ? "FLOOR FIELD"
                        : "LAYER STACK",
            selectedDensityLayer_,
            layerCount == 0U ? 0U : layerCount - 1U,
            dimension,
            dimension
        );

        ImGui::TextDisabled(
            "LMB orbit | RMB / Shift+LMB pan | wheel zoom | R reset"
        );

        ImGui::End();
    }

    void GuiApplication::rebuildDensityVolume(
        const std::optional<std::size_t> firstChangedInstruction
    ) {
        if (firstChangedInstruction.has_value()) {
            density_volume::DensityModel::rebuildFrom(
                densityVolumeStack_,
                session_,
                firstChangedInstruction.value(),
                16U
            );
        } else {
            densityVolumeStack_ =
                    density_volume::DensityModel::build(
                        session_,
                        16U
                    );
        }

        if (densityVolumeStack_.layers.empty()) {
            selectedDensityLayer_ = 0U;
        } else {
            selectedDensityLayer_ =
                    std::min(
                        selectedDensityLayer_,
                        densityVolumeStack_.layers.size() - 1U
                    );
        }

        lastDensityDebuggerStepNumber_.reset();
        densityVolumeCameraFramePending_ = true;
    }

    void GuiApplication::synchronizeDensityLayer(
        const debug::DebuggerSnapshot &snapshot
    ) {
        if (densityVolumeStack_.layers.empty()) {
            selectedDensityLayer_ = 0U;
            return;
        }

        if (
            lastDensityDebuggerStepNumber_.has_value() &&
            lastDensityDebuggerStepNumber_.value() ==
                snapshot.currentStepNumber
        ) {
            return;
        }

        selectedDensityLayer_ =
                std::min(
                    snapshot.currentStepNumber,
                    densityVolumeStack_.layers.size() - 1U
                );

        lastDensityDebuggerStepNumber_ =
                snapshot.currentStepNumber;
    }

    void GuiApplication::selectDensityLayer(
        const std::size_t layerIndex
    ) {
        if (densityVolumeStack_.layers.empty()) {
            selectedDensityLayer_ = 0U;
            return;
        }

        selectedDensityLayer_ =
                std::min(
                    layerIndex,
                    densityVolumeStack_.layers.size() - 1U
                );

        if (selectedDensityLayer_ <= session_.stepCount()) {
            session_.moveToStepNumber(selectedDensityLayer_);
            lastDensityDebuggerStepNumber_ =
                    session_.currentStepNumber();
        }
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

        ImGui::TextUnformatted("\xCF\x81(t)");

        if (jetBrainsMonoHeadingFont_ != nullptr) {
            ImGui::PopFont();
        }

        ImGui::SameLine();
        ImGui::TextDisabled(
            "step %zu/%zu",
            snapshot.currentStepNumber,
            snapshot.stepCount
        );

        ImGui::SameLine();
        ImGui::TextColored(
            simulationHistoryWorker_.busy()
                ? ImVec4{0.34F, 0.74F, 1.0F, 1.0F}
                : simulationBuildError_.empty()
                    ? ImVec4{1.0F, 0.76F, 0.18F, 1.0F}
                    : ImVec4{1.0F, 0.34F, 0.34F, 1.0F},
            "%s",
            simulationHistoryWorker_.busy()
                ? "building"
                : simulationBuildError_.empty()
                    ? playbackPaused_ ? "settle" : "running"
                    : "build failed"
        );

        if (
            !simulationBuildError_.empty() &&
            ImGui::IsItemHovered()
        ) {
            ImGui::SetTooltip(
                "%s",
                simulationBuildError_.c_str()
            );
        }

        if (currentProjectPath_.has_value()) {
            ImGui::SameLine();
            ImGui::TextDisabled(
                "%s%s",
                currentProjectPath_->stem().string().c_str(),
                circuitHasUnsavedEdits_ ? "*" : ""
            );
        }

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 946.0F);

        if (ImGui::Button("Open", ImVec2{72.0F, 0.0F})) {
            queuedProjectOpenPath_ =
                    NativeFileDialog::openProject();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Open project [Ctrl+O]");
        }

        ImGui::SameLine();

        if (ImGui::Button("Save", ImVec2{72.0F, 0.0F})) {
            saveProject();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "%s",
                projectStatusMessage_.empty()
                    ? "Save project [Ctrl+S]"
                    : projectStatusMessage_.c_str()
            );
        }

        ImGui::SameLine();

        if (ImGui::Button("Recent", ImVec2{78.0F, 0.0F})) {
            ImGui::OpenPopup("RecentProjects");
        }

        if (ImGui::BeginPopup("RecentProjects")) {
            ImGui::TextDisabled("Recent projects");
            ImGui::Separator();

            if (recentProjectPaths_.empty()) {
                ImGui::TextDisabled("No recent projects");
            }

            for (
                const std::filesystem::path &path :
                recentProjectPaths_
            ) {
                if (
                    ImGui::MenuItem(
                        path.filename().string().c_str()
                    )
                ) {
                    queuedProjectOpenPath_ = path;
                    queuedProjectIsRecovery_ = false;
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(
                        "%s",
                        path.string().c_str()
                    );
                }
            }

            ImGui::EndPopup();
        }

        ImGui::SameLine();

        const auto modeButton =
                [&](const char *label, const CanvasMode mode) {
            const bool selected =
                    canvasMode_ == mode;

            if (selected) {
                ImGui::PushStyleColor(
                    ImGuiCol_Button,
                    ImVec4{0.08F, 0.20F, 0.29F, 1.0F}
                );
                ImGui::PushStyleColor(
                    ImGuiCol_ButtonHovered,
                    ImVec4{0.10F, 0.28F, 0.39F, 1.0F}
                );
                ImGui::PushStyleColor(
                    ImGuiCol_ButtonActive,
                    ImVec4{0.12F, 0.34F, 0.46F, 1.0F}
                );
            }

            const bool pressed =
                    ImGui::Button(label, ImVec2{102.0F, 0.0F});

            if (selected) {
                ImGui::PopStyleColor(3);
            }

            if (pressed) {
                if (canvasMode_ != mode) {
                    canvasMode_ = mode;
                    densityVolumeCameraFramePending_ = true;
                }
            }
        };

        modeButton("Layer Stack", CanvasMode::LayerStack);
        ImGui::SameLine();
        modeButton("Floor Field", CanvasMode::FloorField);

        ImGui::SameLine();

        if (simulationHistoryWorker_.busy()) {
            ImGui::BeginDisabled();
        }

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

        if (simulationHistoryWorker_.busy()) {
            ImGui::EndDisabled();
        }

        ImGui::End();
    }

    void GuiApplication::drawAlgorithmScripts() {
        ImGui::SeparatorText("Algorithm");

        ImGui::TextDisabled("Next circuit qubits");
        ImGui::SetNextItemWidth(-1.0F);
        ImGui::SliderInt(
            "##RegisterQubits",
            &presetQubitCount_,
            1,
            10
        );

        if (
            ImGui::Button(
                "New blank circuit",
                ImVec2{-1.0F, 0.0F}
            )
        ) {
            queuedBlankCircuitQubitCount_ =
                    static_cast<std::size_t>(
                        std::clamp(
                            presetQubitCount_,
                            1,
                            10
                        )
                    );
            playbackPaused_ = true;
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Start an empty circuit with this many qubits.\n"
                "The current circuit remains available through Undo."
            );
        }

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

            const int minimum =
                    minimumQubitCount(preset);

            const bool canLoad =
                    presetQubitCount_ >= minimum;

            if (!canLoad) {
                ImGui::BeginDisabled();
            }

            const bool drawRaisedRegisterPower =
                    preset == CircuitPreset::PlusRegister;

            const ImVec2 buttonMinimum =
                    ImGui::GetCursorScreenPos();

            const bool clicked =
                    ImGui::Button(
                        drawRaisedRegisterPower
                            ? "##plus-register"
                            : label,
                        ImVec2{buttonWidth, 42.0F}
                    );

            if (drawRaisedRegisterPower) {
                constexpr float superscriptScale =
                        0.72F;

                const ImVec2 buttonMaximum =
                        ImGui::GetItemRectMax();

                constexpr const char *plusKet =
                        "|+\xE2\x9F\xA9";

                const ImVec2 ketSize =
                        ImGui::CalcTextSize(plusKet);

                const ImVec2 exponentSize =
                        ImGui::CalcTextSize("n");

                const float groupWidth =
                        ketSize.x +
                        exponentSize.x * superscriptScale;

                const ImVec2 ketPosition{
                    buttonMinimum.x +
                        (buttonMaximum.x -
                         buttonMinimum.x -
                         groupWidth) *
                        0.5F,
                    buttonMinimum.y +
                        (buttonMaximum.y -
                         buttonMinimum.y -
                         ketSize.y) *
                        0.5F
                };

                const unsigned int textColor =
                        ImGui::GetColorU32(ImGuiCol_Text);

                ImDrawList *drawList =
                        ImGui::GetWindowDrawList();

                drawList->AddText(
                    ketPosition,
                    textColor,
                    plusKet
                );

                drawList->AddText(
                    ImGui::GetFont(),
                    ImGui::GetFontSize() * superscriptScale,
                    ImVec2{
                        ketPosition.x + ketSize.x,
                        ketPosition.y - 1.5F
                    },
                    textColor,
                    "n"
                );
            }

            if (!canLoad) {
                ImGui::EndDisabled();
            }

            if (clicked) {
                if (circuitHasUnsavedEdits_) {
                    presetAwaitingConfirmation_ =
                            preset;
                } else {
                    queuedPreset_ =
                            preset;
                }

                // Every replacement circuit opens on its untouched initial
                // state, even when the previous circuit was playing.
                playbackPaused_ = true;
            }

            if (
                ImGui::IsItemHovered(
                    ImGuiHoveredFlags_DelayShort |
                    ImGuiHoveredFlags_AllowWhenDisabled
                )
            ) {
                if (canLoad) {
                    ImGui::SetTooltip(
                        "%s\nRegister: %d qubits",
                        description,
                        presetQubitCount_
                    );
                } else {
                    ImGui::SetTooltip(
                        "%s\nRequires at least %d qubits; selected: %d.",
                        description,
                        minimum,
                        presetQubitCount_
                    );
                }
            }
        };

        struct AlgorithmEntry {
            const char *label;
            CircuitPreset preset;
            const char *description;
        };

        static constexpr std::array<AlgorithmEntry, 12U> firstPage{{
            {
                "Bell",
                CircuitPreset::Bell,
                "Entangles q0 and q1; extra qubits remain in |0\xE2\x9F\xA9."
            },
            {
                "Plus register",
                CircuitPreset::PlusRegister,
                "Creates a uniform superposition across the selected register."
            },
            {
                "GHZ",
                CircuitPreset::Ghz,
                "Entangles the complete selected register."
            },
            {
                "QFT",
                CircuitPreset::Qft,
                "Builds a Fourier phase history across the selected register."
            },
            {
                "iQFT",
                CircuitPreset::InverseQft,
                "Builds the exact inverse QFT across the selected register."
            },
            {
                "Grover",
                CircuitPreset::Grover,
                "Searches the all-one state across the complete selected register."
            },
            {
                "Deutsch-J",
                CircuitPreset::DeutschJozsa,
                "Uses n-1 inputs and one ancilla for a balanced parity oracle."
            },
            {
                "Kickback",
                CircuitPreset::Kickback,
                "Uses q0/q1 to expose phase kickback; extra qubits remain in |0\xE2\x9F\xA9."
            },
            {
                "Toffoli",
                CircuitPreset::Toffoli,
                "Runs a decomposed CCX on q0-q2; extra qubits remain in |0\xE2\x9F\xA9."
            },
            {
                "Bernstein",
                CircuitPreset::BernsteinVazirani,
                "Recovers an alternating hidden string across n-1 inputs."
            },
            {
                "Teleport",
                CircuitPreset::Teleportation,
                "Teleports from q0 to q2; extra qubits remain in |0\xE2\x9F\xA9."
            },
            {
                "Scramble",
                CircuitPreset::Scramble,
                "Applies a mixed-gate stress test to the selected register."
            }
        }};

        static constexpr std::array<AlgorithmEntry, 10U> secondPage{{
            {
                "Simon",
                CircuitPreset::Simon,
                "Finds hidden period 11 using a two-to-one parity oracle."
            },
            {
                "Shor",
                CircuitPreset::Shor,
                "Compiled order finding for a = 4 mod 15, whose period is 2."
            },
            {
                "QPE",
                CircuitPreset::Qpe,
                "Performs two-bit phase estimation for eigenphase 1/4."
            },
            {
                "VQE",
                CircuitPreset::Vqe,
                "Runs a fixed two-qubit variational ansatz without a classical optimizer."
            },
            {
                "QAOA",
                CircuitPreset::Qaoa,
                "Applies one Max-Cut cost and mixer layer across the selected register."
            },
            {
                "HHL",
                CircuitPreset::Hhl,
                "Demonstrates coherent solving for a fixed 2x2 linear system."
            },
            {
                "SWAP Test",
                CircuitPreset::SwapTest,
                "Estimates overlap between |+\xE2\x9F\xA9 and |1\xE2\x9F\xA9 using a controlled SWAP."
            },
            {
                "Quantum Walk",
                CircuitPreset::QuantumWalk,
                "Runs two coined walk steps on a four-position cycle."
            },
            {
                "BB84",
                CircuitPreset::Bb84,
                "Shows one matched and one mismatched preparation basis."
            },
            {
                "Superdense",
                CircuitPreset::Superdense,
                "Encodes and decodes classical message 11 through one Bell pair."
            }
        }};

        static constexpr std::array<AlgorithmEntry, 8U> thirdPage{{
            {
                "W State",
                CircuitPreset::WState,
                "Shares one excitation equally across the complete register."
            },
            {
                "Dicke k=2",
                CircuitPreset::DickeState,
                "Populates every basis state containing exactly two excitations."
            },
            {
                "Graph State",
                CircuitPreset::GraphState,
                "Creates a linear cluster state with CZ edges between neighbors."
            },
            {
                "Seeded Random",
                CircuitPreset::RandomCircuit,
                "Uses reproducible random rotations and entanglers for uneven probabilities."
            },
            {
                "Weighted State",
                CircuitPreset::WeightedState,
                "Prepares a deterministic non-uniform probability distribution."
            },
            {
                "Bit-flip Code",
                CircuitPreset::BitFlipCode,
                "Encodes one qubit, injects an X error, and coherently corrects it."
            },
            {
                "Steane [[7,1,3]]",
                CircuitPreset::SteaneCode,
                "Prepares the logical-zero state of the seven-qubit Steane code."
            },
            {
                "Shor [[9,1,3]]",
                CircuitPreset::ShorCode,
                "Encodes one qubit against bit and phase errors with nine qubits."
            }
        }};

        static constexpr std::array<AlgorithmEntry, 8U> fourthPage{{
            {
                "Phase-flip Code",
                CircuitPreset::PhaseFlipCode,
                "Encodes in the Hadamard basis, injects a Z error, and coherently corrects it."
            },
            {
                "Five-qubit Code",
                CircuitPreset::FiveQubitCode,
                "Prepares the exact logical-zero state of the perfect [[5,1,3]] code."
            },
            {
                "Quantum Count",
                CircuitPreset::QuantumCounting,
                "Counts one marked state in a two-qubit Grover search space."
            },
            {
                "Amplitude Est.",
                CircuitPreset::AmplitudeEstimation,
                "Estimates a fixed 30% prepared probability with three counting qubits."
            },
            {
                "Ripple Adder",
                CircuitPreset::RippleCarryAdder,
                "Reversibly demonstrates the two-bit addition 1 + 2 = 3."
            },
            {
                "Draper Adder",
                CircuitPreset::DraperAdder,
                "Performs the same addition through controlled Fourier phases."
            },
            {
                "IQP Sample",
                CircuitPreset::Iqp,
                "Runs commuting phase interactions for an uneven sampling distribution."
            },
            {
                "Surface Check",
                CircuitPreset::SurfaceCode,
                "Extracts one stabilizer syndrome from a correlated 3x3 data patch."
            }
        }};

        constexpr std::size_t pageCount =
                4U;

        algorithmPage_ =
                std::min(
                    algorithmPage_,
                    pageCount - 1U
                );

        const float rowSpacing =
                ImGui::GetStyle().ItemSpacing.y;

        const float catalogHeight =
                6.0F * 42.0F +
                5.0F * rowSpacing;

        ImGui::BeginChild(
            "AlgorithmCatalog",
            ImVec2{0.0F, catalogHeight},
            false,
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse
        );

        const auto drawPage =
                [&](const auto &page) {
            for (std::size_t index = 0U; index < page.size(); ++index) {
                const AlgorithmEntry &entry =
                        page[index];

                scriptButton(
                    entry.label,
                    entry.preset,
                    entry.description,
                    index % 2U == 1U
                );
            }
        };

        if (algorithmPage_ == 0U) {
            drawPage(firstPage);
        } else if (algorithmPage_ == 1U) {
            drawPage(secondPage);
        } else if (algorithmPage_ == 2U) {
            drawPage(thirdPage);
        } else {
            drawPage(fourthPage);
        }

        ImGui::EndChild();

        if (
            ImGui::BeginTable(
                "AlgorithmPageControls",
                3,
                ImGuiTableFlags_SizingStretchSame
            )
        ) {
            ImGui::TableNextColumn();

            const bool firstAlgorithmPage =
                    algorithmPage_ == 0U;

            if (firstAlgorithmPage) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("<##PreviousAlgorithmPage", ImVec2{-1.0F, 0.0F})) {
                --algorithmPage_;
            }

            if (firstAlgorithmPage) {
                ImGui::EndDisabled();
            }

            if (
                ImGui::IsItemHovered(
                    ImGuiHoveredFlags_DelayShort |
                    ImGuiHoveredFlags_AllowWhenDisabled
                )
            ) {
                ImGui::SetTooltip("Previous algorithm page");
            }

            ImGui::TableNextColumn();

            const std::string pageLabel =
                    std::to_string(algorithmPage_ + 1U) +
                    " / " +
                    std::to_string(pageCount);

            const float pageLabelOffset =
                    std::max(
                        0.0F,
                        (
                            ImGui::GetContentRegionAvail().x -
                            ImGui::CalcTextSize(pageLabel.c_str()).x
                        ) * 0.5F
                    );

            ImGui::SetCursorPosX(
                ImGui::GetCursorPosX() +
                pageLabelOffset
            );
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(pageLabel.c_str());

            ImGui::TableNextColumn();

            const bool lastAlgorithmPage =
                    algorithmPage_ + 1U >= pageCount;

            if (lastAlgorithmPage) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button(">##NextAlgorithmPage", ImVec2{-1.0F, 0.0F})) {
                ++algorithmPage_;
            }

            if (lastAlgorithmPage) {
                ImGui::EndDisabled();
            }

            if (
                ImGui::IsItemHovered(
                    ImGuiHoveredFlags_DelayShort |
                    ImGuiHoveredFlags_AllowWhenDisabled
                )
            ) {
                ImGui::SetTooltip("Next algorithm page");
            }

            ImGui::EndTable();
        }

        if (
            presetAwaitingConfirmation_.has_value() &&
            !ImGui::IsPopupOpen("Replace custom circuit?")
        ) {
            ImGui::OpenPopup("Replace custom circuit?");
        }

        if (
            ImGui::BeginPopupModal(
                "Replace custom circuit?",
                nullptr,
                ImGuiWindowFlags_AlwaysAutoResize
            )
        ) {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                presetAwaitingConfirmation_.reset();
                ImGui::CloseCurrentPopup();
            }

            ImGui::TextUnformatted(
                "Loading this algorithm replaces the current custom circuit."
            );
            ImGui::TextDisabled(
                "You can restore the complete circuit and register with Undo."
            );
            ImGui::Spacing();

            if (
                ImGui::Button(
                    "Replace circuit",
                    ImVec2{150.0F, 0.0F}
                )
            ) {
                queuedPreset_ =
                        presetAwaitingConfirmation_;
                presetAwaitingConfirmation_.reset();
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (
                ImGui::Button(
                    "Cancel",
                    ImVec2{100.0F, 0.0F}
                )
            ) {
                presetAwaitingConfirmation_.reset();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void GuiApplication::applyPlayback(
        debug::DebuggerSession &session,
        const debug::DebuggerSnapshot &snapshot
    ) {
        if (simulationHistoryWorker_.busy()) {
            return;
        }

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

    void GuiApplication::loadPreset(
        const CircuitPreset preset
    ) {
        recordEditorForUndo();

        circuit_ =
                createPresetCircuit(preset);

        // Presets always start in the all-zero basis state for predictable demos.
        initialState_ =
                quantum::QuantumRegister::basisState(
                    circuit_.qubitCount(),
                    0
        );

        resetEditorTransientState();
        currentProjectPath_.reset();
        projectStatusMessage_.clear();
        circuitHasUnsavedEdits_ = false;
        playbackPaused_ = true;

        rebuildDebuggerAfterCircuitEdit();
    }

    void GuiApplication::applyQueuedPreset() {
        if (!queuedPreset_.has_value()) {
            return;
        }

        const CircuitPreset preset =
                queuedPreset_.value();

        queuedPreset_.reset();
        loadPreset(preset);
    }

    void GuiApplication::createBlankCircuit(
        const std::size_t qubitCount
    ) {
        const std::size_t safeQubitCount =
                std::clamp(
                    qubitCount,
                    std::size_t{1},
                    std::size_t{10}
                );

        recordEditorForUndo();

        circuit_ =
                circuit::QuantumCircuit{
                    safeQubitCount
                };

        initialState_ =
                quantum::QuantumRegister::basisState(
                    safeQubitCount,
                    0
                );

        presetQubitCount_ =
                static_cast<int>(
                    safeQubitCount
                );

        resetEditorTransientState();
        currentProjectPath_.reset();
        projectStatusMessage_.clear();
        circuitHasUnsavedEdits_ = false;
        playbackPaused_ = true;
        rebuildDebuggerAfterCircuitEdit();
    }

    void GuiApplication::clearCircuitInstructions() {
        if (circuit_.instructionCount() == 0U) {
            return;
        }

        recordEditorForUndo();

        circuit_ =
                circuit::QuantumCircuit{
                    circuit_.qubitCount()
                };

        resetEditorTransientState();
        circuitHasUnsavedEdits_ = true;
        playbackPaused_ = true;
        rebuildDebuggerAfterCircuitEdit();
    }

    circuit::QuantumCircuit GuiApplication::createPresetCircuit(CircuitPreset preset) const {
        const std::size_t qubitCount =
                static_cast<std::size_t>(
                    std::clamp(presetQubitCount_, 1, 10)
                );

        if (
            qubitCount <
            static_cast<std::size_t>(
                minimumQubitCount(preset)
            )
        ) {
            throw std::invalid_argument{
                "The selected register is too small for this algorithm."
            };
        }

        if (preset == CircuitPreset::Bell) {
            return algorithms::bellStateCircuit(qubitCount);
        }

        if (preset == CircuitPreset::Ghz) {
            return algorithms::ghzStateCircuit(qubitCount);
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
            return algorithms::groverSearchCircuit(qubitCount);
        }

        if (preset == CircuitPreset::DeutschJozsa) {
            return algorithms::deutschJozsaCircuit(qubitCount);
        }

        if (preset == CircuitPreset::BernsteinVazirani) {
            const std::size_t inputQubitCount =
                    qubitCount - 1U;

            std::size_t hiddenValue{};

            // Repeat 1010... so every register size gets a deterministic oracle.
            for (std::size_t input = 0; input < inputQubitCount; ++input) {
                hiddenValue <<= 1U;

                if (input % 2U == 0U) {
                    hiddenValue |= 1U;
                }
            }

            return algorithms::bernsteinVaziraniCircuit(
                inputQubitCount,
                hiddenValue
            );
        }

        if (preset == CircuitPreset::Toffoli) {
            return algorithms::toffoliDemoCircuit(qubitCount);
        }

        if (preset == CircuitPreset::Kickback) {
            return algorithms::phaseKickbackCircuit(qubitCount);
        }

        if (preset == CircuitPreset::Teleportation) {
            return algorithms::teleportationCircuit(qubitCount);
        }

        if (preset == CircuitPreset::Scramble) {
            return algorithms::scrambleCircuit(qubitCount);
        }

        if (preset == CircuitPreset::Simon) {
            return algorithms::simonCircuit(qubitCount);
        }

        if (preset == CircuitPreset::Shor) {
            return algorithms::shorPeriodFindingCircuit(qubitCount);
        }

        if (preset == CircuitPreset::Qpe) {
            return algorithms::quantumPhaseEstimationCircuit(qubitCount);
        }

        if (preset == CircuitPreset::Vqe) {
            return algorithms::vqeAnsatzCircuit(qubitCount);
        }

        if (preset == CircuitPreset::Qaoa) {
            return algorithms::qaoaMaxCutCircuit(qubitCount);
        }

        if (preset == CircuitPreset::Hhl) {
            return algorithms::hhlDemoCircuit(qubitCount);
        }

        if (preset == CircuitPreset::SwapTest) {
            return algorithms::swapTestCircuit(qubitCount);
        }

        if (preset == CircuitPreset::QuantumWalk) {
            return algorithms::quantumWalkCircuit(qubitCount);
        }

        if (preset == CircuitPreset::Bb84) {
            return algorithms::bb84DemoCircuit(qubitCount);
        }

        if (preset == CircuitPreset::Superdense) {
            return algorithms::superdenseCodingCircuit(qubitCount);
        }

        if (preset == CircuitPreset::WState) {
            return algorithms::wStateCircuit(qubitCount);
        }

        if (preset == CircuitPreset::DickeState) {
            return algorithms::dickeStateCircuit(
                qubitCount,
                2U
            );
        }

        if (preset == CircuitPreset::GraphState) {
            return algorithms::graphStateCircuit(qubitCount);
        }

        if (preset == CircuitPreset::RandomCircuit) {
            return algorithms::randomCircuit(
                qubitCount,
                0x514255424954ULL +
                static_cast<std::uint64_t>(
                    qubitCount
                )
            );
        }

        if (preset == CircuitPreset::WeightedState) {
            return algorithms::weightedStatePreparationCircuit(
                qubitCount
            );
        }

        if (preset == CircuitPreset::BitFlipCode) {
            return algorithms::bitFlipCodeCircuit(qubitCount);
        }

        if (preset == CircuitPreset::SteaneCode) {
            return algorithms::steaneCodeCircuit(qubitCount);
        }

        if (preset == CircuitPreset::ShorCode) {
            return algorithms::shorCodeCircuit(qubitCount);
        }

        if (preset == CircuitPreset::PhaseFlipCode) {
            return algorithms::phaseFlipCodeCircuit(qubitCount);
        }

        if (preset == CircuitPreset::FiveQubitCode) {
            return algorithms::fiveQubitCodeCircuit(qubitCount);
        }

        if (preset == CircuitPreset::QuantumCounting) {
            return algorithms::quantumCountingCircuit(qubitCount);
        }

        if (preset == CircuitPreset::AmplitudeEstimation) {
            return algorithms::amplitudeEstimationCircuit(qubitCount);
        }

        if (preset == CircuitPreset::RippleCarryAdder) {
            return algorithms::rippleCarryAdderCircuit(qubitCount);
        }

        if (preset == CircuitPreset::DraperAdder) {
            return algorithms::draperAdderCircuit(qubitCount);
        }

        if (preset == CircuitPreset::Iqp) {
            return algorithms::iqpCircuit(qubitCount);
        }

        if (preset == CircuitPreset::SurfaceCode) {
            return algorithms::surfaceCodeStabilizerCircuit(qubitCount);
        }

        throw std::invalid_argument{
            "Unsupported circuit preset."
        };
    }

    int GuiApplication::minimumQubitCount(
        const CircuitPreset preset
    ) noexcept {
        if (
            preset == CircuitPreset::Bell ||
            preset == CircuitPreset::Grover ||
            preset == CircuitPreset::DeutschJozsa ||
            preset == CircuitPreset::BernsteinVazirani ||
            preset == CircuitPreset::Kickback ||
            preset == CircuitPreset::Vqe ||
            preset == CircuitPreset::Qaoa ||
            preset == CircuitPreset::Bb84 ||
            preset == CircuitPreset::Superdense ||
            preset == CircuitPreset::WState ||
            preset == CircuitPreset::DickeState ||
            preset == CircuitPreset::GraphState ||
            preset == CircuitPreset::RandomCircuit ||
            preset == CircuitPreset::Iqp
        ) {
            return 2;
        }

        if (
            preset == CircuitPreset::Toffoli ||
            preset == CircuitPreset::Teleportation ||
            preset == CircuitPreset::Qpe ||
            preset == CircuitPreset::SwapTest ||
            preset == CircuitPreset::QuantumWalk ||
            preset == CircuitPreset::BitFlipCode ||
            preset == CircuitPreset::PhaseFlipCode
        ) {
            return 3;
        }

        if (
            preset == CircuitPreset::Simon ||
            preset == CircuitPreset::Shor ||
            preset == CircuitPreset::Hhl ||
            preset == CircuitPreset::AmplitudeEstimation ||
            preset == CircuitPreset::RippleCarryAdder ||
            preset == CircuitPreset::DraperAdder
        ) {
            return 4;
        }

        if (
            preset == CircuitPreset::FiveQubitCode ||
            preset == CircuitPreset::QuantumCounting
        ) {
            return 5;
        }

        if (preset == CircuitPreset::SteaneCode) {
            return 7;
        }

        if (preset == CircuitPreset::ShorCode) {
            return 9;
        }

        if (preset == CircuitPreset::SurfaceCode) {
            return 10;
        }

        return 1;
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

    void GuiApplication::drawReusableSubcircuits() {
        ImGui::Spacing();
        ImGui::SeparatorText("Reusable blocks");

        if (reusableSubcircuits_.empty()) {
            ImGui::TextDisabled(
                "Select circuit gates, then choose Save block."
            );
            return;
        }

        selectedReusableSubcircuit_ =
                std::min(
                    selectedReusableSubcircuit_,
                    reusableSubcircuits_.size() - 1U
                );

        const project::StoredSubcircuit &selected =
                reusableSubcircuits_[
                    selectedReusableSubcircuit_
                ];

        ImGui::SetNextItemWidth(-1.0F);

        if (
            ImGui::BeginCombo(
                "##ReusableBlock",
                selected.name.c_str()
            )
        ) {
            for (
                std::size_t blockIndex = 0U;
                blockIndex < reusableSubcircuits_.size();
                ++blockIndex
            ) {
                const bool isSelected =
                        blockIndex ==
                        selectedReusableSubcircuit_;

                if (
                    ImGui::Selectable(
                        reusableSubcircuits_[
                            blockIndex
                        ].name.c_str(),
                        isSelected
                    )
                ) {
                    selectedReusableSubcircuit_ =
                            blockIndex;
                }
            }

            ImGui::EndCombo();
        }

        const project::StoredSubcircuit &active =
                reusableSubcircuits_[
                    selectedReusableSubcircuit_
                ];

        const bool canInsert =
                active.canInsertInto(
                    circuit_.qubitCount()
                ) &&
                !pendingGate_.has_value();

        if (!canInsert) {
            ImGui::BeginDisabled();
        }

        if (
            ImGui::Button(
                "Insert block",
                ImVec2{-1.0F, 0.0F}
            )
        ) {
            instructionClipboard_ =
                    active.instructions;

            const std::vector<std::size_t> &selection =
                    circuitRenderer_.
                        selectedInstructionIndices();

            queuedClipboardInsertionIndex_ =
                    selection.empty()
                        ? circuit_.instructionCount()
                        : selection.back() + 1U;
        }

        if (
            ImGui::IsItemHovered(
                ImGuiHoveredFlags_AllowWhenDisabled
            )
        ) {
            ImGui::SetTooltip(
                canInsert
                    ? "%zu gates authored on %zu qubits"
                    : "This block contains an operand or register-wide operation that does not fit the current register.",
                active.instructions.size(),
                active.sourceQubitCount
            );
        }

        if (!canInsert) {
            ImGui::EndDisabled();
        }
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
        const GateParameters &parameters
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

        if (gateName == "Sdg") {
            return gates::sDaggerGate();
        }

        if (gateName == "T") {
            return gates::tGate();
        }

        if (gateName == "Tdg") {
            return gates::tDaggerGate();
        }

        if (gateName == "SX") {
            return gates::sxGate();
        }

        if (gateName == "SXdg") {
            return gates::sxDaggerGate();
        }

        if (gateName == "P") {
            return gates::phaseGate(
                parameters.thetaRadians
            );
        }

        if (gateName == "U") {
            return gates::uGate(
                parameters.thetaRadians,
                parameters.phiRadians,
                parameters.lambdaRadians
            );
        }

        if (gateName == "Rx") {
            return gates::rxGate(parameters.thetaRadians);
        }

        if (gateName == "Ry") {
            return gates::ryGate(parameters.thetaRadians);
        }

        if (gateName == "Rz") {
            return gates::rzGate(parameters.thetaRadians);
        }

        throw std::invalid_argument("Unsupported single-qubit gate " + gateName);
    }

    void GuiApplication::applyQueuedCircuitEdits() {
        if (queuedInstructionAngleEdit_.has_value()) {
            const InstructionAngleEdit edit =
                    queuedInstructionAngleEdit_.value();

            queuedInstructionAngleEdit_.reset();

            if (edit.instructionIndex >= circuit_.instructionCount()) {
                return;
            }

            auto snapshots =
                    circuit_.instructionSnapshots();

            circuit::CircuitInstructionSnapshot replacement =
                    snapshots[edit.instructionIndex];

            GateParameters parameters{};
            parameters.thetaRadians =
                    edit.angleRadians;

            if (
                replacement.kind ==
                circuit::CircuitInstructionKind::SingleQubit
            ) {
                replacement.matrix =
                        createSingleQubitGateMatrix(
                            replacement.name,
                            parameters
                        );
            } else if (
                replacement.kind ==
                circuit::CircuitInstructionKind::TwoQubit
            ) {
                replacement.matrix =
                        createTwoQubitGateMatrix(
                            replacement.name,
                            parameters
                        );
            } else {
                return;
            }

            replacement.angleRadians =
                    edit.angleRadians;

            recordEditorForUndo();

            if (
                !circuit_.replaceInstructionSnapshot(
                    edit.instructionIndex,
                    replacement
                )
            ) {
                undoHistory_.pop_back();
                return;
            }

            circuitHasUnsavedEdits_ = true;
            playbackPaused_ = true;

            rebuildDebuggerAfterCircuitEdit(
                edit.instructionIndex,
                edit.instructionIndex + 1U
            );

            circuitRenderer_.selectInstruction(
                edit.instructionIndex
            );

            return;
        }

        if (queuedBlankCircuitQubitCount_.has_value()) {
            const std::size_t qubitCount =
                    queuedBlankCircuitQubitCount_.value();

            queuedBlankCircuitQubitCount_.reset();
            createBlankCircuit(qubitCount);
            return;
        }

        if (queuedClearCircuit_) {
            queuedClearCircuit_ = false;
            clearCircuitInstructions();
            return;
        }

        if (!queuedInstructionDeletions_.empty()) {
            std::vector<std::size_t> instructionIndices =
                    std::move(queuedInstructionDeletions_);

            queuedInstructionDeletions_.clear();

            std::sort(
                instructionIndices.begin(),
                instructionIndices.end()
            );

            instructionIndices.erase(
                std::unique(
                    instructionIndices.begin(),
                    instructionIndices.end()
                ),
                instructionIndices.end()
            );

            instructionIndices.erase(
                std::remove_if(
                    instructionIndices.begin(),
                    instructionIndices.end(),
                    [this](const std::size_t index) {
                        return index >=
                               circuit_.instructionCount();
                    }
                ),
                instructionIndices.end()
            );

            if (instructionIndices.empty()) {
                return;
            }

            const std::size_t firstChangedInstruction =
                    instructionIndices.front();

            recordEditorForUndo();

            for (
                auto instructionIterator =
                        instructionIndices.rbegin();
                instructionIterator != instructionIndices.rend();
                ++instructionIterator
            ) {
                static_cast<void>(
                    circuit_.removeInstruction(
                        *instructionIterator
                    )
                );
            }

            circuitHasUnsavedEdits_ = true;

            const std::size_t nearbyStep =
                    std::min(
                        firstChangedInstruction,
                        circuit_.instructionCount()
                    );

            rebuildDebuggerAfterCircuitEdit(
                firstChangedInstruction,
                nearbyStep
            );

            circuitRenderer_.clearSelection();
            circuitRenderer_.requestFocusStep(
                nearbyStep
            );

            return;
        }

        if (
            queuedClipboardInsertionIndex_.has_value() &&
            !instructionClipboard_.empty()
        ) {
            const std::size_t insertionIndex =
                    std::min(
                        queuedClipboardInsertionIndex_.value(),
                        circuit_.instructionCount()
                    );

            queuedClipboardInsertionIndex_.reset();
            recordEditorForUndo();

            std::vector<std::size_t> insertedIndices;
            insertedIndices.reserve(
                instructionClipboard_.size()
            );

            for (
                std::size_t clipboardIndex = 0U;
                clipboardIndex < instructionClipboard_.size();
                ++clipboardIndex
            ) {
                const std::size_t targetIndex =
                        insertionIndex +
                        clipboardIndex;

                circuit_.insertInstructionSnapshot(
                    targetIndex,
                    instructionClipboard_[clipboardIndex]
                );

                insertedIndices.push_back(targetIndex);
            }

            circuitHasUnsavedEdits_ = true;

            rebuildDebuggerAfterCircuitEdit(
                insertionIndex,
                insertionIndex +
                    insertedIndices.size()
            );

            circuitRenderer_.selectInstructions(
                std::move(insertedIndices)
            );

            return;
        }

        queuedClipboardInsertionIndex_.reset();

        if (queuedInstructionMove_.has_value()) {
            const InstructionMove move =
                    queuedInstructionMove_.value();

            queuedInstructionMove_.reset();

            if (
                move.fromIndex >= circuit_.instructionCount() ||
                move.toIndex >= circuit_.instructionCount() ||
                move.fromIndex == move.toIndex
            ) {
                return;
            }

            recordEditorForUndo();

            const bool moved =
                    circuit_.moveInstruction(
                        move.fromIndex,
                        move.toIndex
                    );

            if (!moved) {
                undoHistory_.pop_back();
                return;
            }

            circuitHasUnsavedEdits_ = true;

            rebuildDebuggerAfterCircuitEdit(
                std::min(
                    move.fromIndex,
                    move.toIndex
                ),
                move.toIndex + 1U
            );

            circuitRenderer_.selectInstruction(
                move.toIndex
            );

            return;
        }

        if (queuedSingleQubitPlacement_.has_value()) {
            const std::string &gateName =
                    queuedSingleQubitPlacement_->gateName;

            const std::size_t targetQubit =
                    queuedSingleQubitPlacement_->targetQubit;

            const GateParameters parameters =
                    queuedSingleQubitParameters_.value_or(
                        GateParameters{}
                    );

            const std::optional<double> angleRadians =
                    usesGateParameters(gateName)
                        ? std::optional<double>{
                            parameters.thetaRadians
                        }
                        : std::nullopt;

            recordEditorForUndo();

            // Placement stores the insertion slot picked by the circuit renderer.
            const std::size_t instructionIndex =
                    std::min(
                        queuedSingleQubitPlacement_->instructionIndex,
                        circuit_.instructionCount()
                    );

            circuit_.insertSingleQubitGate(
                instructionIndex,
                gateName,
                createSingleQubitGateMatrix(gateName, parameters),
                targetQubit,
                angleRadians
            );

            queuedSingleQubitPlacement_.reset();
            queuedSingleQubitParameters_.reset();
            circuitHasUnsavedEdits_ = true;
            inspectorPanel_.focusQubit(targetQubit);

            rebuildDebuggerAfterCircuitEdit(
                instructionIndex,
                instructionIndex + 1U
            );

            circuitRenderer_.clearSelection();
            circuitRenderer_.continuePlacementAfter(
                instructionIndex
            );
        } else if (queuedThreeQubitPlacement_.has_value()) {
            const std::string &gateName =
                    queuedThreeQubitPlacement_->gateName;

            const std::size_t firstQubit =
                    queuedThreeQubitPlacement_->firstQubit;

            const std::size_t secondQubit =
                    queuedThreeQubitPlacement_->secondQubit;

            const std::size_t thirdQubit =
                    queuedThreeQubitPlacement_->thirdQubit;

            const std::size_t instructionIndex =
                    std::min(
                        queuedThreeQubitPlacement_->instructionIndex,
                        circuit_.instructionCount()
                    );

            recordEditorForUndo();

            circuit_.insertThreeQubitGate(
                instructionIndex,
                gateName,
                createThreeQubitGateMatrix(gateName),
                firstQubit,
                secondQubit,
                thirdQubit
            );

            queuedThreeQubitPlacement_.reset();
            circuitHasUnsavedEdits_ = true;
            inspectorPanel_.focusQubit(thirdQubit);

            rebuildDebuggerAfterCircuitEdit(
                instructionIndex,
                instructionIndex + 1U
            );

            circuitRenderer_.clearSelection();
            circuitRenderer_.continuePlacementAfter(
                instructionIndex
            );
        } else if (queuedControlledPlacement_.has_value()) {
            const std::string &gateName =
                    queuedControlledPlacement_->gateName;
            const std::size_t controlQubit =
                    queuedControlledPlacement_->controlQubit;

            const std::size_t targetQubit =
                    queuedControlledPlacement_->targetQubit;

            const std::size_t instructionIndex =
                    std::min(
                        queuedControlledPlacement_->instructionIndex,
                        circuit_.instructionCount()
                    );

            const GateParameters parameters =
                    queuedTwoQubitParameters_.value_or(
                        GateParameters{}
                    );

            const std::optional<double> angleRadians =
                    usesGateParameters(gateName)
                        ? std::optional<double>{
                            parameters.thetaRadians
                        }
                        : std::nullopt;

            recordEditorForUndo();

            // Two-qubit placements retain only their local 4x4 matrix, keeping
            // large-register edits responsive and inexpensive to undo.
            circuit_.insertTwoQubitGate(
                instructionIndex,
                gateName,
                createTwoQubitGateMatrix(gateName, parameters),
                controlQubit,
                targetQubit,
                angleRadians
            );

            queuedControlledPlacement_.reset();
            queuedTwoQubitParameters_.reset();
            circuitHasUnsavedEdits_ = true;
            inspectorPanel_.focusQubit(targetQubit);

            rebuildDebuggerAfterCircuitEdit(
                instructionIndex,
                instructionIndex + 1U
            );

            circuitRenderer_.clearSelection();
            circuitRenderer_.continuePlacementAfter(
                instructionIndex
            );
        }
    }

    void GuiApplication::undoLastCircuitEdit() {
        if (undoHistory_.empty()) {
            return;
        }

        const std::size_t previousStep =
                session_.currentStepNumber();

        redoHistory_.push_back(
            EditorSnapshot{
                circuit_,
                initialState_,
                circuitHasUnsavedEdits_,
                currentProjectPath_
            }
        );

        // Move the latest undo snapshot back into the complete editor state.
        EditorSnapshot restored =
                std::move(
                    undoHistory_.back()
                );

        circuit_ =
                std::move(restored.circuit);

        initialState_ =
                std::move(restored.initialState);

        circuitHasUnsavedEdits_ =
                restored.hasUnsavedEdits;

        currentProjectPath_ =
                std::move(restored.projectPath);

        presetQubitCount_ =
                static_cast<int>(
                    circuit_.qubitCount()
                );

        undoHistory_.pop_back();

        rebuildDebuggerAfterCircuitEdit(
            std::nullopt,
            std::min(
                previousStep,
                circuit_.instructionCount()
            )
        );

        cancelGatePlacement();
        circuitRenderer_.clearSelection();
    }

    void GuiApplication::redoLastCircuitEdit() {
        if (redoHistory_.empty()) {
            return;
        }

        const std::size_t previousStep =
                session_.currentStepNumber();

        undoHistory_.push_back(
            EditorSnapshot{
                circuit_,
                initialState_,
                circuitHasUnsavedEdits_,
                currentProjectPath_
            }
        );

        // Symmetric with undo: restore the complete editor state.
        EditorSnapshot restored =
                std::move(
                    redoHistory_.back()
                );

        circuit_ =
                std::move(restored.circuit);

        initialState_ =
                std::move(restored.initialState);

        circuitHasUnsavedEdits_ =
                restored.hasUnsavedEdits;

        currentProjectPath_ =
                std::move(restored.projectPath);

        presetQubitCount_ =
                static_cast<int>(
                    circuit_.qubitCount()
                );

        redoHistory_.pop_back();

        rebuildDebuggerAfterCircuitEdit(
            std::nullopt,
            std::min(
                previousStep,
                circuit_.instructionCount()
            )
        );

        cancelGatePlacement();
        circuitRenderer_.clearSelection();
    }

    math::ComplexMatrix GuiApplication::createTwoQubitGateMatrix(
        const std::string &gateName,
        const GateParameters &parameters
    ) {
        if (gateName == "CX") {
            return gates::cxGate();
        }

        if (gateName == "CY") {
            return gates::cyGate();
        }

        if (gateName == "CZ") {
            return gates::czGate();
        }

        if (gateName == "CH") {
            return gates::chGate();
        }

        if (gateName == "CS") {
            return gates::csGate();
        }

        if (gateName == "CSdg") {
            return gates::csDaggerGate();
        }

        if (gateName == "CT") {
            return gates::ctGate();
        }

        if (gateName == "CTdg") {
            return gates::ctDaggerGate();
        }

        if (gateName == "CP") {
            return gates::controlledPhaseGate(
                parameters.thetaRadians
            );
        }

        if (gateName == "CRx") {
            return gates::crxGate(parameters.thetaRadians);
        }

        if (gateName == "CRy") {
            return gates::cryGate(parameters.thetaRadians);
        }

        if (gateName == "CRz") {
            return gates::crzGate(parameters.thetaRadians);
        }

        if (gateName == "RXX") {
            return gates::rxxGate(parameters.thetaRadians);
        }

        if (gateName == "RYY") {
            return gates::ryyGate(parameters.thetaRadians);
        }

        if (gateName == "RZZ") {
            return gates::rzzGate(parameters.thetaRadians);
        }

        if (gateName == "DCX") {
            return gates::dcxGate();
        }

        if (gateName == "ECR") {
            return gates::ecrGate();
        }

        if (gateName == "sqrtSWAP") {
            return gates::squareRootSwapGate();
        }

        if (gateName == "fSim") {
            return gates::fSimGate(
                parameters.thetaRadians,
                parameters.phiRadians
            );
        }

        if (gateName == "SWAP") {
            return gates::swapGate();
        }

        if (gateName == "iSWAP") {
            return gates::iSwapGate();
        }

        throw std::invalid_argument(
            "Unsupported controlled gate: " + gateName
        );
    }

    math::ComplexMatrix GuiApplication::createThreeQubitGateMatrix(
        const std::string &gateName
    ) {
        if (gateName == "CCX") {
            return gates::ccxGate();
        }

        if (gateName == "CSWAP") {
            return gates::cSwapGate();
        }

        throw std::invalid_argument{
            "Unsupported three-qubit gate: " +
            gateName
        };
    }


    void GuiApplication::rebuildDebuggerAfterCircuitEdit(
        const std::optional<std::size_t> firstChangedInstruction,
        const std::optional<std::size_t> preferredStep
    ) {
        const std::optional<std::size_t> safeFirstChangedInstruction =
                simulationHistoryWorker_.busy()
                    ? std::nullopt
                    : firstChangedInstruction;

        playbackPaused_ = true;
        simulationBuildError_.clear();

        pendingSimulationRequestId_ =
                simulationHistoryWorker_.request(
                    circuit_,
                    initialState_,
                    session_,
                    densityVolumeStack_,
                    safeFirstChangedInstruction,
                    preferredStep,
                    followManualEdits_
                );
    }

    void GuiApplication::adoptCompletedSimulationHistory() {
        std::optional<SimulationHistoryResult> result =
                simulationHistoryWorker_.takeCompleted();

        if (
            !result.has_value() ||
            result->requestId != pendingSimulationRequestId_
        ) {
            return;
        }

        if (!result->error.empty()) {
            simulationBuildError_ =
                    std::move(result->error);
            return;
        }

        if (!result->session.has_value()) {
            simulationBuildError_ =
                    "Simulation history completed without a debugger trace.";
            return;
        }

        session_ =
                std::move(result->session.value());

        densityVolumeStack_ =
                std::move(result->densityStack);

        if (
            result->followPreferredStep &&
            result->preferredStep.has_value()
        ) {
            session_.moveToStepNumber(
                std::min(
                    result->preferredStep.value(),
                    session_.stepCount()
                )
            );
        }

        if (densityVolumeStack_.layers.empty()) {
            selectedDensityLayer_ = 0U;
        } else {
            selectedDensityLayer_ =
                    std::min(
                        session_.currentStepNumber(),
                        densityVolumeStack_.layers.size() - 1U
                    );
        }

        lastDensityDebuggerStepNumber_.reset();
        densityVolumeCameraFramePending_ = true;
        simulationBuildError_.clear();
        synchronizeDensityLayer(
            session_.snapshot()
        );
    }

    void GuiApplication::saveProject() {
        std::optional<std::filesystem::path> path =
                currentProjectPath_;

        if (!path.has_value()) {
            path =
                    NativeFileDialog::saveProject();
        }

        if (!path.has_value()) {
            return;
        }

        try {
            project::ProjectFile::save(
                path.value(),
                circuit_,
                initialState_
            );

            currentProjectPath_ =
                    path;

            circuitHasUnsavedEdits_ = false;
            projectWorkspace_.discardRecovery();

            if (projectWorkspaceSessionActive_) {
                projectWorkspace_.recordRecentProject(
                    path.value()
                );

                recentProjectPaths_ =
                        projectWorkspace_.recentProjects();
            }

            projectStatusMessage_ =
                    "Saved " +
                    path->filename().string();
        } catch (const std::exception &error) {
            projectStatusMessage_ =
                    std::string{"Save failed: "} +
                    error.what();
        }
    }

    void GuiApplication::applyQueuedProjectOpen() {
        if (!queuedProjectOpenPath_.has_value()) {
            return;
        }

        const bool isRecovery =
                queuedProjectIsRecovery_;

        const std::filesystem::path path =
                std::move(
                    queuedProjectOpenPath_.value()
                );

        queuedProjectOpenPath_.reset();
        queuedProjectIsRecovery_ = false;

        try {
            project::ProjectDocument document =
                    project::ProjectFile::load(path);

            recordEditorForUndo();
            simulationHistoryWorker_.cancel();

            circuit_ =
                    std::move(document.circuit);

            initialState_ =
                    std::move(document.initialState);

            presetQubitCount_ =
                    static_cast<int>(
                        circuit_.qubitCount()
            );

            resetEditorTransientState();
            currentProjectPath_ =
                    isRecovery
                        ? std::nullopt
                        : std::optional<std::filesystem::path>{
                            path
                        };

            circuitHasUnsavedEdits_ =
                    isRecovery;

            playbackPaused_ = true;
            projectStatusMessage_ =
                    isRecovery
                        ? "Recovered unsaved work."
                        : "Opened " +
                          path.filename().string();

            if (
                !isRecovery &&
                projectWorkspaceSessionActive_
            ) {
                projectWorkspace_.recordRecentProject(path);
                recentProjectPaths_ =
                        projectWorkspace_.recentProjects();
            }

            circuitRenderer_.fitToView();
            rebuildDebuggerAfterCircuitEdit();
        } catch (const std::exception &error) {
            projectStatusMessage_ =
                    std::string{"Open failed: "} +
                    error.what();
        }
    }

    void GuiApplication::autosaveProjectIfDue() {
        if (
            !projectWorkspaceSessionActive_ ||
            !circuitHasUnsavedEdits_
        ) {
            return;
        }

        const double now = ImGui::GetTime();

        if (now < nextAutosaveAt_) {
            return;
        }

        try {
            project::ProjectFile::save(
                projectWorkspace_.autosavePath(),
                circuit_,
                initialState_
            );

            projectStatusMessage_ =
                    "Recovery copy updated.";
        } catch (const std::exception &error) {
            projectStatusMessage_ =
                    std::string{"Autosave failed: "} +
                    error.what();
        }

        nextAutosaveAt_ = now + 8.0;
    }

    void GuiApplication::drawRecoveryPrompt() {
        if (
            recoveryPromptPending_ &&
            !recoveryPopupOpened_
        ) {
            ImGui::OpenPopup("Recover unsaved work");
            recoveryPopupOpened_ = true;
        }

        if (
            !ImGui::BeginPopupModal(
                "Recover unsaved work",
                nullptr,
                ImGuiWindowFlags_AlwaysAutoResize
            )
        ) {
            return;
        }

        ImGui::TextUnformatted(
            "QubitCanvas found a recovery copy from an earlier session."
        );

        ImGui::TextDisabled(
            "Recovering opens it as an unsaved project and leaves named files untouched."
        );

        ImGui::Spacing();

        if (ImGui::Button("Recover", ImVec2{130.0F, 0.0F})) {
            queuedProjectOpenPath_ =
                    projectWorkspace_.autosavePath();

            queuedProjectIsRecovery_ = true;
            recoveryPromptPending_ = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Discard", ImVec2{130.0F, 0.0F})) {
            projectWorkspace_.discardRecovery();
            recoveryPromptPending_ = false;
            projectStatusMessage_ =
                    "Recovery copy discarded.";
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    void GuiApplication::recordEditorForUndo() {
        undoHistory_.push_back(
            EditorSnapshot{
                circuit_,
                initialState_,
                circuitHasUnsavedEdits_,
                currentProjectPath_
            }
        );
        redoHistory_.clear();
    }

    void GuiApplication::resetEditorTransientState() noexcept {
        pendingGate_.reset();
        pendingGateParameters_.reset();
        queuedControlledPlacement_.reset();
        queuedThreeQubitPlacement_.reset();
        queuedSingleQubitPlacement_.reset();
        queuedSingleQubitParameters_.reset();
        queuedTwoQubitParameters_.reset();
        queuedInstructionDeletions_.clear();
        queuedInstructionMove_.reset();
        queuedClipboardInsertionIndex_.reset();
        queuedInstructionAngleEdit_.reset();
        inlineAngleInstructionIndex_.reset();
        presetAwaitingConfirmation_.reset();
        gateLibraryPanel_.clearSelection();
        circuitRenderer_.cancelPlacement();
        circuitRenderer_.clearSelection();
        lastInspectorSelection_.reset();
    }
}

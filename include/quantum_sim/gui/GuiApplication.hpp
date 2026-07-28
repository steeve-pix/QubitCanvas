#pragma once

#include "imgui.h"
#include "SimulationHistoryWorker.hpp"
#include "panels/GateLibraryPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/debug/DebuggerSession.hpp"
#include "quantum_sim/project/ProjectWorkspace.hpp"
#include "quantum_sim/project/SubcircuitLibrary.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "rendering/BlochSphereRenderer.hpp"
#include "rendering/CircuitRenderer.hpp"
#include "rendering/DensityVolumeCameraController.hpp"
#include "rendering/DensityVolumeModel.hpp"
#include "rendering/DensityVolumeRenderer.hpp"

#include <optional>
#include <random>
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace quantum_sim::gui {
    /**
     * Optional startup behavior used by unattended visual regression captures.
     */
    struct GuiLaunchOptions {
        bool hiddenWindow{false};
        std::optional<std::string> capturePath;
        std::size_t captureAfterFrames{8U};
        std::optional<std::size_t> algorithmPage;
        std::optional<std::size_t> gatePage;
        std::optional<std::string> armedGate;
        std::optional<std::size_t> selectedInstructionIndex;
        bool startAtFinalStep{false};
        bool startInFloorField{false};
        bool isolateDensityLayer{false};
        std::optional<std::size_t> comparisonDensityLayer;
    };

    /**
     * Built-in circuit scripts available from the left-side algorithm panel.
     */
    enum class CircuitPreset {
        Bell,
        Ghz,
        PlusRegister,
        Qft,
        InverseQft,
        Grover,
        DeutschJozsa,
        BernsteinVazirani,
        Toffoli,
        Kickback,
        Teleportation,
        Scramble,
        Simon,
        Shor,
        Qpe,
        Vqe,
        Qaoa,
        Hhl,
        SwapTest,
        QuantumWalk,
        Bb84,
        Superdense,
        WState,
        DickeState,
        GraphState,
        RandomCircuit,
        WeightedState,
        BitFlipCode,
        SteaneCode,
        ShorCode,
        PhaseFlipCode,
        FiveQubitCode,
        QuantumCounting,
        AmplitudeEstimation,
        RippleCarryAdder,
        DraperAdder,
        Iqp,
        SurfaceCode
    };

    /**
     * Main quantum-state background visualization mode.
     */
    enum class CanvasMode {
        FloorField,
        LayerStack
    };

    /**
     * Dear ImGui/GLFW application shell for the interactive QubitCanvas GUI.
     */
    class GuiApplication {
    public:
        /**
         * Creates a GUI session around an editable circuit and initial register.
         *
         * @param circuit Circuit edited and executed by the UI.
         * @param initialState Starting state used by the debugger trace.
         * @param launchOptions Optional hidden capture and initial-view settings.
         */
        GuiApplication(
            circuit::QuantumCircuit &circuit,
            const quantum::QuantumRegister &initialState,
            GuiLaunchOptions launchOptions = {}
        );

        /**
         * Opens the window and runs the GUI event/render loop until closed.
         *
         * @throws std::runtime_error if GLFW, the window, or fonts fail to initialize.
         */
        void run();

        /**
         * Creates a supported single-qubit gate by display name.
         *
         * @param gateName Supported one-qubit gate display name.
         * @param parameters Angles used by parameterized gates.
         * @return 2x2 unitary gate matrix.
         * @throws std::invalid_argument if the gate is unsupported.
         */
        [[nodiscard]] math::ComplexMatrix createSingleQubitGateMatrix(
            const std::string &gateName,
            const GateParameters &parameters = GateParameters{}
        ) const;

        /**
         * Applies any queued insertion/deletion produced by the UI.
         */
        void applyQueuedCircuitEdits();

        /**
         * Restores the previous circuit-edit snapshot when available.
         */
        void undoLastCircuitEdit();

        /**
         * Reapplies the most recently undone circuit edit when available.
         */
        void redoLastCircuitEdit();

        /**
         * Creates a supported compact two-qubit gate by display name.
         *
         * @param gateName Supported two-qubit gate display name.
         * @param parameters Angles used by parameterized gates.
         * @return Compact 4x4 unitary matrix for the selected two-qubit gate.
         * @throws std::invalid_argument if gateName is not supported.
         */
        [[nodiscard]] static math::ComplexMatrix createTwoQubitGateMatrix(
            const std::string &gateName,
            const GateParameters &parameters = GateParameters{}
        );

        /**
         * Creates a supported compact three-qubit gate by display name.
         *
         * @param gateName `CCX` or `CSWAP`.
         * @return Compact 8x8 unitary matrix.
         * @throws std::invalid_argument if gateName is unsupported.
         */
        [[nodiscard]] static math::ComplexMatrix createThreeQubitGateMatrix(
            const std::string &gateName
        );

    private:
        /**
         * Complete authoring state stored by Undo and Redo.
         *
         * A snapshot includes the register because loading a preset or creating
         * a blank circuit may change the qubit count as well as the gates.
         */
        struct EditorSnapshot {
            circuit::QuantumCircuit circuit;
            quantum::QuantumRegister initialState;
            bool hasUnsavedEdits{false};
            std::optional<std::filesystem::path> projectPath;
        };

        /**
         * Deferred parameter edit applied before the next simulation rebuild.
         */
        struct InstructionAngleEdit {
            std::size_t instructionIndex{};
            double angleRadians{};
            bool recordUndo{true};
        };

        ImFont *jetBrainsMonoFont_{nullptr};
        ImFont *jetBrainsMonoHeadingFont_{nullptr};
        circuit::QuantumCircuit &circuit_;
        quantum::QuantumRegister initialState_;
        debug::DebuggerSession session_;
        InspectorPanel inspectorPanel_;
        CircuitRenderer circuitRenderer_;
        density_volume::Renderer densityVolumeRenderer_;
        density_volume::CameraController densityVolumeCamera_;
        density_volume::DensityStack densityVolumeStack_;
        SimulationHistoryWorker simulationHistoryWorker_;
        GateLibraryPanel gateLibraryPanel_;
        std::optional<std::string> pendingGate_;
        std::optional<GateParameters> pendingGateParameters_;
        std::optional<ControlledPlacement> queuedControlledPlacement_;
        std::optional<ThreeQubitPlacement> queuedThreeQubitPlacement_;
        std::optional<SingleQubitPlacement> queuedSingleQubitPlacement_;
        std::optional<GateParameters> queuedSingleQubitParameters_;
        std::optional<GateParameters> queuedTwoQubitParameters_;
        std::vector<std::size_t> queuedInstructionDeletions_;
        std::optional<InstructionMove> queuedInstructionMove_;
        std::vector<circuit::CircuitInstructionSnapshot> instructionClipboard_;
        std::optional<std::size_t> queuedClipboardInsertionIndex_;
        std::optional<InstructionAngleEdit> queuedInstructionAngleEdit_;
        std::optional<std::size_t> inlineAngleInstructionIndex_;
        float inlineAnglePiCoefficient_{0.5F};
        bool inlineAngleEditActive_{false};
        double nextInlineAnglePreviewAt_{0.0};
        project::SubcircuitLibrary subcircuitLibrary_;
        std::vector<project::StoredSubcircuit> reusableSubcircuits_;
        std::size_t selectedReusableSubcircuit_{0U};
        std::array<char, 64U> reusableSubcircuitName_{};
        std::optional<CircuitPreset> queuedPreset_;
        std::optional<CircuitPreset> presetAwaitingConfirmation_;
        std::optional<std::size_t> queuedBlankCircuitQubitCount_;
        bool queuedClearCircuit_{false};
        std::vector<EditorSnapshot> undoHistory_;
        std::vector<EditorSnapshot> redoHistory_;
        std::mt19937 randomEngine_{std::random_device{}()};
        CanvasMode canvasMode_{CanvasMode::LayerStack};
        int presetQubitCount_{4};
        std::size_t algorithmPage_{0U};
        bool playbackPaused_{true};
        double nextAutoStepAt_{0.0};
        /**
         * Fixed emissive intensity used by both density visualization modes.
         *
         * Keeping this value internal gives Layer Stack and Floor Field a
         * consistent presentation without exposing a duplicate palette control.
         */
        static constexpr float densityVolumeHeatAmount_{1.5F};
        std::string lastSampleLabel_{"none"};
        std::size_t selectedDensityLayer_{};
        std::optional<std::size_t> lastDensityDebuggerStepNumber_;
        bool densityVolumePointerDragged_{false};
        bool densityVolumeCameraFramePending_{true};
        bool isolateDensityLayer_{false};
        bool compareDensityLayers_{false};
        std::size_t comparisonDensityLayer_{};
        bool circuitFocusMode_{false};
        bool followManualEdits_{true};
        bool circuitHasUnsavedEdits_{false};
        std::optional<std::size_t> lastInspectorSelection_;
        GuiLaunchOptions launchOptions_;
        std::size_t renderedFrameCount_{0U};
        std::uint64_t pendingSimulationRequestId_{};
        std::string simulationBuildError_;
        std::optional<std::filesystem::path> currentProjectPath_;
        std::optional<std::filesystem::path> queuedProjectOpenPath_;
        std::optional<std::filesystem::path> queuedQasmOpenPath_;
        bool queuedProjectIsRecovery_{false};
        std::string projectStatusMessage_;
        project::ProjectWorkspace projectWorkspace_;
        std::vector<std::filesystem::path> recentProjectPaths_;
        bool projectWorkspaceSessionActive_{false};
        bool recoveryPromptPending_{false};
        bool recoveryPopupOpened_{false};
        double nextAutosaveAt_{0.0};

        /**
         * Rebuilds debugger state after the editable circuit changes.
         *
         * @param firstChangedInstruction Earliest affected instruction. Omit
         *        when the entire circuit may have changed.
         * @param preferredStep Step to show when followManualEdits_ is enabled.
         */
        void rebuildDebuggerAfterCircuitEdit(
            std::optional<std::size_t> firstChangedInstruction = std::nullopt,
            std::optional<std::size_t> preferredStep = std::nullopt
        );

        /**
         * Recomputes the shared 2D/3D density history after a trace rebuild.
         */
        void rebuildDensityVolume(
            std::optional<std::size_t> firstChangedInstruction = std::nullopt
        );

        /**
         * Atomically adopts the newest completed background history.
         *
         * Stale generations are ignored, and failed builds leave the last
         * complete simulation visible while exposing a concise status error.
         */
        void adoptCompletedSimulationHistory();

        /**
         * Writes the current circuit to its existing path or asks for one.
         */
        void saveProject();

        /**
         * Loads the project path selected during the previous UI frame.
         */
        void applyQueuedProjectOpen();

        /**
         * Imports a queued OpenQASM document as an unnamed editable circuit.
         */
        void applyQueuedQasmOpen();

        /**
         * Writes unsaved work to the separate recovery project when due.
         */
        void autosaveProjectIfDue();

        /**
         * Offers to restore or discard a durable recovery project.
         */
        void drawRecoveryPrompt();

        /**
         * Follows debugger navigation while preserving an explicit initial-layer selection.
         *
         * @param snapshot Current debugger navigation state.
         */
        void synchronizeDensityLayer(const debug::DebuggerSnapshot &snapshot);

        /**
         * Selects one density layer and synchronizes post-gate layers to the debugger.
         *
         * @param layerIndex Initial or post-gate density layer index.
         */
        void selectDensityLayer(std::size_t layerIndex);

        /**
         * Stores the complete current editor state and clears stale redo history.
         */
        void recordEditorForUndo();

        /**
         * Clears placement, selection, and queued instruction state after a
         * whole-circuit replacement.
         */
        void resetEditorTransientState() noexcept;

        /**
         * Applies QubitCanvas colors, spacing, and rounding to the ImGui style.
         */
        void configureStyle() const;

        /**
         * Pushes the regular JetBrains Mono font for the whole GUI frame.
         */
        void pushApplicationFont() const;

        /**
         * Pops the regular JetBrains Mono font pushed for the GUI frame.
         */
        void popApplicationFont() const;

        /**
         * Handles application-wide keyboard commands that are safe outside text input.
         *
         * Escape cancels an armed gate placement and Space toggles playback.
         */
        void handleGlobalShortcuts();

        /**
         * Arms a gate for repeated placement and synchronizes its palette state.
         *
         * @param gateName Gate display name.
         */
        void armGatePlacement(std::string gateName);

        /**
         * Clears all active and partially completed gate-placement state.
         */
        void cancelGatePlacement() noexcept;

        /**
         * Draws the application background behind the docked panels.
         */
        void drawBackdrop() const;

        /**
         * Draws a live angle editor anchored above the selected circuit gate.
         *
         * The overlay is an interactive child drawn above the circuit canvas.
         * Keeping both surfaces in the same parent gives the slider pointer
         * ownership without changing the circuit layout or interrupting a
         * double-click on the gate.
         *
         * @param panelPosition Circuit panel top-left corner in screen space.
         * @param panelSize Circuit panel dimensions.
         */
        void drawSelectedGateAngleEditor(
            const ImVec2 &panelPosition,
            const ImVec2 &panelSize
        );

        /**
         * Displays the OpenGL framebuffer texture in the center Density Volume panel.
         *
         * @param position Screen-space top-left corner of the panel.
         * @param size Screen-space width and height of the panel.
         */
        void drawDensityVolumeViewport(const ImVec2 &position, const ImVec2 &size);

        /**
         * Draws playback, scrub, view-mode, and sampling controls.
         *
         * @param session Debugger session controlled by the top bar.
         * @param snapshot Current debugger state.
         */
        void drawTopBar(debug::DebuggerSession &session, const debug::DebuggerSnapshot &snapshot);

        /**
         * Draws OpenQASM, diagram, numerical export, and recent-file commands.
         */
        void drawExchangeMenu(
            const debug::DebuggerSnapshot &snapshot
        );

        /**
         * Draws built-in algorithm script buttons and visualization controls.
         */
        void drawAlgorithmScripts();

        /**
         * Draws the compact persistent-block chooser in the Gate Library.
         */
        void drawReusableSubcircuits();

        /**
         * Draws the compact bottom status strip.
         *
         * @param snapshot Current debugger state.
         */
        void drawBottomStatus(const debug::DebuggerSnapshot &snapshot) const;

        /**
         * Advances the debugger while playback is active.
         *
         * @param session Debugger session to advance.
         * @param snapshot Snapshot used to decide whether movement is possible.
         */
        void applyPlayback(debug::DebuggerSession &session, const debug::DebuggerSnapshot &snapshot);

        /**
         * Replaces the editable circuit with one of the built-in presets.
         *
         * @param preset Preset to load.
         *
         * Loading always pauses playback and displays step zero so the
         * untouched initial register is visible before any gate executes.
         */
        void loadPreset(CircuitPreset preset);

        /**
         * Applies a preset queued by the previous frame before snapshots are created.
         */
        void applyQueuedPreset();

        /**
         * Replaces the editor with an empty circuit using the requested register size.
         *
         * @param qubitCount Number of qubits in the new blank circuit.
         */
        void createBlankCircuit(std::size_t qubitCount);

        /**
         * Removes every instruction while preserving the current register size
         * and initial state.
         */
        void clearCircuitInstructions();

        /**
         * Builds a circuit for one built-in preset.
         *
         * @param preset Preset to build.
         * @return Circuit whose register size matches presetQubitCount_.
         */
        [[nodiscard]] circuit::QuantumCircuit createPresetCircuit(CircuitPreset preset) const;

        /**
         * Returns the smallest register accepted by a built-in circuit.
         *
         * @param preset Preset whose requirement should be queried.
         * @return Minimum supported qubit count.
         */
        [[nodiscard]] static int minimumQubitCount(CircuitPreset preset) noexcept;

        /**
         * Samples the current state and stores a display label for the status bar.
         *
         * @param state Register state to measure.
         */
        void sampleCurrentState(const quantum::QuantumRegister &state);

        bool showHistoryDebugInfo_{false};
    };
}

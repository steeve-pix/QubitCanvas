#pragma once

#include "imgui.h"
#include "panels/GateLibraryPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/debug/DebuggerSession.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "rendering/BlochSphereRenderer.hpp"
#include "rendering/CircuitRenderer.hpp"
#include "rendering/DensityVolumeCameraController.hpp"
#include "rendering/DensityVolumeModel.hpp"
#include "rendering/DensityVolumeRenderer.hpp"

#include <optional>
#include <random>
#include <string>
#include <vector>

namespace quantum_sim::gui {
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
        Scramble
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
         */
        GuiApplication(circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState);

        /**
         * Opens the window and runs the GUI event/render loop until closed.
         *
         * @throws std::runtime_error if GLFW, the window, or fonts fail to initialize.
         */
        void run();

        /**
         * Creates a supported single-qubit gate by display name.
         *
         * @param gateName Gate name such as H, X, Y, Z, S, Sdg, T, Tdg, Rx, Ry, or Rz.
         * @param angleRadians Required rotation angle for Rx, Ry, and Rz.
         * @return 2x2 unitary gate matrix.
         * @throws std::invalid_argument if the gate is unsupported or a rotation has no angle.
         */
        [[nodiscard]] math::ComplexMatrix createSingleQubitGateMatrix(
            const std::string &gateName,
            std::optional<double> angleRadians = std::nullopt
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
         * @param gateName Gate name such as CX, CY, CZ, SWAP, or iSWAP.
         * @return Compact 4x4 unitary matrix for the selected two-qubit gate.
         * @throws std::invalid_argument if gateName is not supported.
         */
        [[nodiscard]] static math::ComplexMatrix createTwoQubitGateMatrix(
            const std::string &gateName
        );

    private:
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
        GateLibraryPanel gateLibraryPanel_;
        std::optional<std::string> pendingGate_;
        std::optional<double> pendingRotationAngleRadians_;
        std::optional<ControlledPlacement> queuedControlledPlacement_;
        std::optional<SingleQubitPlacement> queuedSingleQubitPlacement_;
        std::optional<double> queuedSingleQubitRotationAngleRadians_;
        std::optional<std::size_t> queuedInstructionDeletion_;
        std::optional<CircuitPreset> queuedPreset_;
        std::vector<circuit::QuantumCircuit> undoHistory_;
        std::vector<circuit::QuantumCircuit> redoHistory_;
        std::mt19937 randomEngine_{std::random_device{}()};
        CanvasMode canvasMode_{CanvasMode::LayerStack};
        int presetQubitCount_{4};
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

        /**
         * Rebuilds debugger state after the editable circuit changes.
         */
        void rebuildDebuggerAfterCircuitEdit();

        /**
         * Recomputes the shared 2D/3D density history after a trace rebuild.
         */
        void rebuildDensityVolume();

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
         * Stores the current circuit for undo and clears stale redo history.
         */
        void recordCircuitForUndo();

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
         * Draws the application background behind the docked panels.
         */
        void drawBackdrop() const;

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
         * Draws built-in algorithm script buttons and visualization controls.
         */
        void drawAlgorithmScripts();

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

#pragma once

#include "imgui.h"
#include "panels/GateLibraryPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/debug/DebuggerSession.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "rendering/BlochSphereRenderer.hpp"
#include "rendering/CircuitRenderer.hpp"

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
         * @param gateName Gate name such as H, X, Y, Z, S, T, Rx, Ry, or Rz.
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
         * Creates a supported two-qubit/full-register gate by display name.
         *
         * @param gateName Gate name such as CX, CY, CZ, SWAP, or iSWAP.
         * @param controlQubit Control or first qubit.
         * @param targetQubit Target or second qubit.
         * @return Full-register unitary matrix sized for circuit_.
         * @throws std::invalid_argument if gateName is not supported.
         */
        [[nodiscard]] math::ComplexMatrix createControlledGateMatrix(const std::string &gateName,
                                                                     std::size_t controlQubit, std::size_t targetQubit) const;

    private:
        ImFont *jetBrainsMonoFont_{nullptr};
        ImFont *jetBrainsMonoHeadingFont_{nullptr};
        circuit::QuantumCircuit &circuit_;
        quantum::QuantumRegister initialState_;
        debug::DebuggerSession session_;
        InspectorPanel inspectorPanel_;
        CircuitRenderer circuitRenderer_;
        GateLibraryPanel gateLibraryPanel_;
        std::optional<std::string> pendingGate_;
        std::optional<double> pendingRotationAngleRadians_;
        std::optional<ControlledPlacement> queuedControlledPlacement_;
        std::optional<SingleQubitPlacement> queuedSingleQubitPlacement_;
        std::optional<double> queuedSingleQubitRotationAngleRadians_;
        std::optional<std::size_t> queuedInstructionDeletion_;
        std::vector<circuit::QuantumCircuit> undoHistory_;
        std::vector<circuit::QuantumCircuit> redoHistory_;
        std::mt19937 randomEngine_{std::random_device{}()};
        CanvasMode canvasMode_{CanvasMode::LayerStack};
        int presetQubitCount_{4};
        bool playbackPaused_{true};
        double nextAutoStepAt_{0.0};
        float heatAmount_{0.78F};
        float renderYaw_{-0.46F};
        float renderPitch_{0.24F};
        float renderZoom_{1.30F};
        ImVec2 renderPan_{0.0F, 0.0F};
        std::string lastSampleLabel_{"none"};

        /**
         * Rebuilds debugger state after the editable circuit changes.
         */
        void rebuildDebuggerAfterCircuitEdit();

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
         * Draws the animated quantum-state background visualization.
         *
         * @param session Debugger trace used to render full-history layers.
         * @param snapshot Current debugger snapshot used as the source state.
         */
        void drawBackdrop(const debug::DebuggerSession &session, const debug::DebuggerSnapshot &snapshot);

        /**
         * Applies mouse orbit, pan, zoom, and reset gestures to the state render.
         *
         * @param minimum Screen-space top-left corner of the interactive render area.
         * @param maximum Screen-space bottom-right corner of the interactive render area.
         */
        void handleRenderCameraInput(const ImVec2 &minimum, const ImVec2 &maximum);

        /**
         * Restores the state-volume camera to its reference composition.
         */
        void resetRenderCamera();

        /**
         * Opens rich demo traces at a settled mid-history frame.
         */
        void settleDebuggerPreview();

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
         */
        void loadPreset(CircuitPreset preset);

        /**
         * Builds a circuit for one built-in preset.
         *
         * @param preset Preset to build.
         * @return Newly constructed circuit.
         */
        [[nodiscard]] circuit::QuantumCircuit createPresetCircuit(CircuitPreset preset) const;

        /**
         * Samples the current state and stores a display label for the status bar.
         *
         * @param state Register state to measure.
         */
        void sampleCurrentState(const quantum::QuantumRegister &state);

        bool showHistoryDebugInfo_{false};
    };
}

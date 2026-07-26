#pragma once

#include "imgui.h"
#include "panels/GateLibraryPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/debug/DebuggerSession.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "rendering/BlochSphereRenderer.hpp"
#include "rendering/CircuitRenderer.hpp"
#include "rendering/QaveCameraController.hpp"
#include "rendering/QaveDensityModel.hpp"
#include "rendering/QaveRenderer.hpp"

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
        qave::Renderer qaveRenderer_;
        qave::CameraController qaveCamera_;
        qave::DensityStack qaveDensityStack_;
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
        std::string lastSampleLabel_{"none"};
        std::size_t selectedQaveLayer_{};
        std::optional<std::size_t> lastQaveDebuggerStep_;
        bool qavePointerDragged_{false};

        /**
         * Rebuilds debugger state after the editable circuit changes.
         */
        void rebuildDebuggerAfterCircuitEdit();

        /**
         * Recomputes the shared 2D/3D density history after a trace rebuild.
         */
        void rebuildQaveDensityStack();

        /**
         * Follows debugger navigation while preserving an explicit initial-layer selection.
         *
         * @param snapshot Current debugger navigation state.
         */
        void synchronizeQaveLayer(const debug::DebuggerSnapshot &snapshot);

        /**
         * Selects one density layer and synchronizes post-gate layers to the debugger.
         *
         * @param layerIndex Initial or post-gate density layer index.
         */
        void selectQaveLayer(std::size_t layerIndex);

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
         * Draws the application background behind the docked panels.
         */
        void drawBackdrop() const;

        /**
         * Displays the OpenGL framebuffer texture in the center QAVE panel.
         *
         * @param position Screen-space top-left corner of the panel.
         * @param size Screen-space width and height of the panel.
         */
        void drawQaveViewport(const ImVec2 &position, const ImVec2 &size);

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

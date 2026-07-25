#pragma once

#include "imgui.h"
#include "panels/GateLibraryPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/debug/DebuggerSession.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "rendering/BlochSphereRenderer.hpp"
#include "rendering/CircuitRenderer.hpp"
#include <random>
#include <optional>
#include <string>
#include <vector>

namespace quantum_sim::gui {
    enum class CircuitPreset {
        Bell,
        Ghz,
        PlusRegister,
        PhaseLadder,
        EntangleChain,
        Scramble
    };

    enum class CanvasMode {
        FloorField,
        LayerStack
    };

    class GuiApplication {
    public:
        GuiApplication(circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState);

        void run();

        [[nodiscard]] math::ComplexMatrix createSingleQubitGateMatrix(const std::string &gateName) const;

        void applyQueuedCircuitEdits();

        void undoLastCircuitEdit();

        void redoLastCircuitEdit();

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
        std::optional<ControlledPlacement> queuedControlledPlacement_;
        std::optional<SingleQubitPlacement> queuedSingleQubitPlacement_;
        std::optional<std::size_t> queuedInstructionDeletion_;
        std::vector<circuit::QuantumCircuit> undoHistory_;
        std::vector<circuit::QuantumCircuit> redoHistory_;
        std::mt19937 randomEngine_{std::random_device{}()};
        CanvasMode canvasMode_{CanvasMode::FloorField};
        int presetQubitCount_{3};
        bool playbackPaused_{true};
        double nextAutoStepAt_{0.0};
        float heatAmount_{0.78F};
        std::string lastSampleLabel_{"none"};

        void rebuildDebuggerAfterCircuitEdit();

        void recordCircuitForUndo();

        void configureStyle() const;

        void pushApplicationFont() const;

        void popApplicationFont() const;

        void drawBackdrop(const debug::DebuggerSnapshot &snapshot) const;

        void drawTopBar(debug::DebuggerSession &session, const debug::DebuggerSnapshot &snapshot);

        void drawAlgorithmScripts();

        void drawBottomStatus(const debug::DebuggerSnapshot &snapshot) const;

        void applyPlayback(debug::DebuggerSession &session, const debug::DebuggerSnapshot &snapshot);

        void loadPreset(CircuitPreset preset);

        [[nodiscard]] circuit::QuantumCircuit createPresetCircuit(CircuitPreset preset) const;

        void sampleCurrentState(const quantum::QuantumRegister &state);

        bool showHistoryDebugInfo_{false};
    };
}

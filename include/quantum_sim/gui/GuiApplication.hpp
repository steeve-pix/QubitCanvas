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
#include <string>
#include <vector>

namespace quantum_sim::gui {
    class GuiApplication {
    public:
        GuiApplication(circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState);

        void run();

        [[nodiscard]] math::ComplexMatrix createSingleQubitGateMatrix(const std::string &gateName) const;

        void applyQueuedCircuitEdits();

        void undoLastCircuitEdit();

        void redoLastCircuitEdit();

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

        void rebuildDebuggerAfterCircuitEdit();

        void recordCircuitForUndo();

        bool showHistoryDebugInfo_{false};
    };
}

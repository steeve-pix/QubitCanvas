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

namespace quantum_sim::gui {
    class GuiApplication {
    public:
        GuiApplication(const circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState);

        void run();

    private:
        ImFont *jetBrainsMonoFont_{nullptr};
        ImFont *jetBrainsMonoHeadingFont_{nullptr};
        circuit::QuantumCircuit circuit_;
        debug::DebuggerSession session_;
        InspectorPanel inspectorPanel_;
        CircuitRenderer circuitRenderer_;
        GateLibraryPanel gateLibraryPanel_;
        std::optional<std::string> pendingGate_;
    };
}

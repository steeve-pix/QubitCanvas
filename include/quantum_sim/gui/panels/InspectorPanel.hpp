#pragma once

#include "imgui.h"

#include <cstddef>
#include <optional>

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/debug/DebuggerSession.hpp"
#include "quantum_sim/gui/rendering/BlochSphereRenderer.hpp"

namespace quantum_sim::gui {
    class InspectorPanel {
    public:
        void draw(
            debug::DebuggerSession &session,
            const debug::DebuggerSnapshot &snapshot,
            const circuit::QuantumCircuit &circuit,
            std::optional<std::size_t> selectedInstructionIndex,
            ImFont *headingFont
        );

    private:
        BlochSphereRenderer blochSphereRenderer_;
    };
}

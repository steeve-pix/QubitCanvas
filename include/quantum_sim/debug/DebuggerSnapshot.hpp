#pragma once
#include <cstddef>
#include <functional>
#include <optional>

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

namespace quantum_sim::debug {
    /**
     * Read-only view of the debugger state for one UI frame.
     *
     * References point back into DebuggerSession-owned storage and remain valid until
     * the session is rebuilt or advanced again.
     */
    struct DebuggerSnapshot {
        std::size_t currentStepIndex;
        std::size_t stepCount;
        std::optional<std::reference_wrapper<const circuit::CircuitInstructionInfo> > instruction;
        std::reference_wrapper<const quantum::QuantumRegister> beforeState;
        std::reference_wrapper<const quantum::QuantumRegister> afterState;
        bool canMoveNext;
        bool canMovePrevious;
    };
}

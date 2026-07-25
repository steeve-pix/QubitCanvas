#pragma once
#include <cstddef>
#include <functional>
#include <optional>

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

namespace quantum_sim::debug {
    struct DebuggerSnapshot {
        std::size_t currentStepIndex, stepCount;
        std::optional<std::reference_wrapper<const circuit::CircuitInstructionInfo> > instruction;
        std::reference_wrapper<const quantum::QuantumRegister> beforeState;
        std::reference_wrapper<const quantum::QuantumRegister> afterState;
        bool canMoveNext, canMovePrevious;
    };
}

#pragma once
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include <optional>
#include <ostream>

namespace quantum_sim::debug {
    void runInteractiveDebugger(const circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState);
}

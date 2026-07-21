#pragma once
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include <optional>
#include <ostream>

namespace quantum_sim::debug {
    [[nodiscard]] std::string gateExplanation(const std::string &gateName);

    void runInteractiveDebugger(const circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState);
}

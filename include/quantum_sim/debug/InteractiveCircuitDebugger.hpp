#pragma once
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include <optional>
#include <ostream>

namespace quantum_sim::debug {
    void runInteractiveDebugger(const circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState);

    void printCircuitDiagram(const circuit::QuantumCircuit &circuit, std::ostream &output,
                             std::optional<std::size_t> currentInstruction = std::nullopt);
}

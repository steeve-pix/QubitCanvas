#pragma once
#include "quantum_sim/circuit/QuantumCircuit.hpp"

namespace quantum_sim::debug {
    void runInteractiveDebugger(const circuit::QuantumCircuit& circuit,quantum::QuantumRegister& initialState);
}

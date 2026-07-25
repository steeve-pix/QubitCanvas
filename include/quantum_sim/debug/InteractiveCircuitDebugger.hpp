#pragma once
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include <string>

namespace quantum_sim::debug {
    /**
     * Looks up a short explanation for a supported gate name.
     *
     * @param gateName Display name such as H, CX, SWAP, Rx, Ry, or Rz.
     * @return Human-readable explanation, or a fallback message for unknown gates.
     */
    [[nodiscard]] std::string gateExplanation(const std::string &gateName);

    /**
     * Runs the console debugger for a circuit and starting register.
     *
     * @param circuit Circuit to inspect.
     * @param initialState Register used as the execution start.
     */
    void runInteractiveDebugger(const circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState);
}

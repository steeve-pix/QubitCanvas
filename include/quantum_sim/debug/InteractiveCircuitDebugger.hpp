#pragma once
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include <optional>
#include <ostream>

namespace quantum_sim::debug {
    /**
     * Provides a textual explanation of the functionality and behavior of a specified quantum gate.
     *
     * @param gateName The name of the quantum gate for which the explanation is requested.
     *                 Examples include "H", "CX", "X", "Y", "Z", "S", "T", "SWAP", "Rx", "Ry", "Rz".
     *
     * @return A string containing a human-readable explanation of the gate's operation
     *         and its effect in the quantum context. Returns "No explanation is available for this gate yet."
     *         if the given gateName is not recognized.
     */
    [[nodiscard]] std::string gateExplanation(const std::string &gateName);

    /**
     * Runs an interactive debugger for a given quantum circuit, allowing for
     * step-by-step inspection of its execution and the state of the quantum register.
     *
     * This debugger provides functionalities such as visualizing the circuit,
     * inspecting quantum state amplitudes, comparing states before and after
     * each instruction, and navigating through the circuit's instructions manually
     * or in an autoplay mode.
     *
     * @param circuit The quantum circuit to be debugged. This contains the instructions
     *                and structure of the quantum computation to be examined interactively.
     * @param initialState The initial quantum register state to execute the circuit on.
     *                     This serves as the starting point for the debugger.
     */
    void runInteractiveDebugger(const circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState);
}

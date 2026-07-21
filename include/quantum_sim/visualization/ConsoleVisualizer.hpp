#pragma once

#include "quantum_sim/quantum/QuantumRegister.hpp"

#include <cstddef>
#include <iosfwd>
#include <vector>
#include <ostream>

#include "quantum_sim/circuit/QuantumCircuit.hpp"

namespace quantum_sim::visualization {
    /**
     * @brief Generates a visual representation of quantum state probabilities
     *        as bar charts and writes it to the output stream.
     *
     * @param state The quantum register containing the states and probabilities to visualize.
     * @param output The output stream where the probability bar chart will be written.
     * @param barWidth The maximum width of the bar chart for each state, in characters.
     */
    void printProbabilityBars(const quantum::QuantumRegister &state, std::ostream &output, std::size_t barWidth = 50);

    /**
     * @brief Prints a visualization of the measurement results as a bar chart to the provided output stream.
     *
     * The method creates a textual representation of the measurement frequencies for each quantum state
     * in a quantum register. Each bar represents the frequency of a specific state based on the counts
     * provided in the input. The length of the bar represents the relative frequency, scaled by the given bar width.
     *
     * @param state The quantum register containing the quantum states to be displayed.
     * @param counts A vector containing the measurement counts for each quantum state in the register.
     *               The size of the vector must match the number of states in the register.
     * @param output The output stream where the textual bar chart will be printed.
     * @param barWidth The total width (in characters) of each bar in the visual representation.
     *                 Higher values result in more detailed representations.
     *
     * @throws std::invalid_argument if the size of `counts` does not match the number of states in the `state`.
     */
    void printShotBars(const quantum::QuantumRegister &state, const std::vector<std::size_t> &counts,
                       std::ostream &output, std::size_t barWidth = 50);

    /**
     * Prints the execution trace of a quantum computation, including the initial state and the subsequent states
     * after each step in the trace. The visualization is rendered as probability bars to the specified output stream.
     *
     * @param initialState The initial state of the quantum register before any trace steps are applied.
     * @param trace A vector of trace steps representing the sequence of operations and the respective states
     *              of the quantum register after each step.
     * @param output The output stream to which the execution trace and probability bars should be written.
     * @param barWidth The width of the probability bars used for visualizing the quantum state.
     */
    void printExecutionTrace(const quantum::QuantumRegister &initialState, const std::vector<circuit::TraceStep> &trace,
                             std::ostream &output, std::size_t barWidth = 50);

    /**
     * Prints a textual representation of a quantum circuit diagram to the specified output stream.
     *
     * The method iterates through the qubits in the circuit and visualizes the quantum operations applied to them.
     * For each instruction, it determines its type (single-qubit gate, controlled gate, etc.) and generates
     * an appropriate visualization for the circuit. Additionally, connections between control and target qubits
     * for controlled gates are displayed below the diagram.
     *
     * @param circuit Reference to the QuantumCircuit object containing the circuit's qubits and instructions.
     * @param output Reference to the std::ostream object where the circuit diagram will be printed.
     */
    void printCircuitDiagram(const circuit::QuantumCircuit &circuit, std::ostream &output,
                             std::optional<std::size_t> currentInstruction = std::nullopt);
}

#pragma once

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <vector>

namespace quantum_sim::visualization {
    /**
     * Prints one probability bar per basis state.
     *
     * @param state Register whose probabilities are printed.
     * @param output Destination stream.
     * @param barWidth Maximum bar width in characters.
     */
    void printProbabilityBars(const quantum::QuantumRegister &state, std::ostream &output, std::size_t barWidth = 50);

    /**
     * Prints measurement shot counts as scaled bars.
     *
     * @param state Register used for state labels.
     * @param counts Measurement counts indexed by basis state.
     * @param output Destination stream.
     * @param barWidth Maximum bar width in characters.
     * @throws std::invalid_argument if counts size does not match state.stateCount().
     */
    void printShotBars(const quantum::QuantumRegister &state, const std::vector<std::size_t> &counts,
                       std::ostream &output, std::size_t barWidth = 50);

    /**
     * Prints the initial state followed by each traced step.
     *
     * @param initialState State before the first instruction.
     * @param trace Execution trace returned by QuantumCircuit::executeWithTrace().
     * @param output Destination stream.
     * @param barWidth Maximum probability bar width in characters.
     */
    void printExecutionTrace(const quantum::QuantumRegister &initialState, const std::vector<circuit::TraceStep> &trace,
                             std::ostream &output, std::size_t barWidth = 50);

    /**
     * Prints an ASCII circuit diagram.
     *
     * @param circuit Circuit to draw.
     * @param output Destination stream.
     * @param currentInstruction Optional instruction to highlight.
     */
    void printCircuitDiagram(const circuit::QuantumCircuit &circuit, std::ostream &output,
                             std::optional<std::size_t> currentInstruction = std::nullopt);

    /**
     * Prints all amplitudes in basis-state order.
     *
     * @param state Register whose amplitudes are printed.
     * @param output Destination stream.
     */
    void printAmplitudes(const quantum::QuantumRegister &state, std::ostream &output);

    /**
     * Prints per-state amplitude/probability differences.
     *
     * @param beforeState State before a transformation.
     * @param afterState State after a transformation.
     * @param output Destination stream.
     * @throws std::invalid_argument if the registers have different state counts.
     */
    void printStateComparison(const quantum::QuantumRegister &beforeState, const quantum::QuantumRegister &afterState,
                              std::ostream &output);

    /**
     * Prints Bloch-vector coordinates and angles for a single-qubit state.
     *
     * @param state Single-qubit register.
     * @param output Destination stream.
     * @throws std::invalid_argument if state does not contain exactly one qubit.
     */
    void printBlochVector(const quantum::QuantumRegister &state, std::ostream &output);

    /**
     * Prints a compact ASCII Bloch-sphere projection.
     *
     * @param state Single-qubit register.
     * @param output Destination stream.
     * @throws std::invalid_argument if state does not contain exactly one qubit.
     */
    void printAsciiBlochSphere(const quantum::QuantumRegister &state, std::ostream &output);
}

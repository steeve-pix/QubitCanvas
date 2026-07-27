#pragma once

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include <cstddef>

namespace quantum_sim::algorithms {
    /**
     * Builds a Bell-state demonstration in the leading two qubits.
     *
     * Additional register qubits remain in their initial state.
     *
     * @param qubitCount Total register size. Must be at least 2.
     * @return Circuit containing H(q0) and CX(q0 -> q1).
     * @throws std::invalid_argument if qubitCount is less than 2.
     */
    [[nodiscard]] circuit::QuantumCircuit bellStateCircuit(
        std::size_t qubitCount = 2U
    );

    /**
     * Builds a register-wide equal superposition circuit.
     *
     * @param qubitCount Number of qubits to place into superposition.
     * @return Circuit containing one Hadamard gate per qubit.
     */
    [[nodiscard]] circuit::QuantumCircuit equalSuperpositionCircuit(std::size_t qubitCount);

    /**
     * Builds a QFT-style phase-history showcase circuit for the GUI.
     *
     * @param qubitCount Number of qubits to use in the scripted circuit.
     * @return Circuit with Hadamards, decomposed phase interactions, swaps, and a final phase pass.
     * @throws std::invalid_argument if qubitCount is zero.
     */
    [[nodiscard]] circuit::QuantumCircuit qftCircuit(std::size_t qubitCount);

    /**
     * Builds the exact inverse of the QFT showcase circuit.
     *
     * @param qubitCount Number of qubits to use in the scripted circuit.
     * @return Circuit containing the reversed swaps, phase interactions, and rotations.
     * @throws std::invalid_argument if qubitCount is zero or cannot be represented safely.
     */
    [[nodiscard]] circuit::QuantumCircuit inverseQftCircuit(std::size_t qubitCount);

    /**
     * Builds a GHZ state across the complete selected register.
     *
     * @param qubitCount Number of qubits in the GHZ chain.
     * @return Circuit containing H(q0) followed by nearest-neighbor CX gates.
     * @throws std::invalid_argument if qubitCount is zero.
     */
    [[nodiscard]] circuit::QuantumCircuit ghzStateCircuit(
        std::size_t qubitCount = 3U
    );

    /**
     * Builds the canonical two-qubit Grover search for the marked state |11⟩.
     * in the leading two qubits.
     *
     * Additional register qubits remain in their initial state.
     *
     * @param qubitCount Total register size. Must be at least 2.
     * @return Circuit with one phase-oracle and diffusion iteration on q0/q1.
     * @throws std::invalid_argument if qubitCount is less than 2.
     */
    [[nodiscard]] circuit::QuantumCircuit groverSearchCircuit(
        std::size_t qubitCount = 2U
    );

    /**
     * Builds a balanced Deutsch-Jozsa demonstration.
     *
     * q0 through q(n - 2) are input qubits and q(n - 1) is the oracle ancilla.
     * The balanced function is the parity of every input bit.
     *
     * @param qubitCount Total register size. Must be at least 2.
     * @return Circuit using qubitCount - 1 inputs and one ancilla.
     * @throws std::invalid_argument if qubitCount is less than 2.
     */
    [[nodiscard]] circuit::QuantumCircuit deutschJozsaCircuit(
        std::size_t qubitCount = 3U
    );

    /**
     * Builds a Bernstein-Vazirani circuit for a caller-provided hidden bit string.
     *
     * Input qubits occupy q0 through q(inputQubitCount - 1), with the oracle
     * ancilla stored in the final qubit. The most-significant hidden bit maps to q0.
     *
     * @param inputQubitCount Number of input qubits, excluding the oracle ancilla.
     * @param hiddenValue Hidden bit string encoded as an unsigned integer.
     * @return Circuit that recovers hiddenValue on the input register.
     * @throws std::invalid_argument if the input count is zero, too large, or hiddenValue does not fit.
     */
    [[nodiscard]] circuit::QuantumCircuit bernsteinVaziraniCircuit(
        std::size_t inputQubitCount,
        std::size_t hiddenValue
    );

    /**
     * Builds a Toffoli demonstration using H, T, inverse-T, and CX gates.
     *
     * The circuit prepares q0 and q1 as |1⟩, then toggles q2 through a standard
     * decomposition so execution from |000⟩ finishes in |111⟩.
     *
     * Additional register qubits remain in their initial state.
     *
     * @param qubitCount Total register size. Must be at least 3.
     * @return Decomposed Toffoli demonstration on q0, q1, and q2.
     * @throws std::invalid_argument if qubitCount is less than 3.
     */
    [[nodiscard]] circuit::QuantumCircuit toffoliDemoCircuit(
        std::size_t qubitCount = 3U
    );

    /**
     * Builds a two-qubit phase-kickback demonstration.
     *
     * The target is prepared in |-⟩ before a controlled-X oracle. A final
     * Hadamard pass exposes the kicked-back phase as the basis state |11⟩.
     *
     * Additional register qubits remain in their initial state.
     *
     * @param qubitCount Total register size. Must be at least 2.
     * @return Phase-kickback circuit on q0 and q1.
     * @throws std::invalid_argument if qubitCount is less than 2.
     */
    [[nodiscard]] circuit::QuantumCircuit phaseKickbackCircuit(
        std::size_t qubitCount = 2U
    );

    /**
     * Builds a coherent three-qubit teleportation demonstration.
     *
     * q0 is prepared with Ry(pi/3) and Rz(pi/5), q1/q2 form the Bell pair,
     * and controlled corrections replace measurement feed-forward so every
     * debugger step remains unitary. The prepared state finishes on q2.
     *
     * Additional register qubits remain in their initial state.
     *
     * @param qubitCount Total register size. Must be at least 3.
     * @return Coherent teleportation circuit on q0, q1, and q2.
     * @throws std::invalid_argument if qubitCount is less than 3.
     */
    [[nodiscard]] circuit::QuantumCircuit teleportationCircuit(
        std::size_t qubitCount = 3U
    );

    /**
     * Builds a deterministic mixed-gate circuit for visualization stress testing.
     *
     * @param qubitCount Number of qubits to scramble.
     * @return Circuit containing superposition, local phase gates, entanglement, and an optional swap.
     * @throws std::invalid_argument if qubitCount is zero.
     */
    [[nodiscard]] circuit::QuantumCircuit scrambleCircuit(std::size_t qubitCount);

    /**
     * Builds a one-qubit Rx rotation demo.
     *
     * @param angleRadians Rotation angle in radians.
     * @return Circuit with one Rx instruction.
     */
    [[nodiscard]] circuit::QuantumCircuit rxRotationCircuit(double angleRadians);

    /**
     * Builds a one-qubit Ry rotation demo.
     *
     * @param angleRadians Rotation angle in radians.
     * @return Circuit with one Ry instruction.
     */
    [[nodiscard]] circuit::QuantumCircuit ryRotationCircuit(double angleRadians);

    /**
     * Builds a one-qubit Rz rotation demo.
     *
     * @param angleRadians Rotation angle in radians.
     * @return Circuit with one Rz instruction.
     */
    [[nodiscard]] circuit::QuantumCircuit rzRotationCircuit(double angleRadians);
}

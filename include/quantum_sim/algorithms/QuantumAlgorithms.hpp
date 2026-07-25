#pragma once

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include <cstddef>

namespace quantum_sim::algorithms {
    /**
     * @return Two-qubit Bell-state demo circuit: H(q0), CX(q0 -> q1).
     */
    [[nodiscard]] circuit::QuantumCircuit bellStateCircuit();

    /**
     * Builds a register-wide equal superposition circuit.
     *
     * @param qubitCount Number of qubits to place into superposition.
     * @return Circuit containing one Hadamard gate per qubit.
     */
    [[nodiscard]] circuit::QuantumCircuit equalSuperpositionCircuit(std::size_t qubitCount);

    /**
     * @return Three-qubit GHZ circuit: H(q0), CX(q0 -> q1), CX(q1 -> q2).
     */
    [[nodiscard]] circuit::QuantumCircuit ghzStateCircuit();

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

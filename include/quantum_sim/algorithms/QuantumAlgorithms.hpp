#pragma once

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include <cstddef>

namespace quantum_sim::algorithms {
    /**
     * Constructs a quantum circuit to generate a Bell state.
     *
     * The function creates a two-qubit quantum circuit and applies the following gates:
     * - A Hadamard gate on the first qubit to create superposition.
     * - A CX gate with the first qubit as the control and the second qubit as the target.
     *
     * @return A quantum circuit representing the Bell state generation process.
     */
    [[nodiscard]] circuit::QuantumCircuit bellStateCircuit();

    /**
     * Constructs a quantum circuit that initializes all qubits into an equal superposition state
     * by applying the Hadamard gate to each qubit in the circuit.
     *
     * @param qubitCount The number of qubits in the circuit.
     * @return A QuantumCircuit object with all qubits in an equal superposition state.
     */
    [[nodiscard]] circuit::QuantumCircuit equalSuperpositionCircuit(std::size_t qubitCount);

    /**
     * Constructs a quantum circuit to generate a GHZ state.
     *
     * The function initializes a three-qubit quantum circuit and applies the following gates:
     * - A Hadamard gate on the first qubit to create superposition.
     * - A CX gate with the first qubit as the control and the second qubit as the target.
     * - Another CX gate with the second qubit as the control and the third qubit as the target.
     *
     * @return A quantum circuit configured to produce a GHZ state.
     */
    [[nodiscard]] circuit::QuantumCircuit ghzStateCircuit();

    /**
     * Creates a quantum circuit to perform an Rx rotation on a single qubit.
     *
     * This function initializes a single-qubit quantum circuit and applies
     * an Rx rotation gate with the specified rotation angle (in radians)
     * to the qubit.
     *
     * @param angleRadians The rotation angle in radians for the Rx gate.
     * @return A quantum circuit containing a single qubit with the specified Rx rotation gate applied.
     */
    [[nodiscard]] circuit::QuantumCircuit rxRotationCircuit(double angleRadians);

    /**
     * Constructs a quantum circuit to apply a single-qubit Ry rotation gate.
     *
     * This function creates a one-qubit quantum circuit and applies an Ry gate
     * to the qubit, which rotates it around the Y-axis of the Bloch sphere
     * by the specified angle in radians.
     *
     * @param angleRadians The angle, in radians, by which the qubit is rotated
     *                     around the Y-axis.
     * @return A quantum circuit with the Ry rotation gate applied.
     */
    [[nodiscard]] circuit::QuantumCircuit ryRotationCircuit(double angleRadians);

    /**
     * Constructs a single-qubit quantum circuit with an Rz rotation gate.
     *
     * This function creates a quantum circuit with one qubit and applies an Rz
     * gate to that qubit. The Rz gate rotates the qubit's state around the Z-axis
     * of the Bloch sphere by the specified angle in radians.
     *
     * @param angleRadians The angle of rotation in radians to apply with the Rz gate.
     * @return A quantum circuit with the specified Rz rotation applied to the single qubit.
     */
    [[nodiscard]] circuit::QuantumCircuit rzRotationCircuit(double angleRadians);
}

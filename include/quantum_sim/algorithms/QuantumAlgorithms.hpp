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

    [[nodiscard]] circuit::QuantumCircuit rxRotationCircuit(double angleRadians);
}

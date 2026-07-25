#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"

#include "quantum_sim/gates/QuantumGates.hpp"

namespace quantum_sim::algorithms {
    circuit::QuantumCircuit bellStateCircuit() {
        circuit::QuantumCircuit circuit{2};

        // H creates superposition; CX entangles q1 with q0.
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0);
        circuit.addControlledGate("CX", gates::cxGate(), 0, 1);

        return circuit;
    }

    circuit::QuantumCircuit equalSuperpositionCircuit(std::size_t qubitCount) {
        circuit::QuantumCircuit circuit{qubitCount};

        // Applying H to every qubit creates a uniform distribution over 2^n states.
        for (std::size_t qubit = 0; qubit < qubitCount; ++qubit) {
            circuit.addSingleQubitGate("H", gates::hadamardGate(), qubit);
        }

        return circuit;
    }

    circuit::QuantumCircuit ghzStateCircuit() {
        circuit::QuantumCircuit circuit{3};

        // GHZ chains two CX gates from a superposed first qubit.
        circuit.addSingleQubitGate(
            "H",
            gates::hadamardGate(),
            0
        );

        circuit.addControlledGate(
            "CX",
            gates::cxGate(3, 0, 1),
            0,
            1
        );

        circuit.addControlledGate(
            "CX",
            gates::cxGate(3, 1, 2),
            1,
            2
        );

        return circuit;
    }

    circuit::QuantumCircuit rxRotationCircuit(double angleRadians) {
        circuit::QuantumCircuit circuit{1};

        circuit.addSingleQubitGate("Rx", gates::rxGate(angleRadians), 0);

        return circuit;
    }

    circuit::QuantumCircuit ryRotationCircuit(double angleRadians) {
        circuit::QuantumCircuit circuit{1};

        circuit.addSingleQubitGate("Ry", gates::ryGate(angleRadians), 0);

        return circuit;
    }

    circuit::QuantumCircuit rzRotationCircuit(double angleRadians) {
        circuit::QuantumCircuit circuit{1};

        circuit.addSingleQubitGate("Rz", gates::rzGate(angleRadians), 0);

        return circuit;
    }
}

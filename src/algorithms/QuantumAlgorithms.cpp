#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"

#include "quantum_sim/gates/QuantumGates.hpp"

namespace quantum_sim::algorithms {
    circuit::QuantumCircuit bellStateCircuit() {
        circuit::QuantumCircuit circuit{2};

        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0);
        circuit.addControlledGate("CNOT", gates::cnotGate(), 0, 1);

        return circuit;
    }

    circuit::QuantumCircuit equalSuperpositionCircuit(std::size_t qubitCount) {
        circuit::QuantumCircuit circuit{qubitCount};

        for (std::size_t qubit = 0; qubit < qubitCount; ++qubit) {
            circuit.addSingleQubitGate("H", gates::hadamardGate(), qubit);
        }

        return circuit;
    }

    circuit::QuantumCircuit ghzStateCircuit() {
        circuit::QuantumCircuit circuit{3};

        circuit.addSingleQubitGate(
            "H",
            gates::hadamardGate(),
            0
        );

        circuit.addControlledGate(
            "CNOT",
            gates::cnotGate(3,0,1),
            0,
            1
        );

        circuit.addControlledGate(
            "CNOT",
            gates::cnotGate(3,1,2),
            1,
            2
        );

        return circuit;
    }
}

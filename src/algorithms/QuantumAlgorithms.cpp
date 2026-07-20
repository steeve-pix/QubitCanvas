#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"

#include "quantum_sim/gates/QuantumGates.hpp"

namespace quantum_sim::algorithms {
    circuit::QuantumCircuit bellStateCircuit() {
        circuit::QuantumCircuit circuit{2};

        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0);
        circuit.addControlledGate("CNOT", gates::cnotGate(), 0, 1);

        return circuit;
    }
}

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/gates/QuantumGates.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

#include <cstddef>
#include <iostream>
#include <random>

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/visualization/ConsoleVisualizer.hpp"

int main() {
    using quantum_sim::circuit::QuantumCircuit;
    using quantum_sim::quantum::QuantumRegister;

    QuantumCircuit bellCircuit{2};

    bellCircuit.addSingleQubitGate(quantum_sim::gates::hadamardGate(), 0);
    bellCircuit.addFullRegisterGate(quantum_sim::gates::cnotGate());

    const QuantumRegister initialState = QuantumRegister::basisState(2, 0);

    std::random_device seedSource{};
    std::mt19937 randomEngine{seedSource()};

    constexpr std::size_t shotCount = 1'000;

    const QuantumRegister bellState = bellCircuit.execute(initialState);

    std::cout << "Theoretical probabilities:\n";

    quantum_sim::visualization::printProbabilityBars(
        bellState,
        std::cout
    );

    const std::vector<std::size_t> counts =
            bellCircuit.runShots(initialState, shotCount, randomEngine);

    return 0;
}

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

    bellCircuit.addSingleQubitGate("H", quantum_sim::gates::hadamardGate(), 0);
    bellCircuit.addControlledGate("CNOT", quantum_sim::gates::cnotGate(), 0, 1);

    const QuantumRegister initialState = QuantumRegister::basisState(2, 0);

    std::random_device seedSource{};
    std::mt19937 randomEngine{seedSource()};

    constexpr std::size_t shotCount = 1'000;

    const QuantumRegister bellState = bellCircuit.execute(initialState);

    std::cout << "Theoretical probabilities:\n";

    quantum_sim::visualization::printProbabilityBars(bellState, std::cout);

    const std::vector<std::size_t> counts =
            bellCircuit.runShots(initialState, shotCount, randomEngine);

    std::cout << "\nObserved shot results:\n";

    quantum_sim::visualization::printShotBars(bellState, counts, std::cout);

    std::cout << "----------------------------------------------------\n";

    const std::vector<quantum_sim::circuit::TraceStep> trace =
            bellCircuit.executeWithTrace(initialState);

    std::cout << "Circuit execution trace:\n\n";
    quantum_sim::visualization::printExecutionTrace(initialState, trace, std::cout);

    std::cout << "----------------------------------------------------\n";
    std::cout << "Circuit diagram:\n";

    quantum_sim::visualization::printCircuitDiagram(bellCircuit, std::cout);

    std::cout << '\n';

    return 0;
}

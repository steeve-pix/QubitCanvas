#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/gates/QuantumGates.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

#include <cstddef>
#include <iostream>
#include <random>

#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/visualization/ConsoleVisualizer.hpp"

int main() {
    using quantum_sim::circuit::QuantumCircuit;
    using quantum_sim::quantum::QuantumRegister;

    const QuantumCircuit result = quantum_sim::algorithms::equalSuperpositionCircuit(3);

    const QuantumRegister initialState = QuantumRegister::basisState(3, 0);

    std::random_device seedSource{};
    std::mt19937 randomEngine{seedSource()};

    constexpr std::size_t shotCount = 1'000;

    const QuantumRegister superpositionState = result.execute(initialState);

    std::cout << "Theoretical probabilities:\n";

    quantum_sim::visualization::printProbabilityBars(superpositionState, std::cout);

    const std::vector<std::size_t> counts =
            result.runShots(initialState, shotCount, randomEngine);

    std::cout << "\nObserved shot results:\n";

    quantum_sim::visualization::printShotBars(superpositionState, counts, std::cout);

    std::cout << "----------------------------------------------------\n";

    const std::vector<quantum_sim::circuit::TraceStep> trace =
            result.executeWithTrace(initialState);

    std::cout << "Circuit execution trace:\n\n";
    quantum_sim::visualization::printExecutionTrace(initialState, trace, std::cout);

    std::cout << "----------------------------------------------------\n";
    std::cout << "Circuit diagram:\n";

    quantum_sim::visualization::printCircuitDiagram(result, std::cout);

    std::cout << '\n';

    return 0;
}

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/gates/QuantumGates.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

#include <cstddef>
#include <iostream>
#include <random>

#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"
#include "quantum_sim/visualization/ConsoleVisualizer.hpp"

int main() {
    using quantum_sim::circuit::QuantumCircuit;
    using quantum_sim::quantum::QuantumRegister;

    const QuantumCircuit bellCircuit = quantum_sim::algorithms::bellStateCircuit();

    const QuantumRegister initialState = QuantumRegister::basisState(2, 0);

    std::random_device seedSource{};
    std::mt19937 randomEngine{seedSource()};

    constexpr std::size_t shotCount = 1'000;

    const QuantumRegister bellState = bellCircuit.execute(initialState);

    const std::vector<quantum_sim::circuit::TraceStep> trace =
            bellCircuit.executeWithTrace(initialState);

    std::cout << "Circuit execution trace:\n\n";
    quantum_sim::debug::runInteractiveDebugger(bellCircuit, initialState);

    std::cout << "\n";
    std::cout << "----------------------------------------------------\n";
    // std::cout << "Circuit diagram:\n";
    quantum_sim::debug::printCircuitDiagram(bellCircuit, std::cout);

    std::cout << '\n';

    return 0;
}

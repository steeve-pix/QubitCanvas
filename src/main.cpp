#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"

#include <cstddef>
#include <iostream>
#include <random>
#include <stdexcept>

int readAlgorithmChoice();

int main() {
    using quantum_sim::circuit::QuantumCircuit;
    using quantum_sim::quantum::QuantumRegister;

    const int choice = readAlgorithmChoice();

    if (choice < 1 || choice > 3) {
        std::cerr << "Invalid choice. Please enter 1, 2 or 3.\n";
        return 1;
    }

    const QuantumCircuit circuit = [&]() {
        switch (choice) {
            case 1:
                return quantum_sim::algorithms::bellStateCircuit();
            case 2:
                return quantum_sim::algorithms::equalSuperpositionCircuit(3);
            case 3:
                return quantum_sim::algorithms::ghzStateCircuit();
            default:
                throw std::invalid_argument{"Unsupported algorithm choice."};
        }
    }();


    const QuantumRegister initialState =
            QuantumRegister::basisState(circuit.qubitCount(), 0);

    std::random_device seedSource{};

    const QuantumRegister bellState = circuit.execute(initialState);

    const std::vector<quantum_sim::circuit::TraceStep> trace =
            circuit.executeWithTrace(initialState);

    std::cout << "Circuit execution trace:\n\n";
    quantum_sim::debug::runInteractiveDebugger(circuit, initialState);

    return 0;
}


int readAlgorithmChoice() {
    std::cout
            << "Choose a quantum demonstration:\n"
            << "1. Bell state\n"
            << "2. Equal superposition\n"
            << "3. GHZ state\n"
            << "Choice: ";

    int choice{};
    std::cin >> choice;

    return choice;
}

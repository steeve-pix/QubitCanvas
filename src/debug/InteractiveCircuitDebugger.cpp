#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"
#include "quantum_sim/visualization/ConsoleVisualizer.hpp"

#include <iostream>
#include <limits>

namespace quantum_sim::debug {
    void runInteractiveDebugger(const circuit::QuantumCircuit &circuit, quantum::QuantumRegister &initialState) {
        const auto trace = circuit.executeWithTrace(initialState);

        std::cout << "Initial state:\n";
        visualization::printProbabilityBars(initialState, std::cout);

        for (const auto &step: trace) {
            std::cout << "\n";
            std::cout << step.description << "\n";

            visualization::printProbabilityBars(step.state, std::cout);

            std::cout << "\nPress Enter to continue...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        std::cout << "\nCircuit execution complete.\n";
    }
}

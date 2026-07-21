#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"
#include "quantum_sim/visualization/ConsoleVisualizer.hpp"

#include <iostream>
#include <limits>
#include <cctype>

namespace quantum_sim::debug {
    char readDebuggerCommand() {
        std::cout << "\n[n] Next, [p] Previous, [r] Restart, " << "[h] Help, [q] Quit: ";

        char command{};
        std::cin >> command;

        command = static_cast<char>(
            std::tolower(static_cast<unsigned char>(command))
        );

        return command;
    };

    void runInteractiveDebugger(const circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState) {
        const auto trace = circuit.executeWithTrace(initialState);

        std::cout << "Initial state:\n";
        visualization::printProbabilityBars(initialState, std::cout);

        std::size_t currentStep{};
        while (currentStep < trace.size()) {
            const auto &step = trace[currentStep];

            std::cout << "\nCircuit:\n";
            visualization::printCircuitDiagram(circuit, std::cout, currentStep);


            std::cout << "\n========== Step " << (currentStep + 1) << " / " << trace.size() << " ==========\n";
            std::cout << step.description << "\n";

            visualization::printProbabilityBars(step.state, std::cout);

            char command = readDebuggerCommand();

            if (command == 'q') {
                std::cout << "\nDebugger closed.\n";
                return;
            }
            if (command == 'n') {
                ++currentStep;
            } else if (command == 'p') {
                if (currentStep > 0) {
                    --currentStep;
                } else {
                    std::cout << "\nAlready at the first step.\n";
                }
            } else if (command == 'r') {
                currentStep = 0;
            } else if (command == 'h') {
                std::cout
                        << "\nDebugger commands:\n"
                        << "  n - Move to the next instruction\n"
                        << "  p - Move to the previous instruction\n"
                        << "  r - Restart from the first instruction\n"
                        << "  h - Show this help menu\n"
                        << "  q - Quit the debugger\n";
            } else {
                std::cout << "\nUnknown command. Type 'h' to view available commands.\n";
            }
        }
        std::cout << "\nCircuit execution complete.\n";
    }
}

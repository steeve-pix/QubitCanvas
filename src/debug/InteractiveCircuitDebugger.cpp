#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"
#include "quantum_sim/visualization/ConsoleVisualizer.hpp"

#include <iostream>
#include <limits>
#include <cctype>
#include <chrono>
#include <thread>
#include <string>

namespace quantum_sim::debug {
    char readDebuggerCommand() {
        std::cout
                << "\n[n] Next, [p] Previous, [r] Restart, "
                << "[a] Auto-play, [i] Inspect amplitudes, "
                << "[c] Compare changes, [h] Help, [q] Quit: ";

        char command{};
        std::cin >> command;

        command = static_cast<char>(
            std::tolower(static_cast<unsigned char>(command))
        );

        return command;
    };

    void waitForAutoPlay() {
        std::this_thread::sleep_for(std::chrono::milliseconds{750});
    }

    std::string gateExplanation(const std::string &gateName) {
        if (gateName == "H") {
            return
                    "Create a superposition by mixing the |0> "
                    "and |1> amplitudes.";
        }

        if (gateName == "CNOT") {
            return
                    "Flips the target qubit when the control qubit is |1>. "
                    "It can create entanglement.";
        }
        if (gateName == "X") {
            return
                    "Flips |0> to |1> and |1> to |0>. "
                    "It is the quantum equivalent of a classical NOT gate.";
        }
        if (gateName == "Y") {
            return
                    "Flips the qubit like X, while also changing its phase. "
                    "It maps |0> to i|1> and |1> to -i|0>.";
        }
        if (gateName == "Z") {
            return
                    "Leaves |0> unchanged and multiplies the |1> amplitude by -1. "
                    "This changes phase without changing measurement probabilities.";
        }
        if (gateName == "S") {
            return
                    "Leaves |0> unchanged and multiplies the |1> amplitude by i. "
                    "This adds a phase of pi/2 radians.";
        }
        if (gateName == "T") {
            return
                    "Leaves |0> unchanged and adds a phase of pi/4 radians "
                    "to the |1> amplitude.";
        }
        if (gateName == "SWAP") {
            return
                    "Exchanges the quantum states of two qubits. "
                    "The first qubit receives the second qubit's state, and vice versa.";
        }
        
        return "No explanation is available for this gate yet.";
    }

    void runInteractiveDebugger(const circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState) {
        const auto trace = circuit.executeWithTrace(initialState);
        const auto instructions = circuit.instructionInfo();

        std::cout << "Initial state:\n";
        visualization::printProbabilityBars(initialState, std::cout);

        std::size_t currentStep{};
        bool autoPlay = false;
        while (currentStep < trace.size()) {
            const auto &step = trace[currentStep];
            const auto &instruction = instructions[currentStep];

            std::cout << "\nCircuit:\n";
            visualization::printCircuitDiagram(circuit, std::cout, currentStep);


            std::cout << "\n========== Step " << (currentStep + 1) << " / " << trace.size() << " ==========\n";
            std::cout << step.description << "\n";
            std::cout
                    << "Explanation: "
                    << gateExplanation(instruction.name)
                    << '\n';

            visualization::printProbabilityBars(step.state, std::cout);
            if (autoPlay) {
                waitForAutoPlay();
                ++currentStep;
                continue;
            }

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
            } else if (command == 'a') {
                autoPlay = true;
                ++currentStep;
            } else if (command == 'i') {
                std::cout << "\nCurrent amplitudes:\n";

                visualization::printAmplitudes(
                    step.state,
                    std::cout
                );
            } else if (command == 'c') {
                const quantum::QuantumRegister &beforeState = currentStep == 0
                                                                  ? initialState
                                                                  : trace[currentStep - 1].state;
                const quantum::QuantumRegister &afterState = step.state;
                std::cout
                        << "\nChanges caused by "
                        << step.description
                        << ":\n";

                visualization::printStateComparison(beforeState, afterState, std::cout);
            } else if (command == 'h') {
                std::cout
                        << "\nDebugger commands:\n"
                        << "  a - Automatically execute remaining instructions\n"
                        << "  n - Move to the next instruction\n"
                        << "  p - Move to the previous instruction\n"
                        << "  r - Restart from the first instruction\n"
                        << "  i - Inspect the current state's amplitudes\n"
                        << "  c - Compare the state before and after the current instruction\n"
                        << "  h - Show this help menu\n"
                        << "  q - Quit the debugger\n";
            } else {
                std::cout << "\nUnknown command. Type 'h' to view available commands.\n";
            }
        }
        std::cout << "\nCircuit execution complete.\n";
    }
}

#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"
#include "quantum_sim/visualization/ConsoleVisualizer.hpp"

#include <iostream>
#include <limits>
#include <cctype>
#include <chrono>
#include <thread>
#include <string>

#include "quantum_sim/debug/DebuggerSession.hpp"

namespace {
    char readDebuggerCommand() {
        std::cout
                << "\n[n] Next, [p] Previous, [r] Restart, "
                << "[a] Auto-play, [i] Inspect amplitudes, "
                << "[c] Compare changes, [b] Bloch vector, [h] Help, [q] Quit: ";

        char command{};
        std::cin >> command;

        return static_cast<char>(std::tolower(static_cast<unsigned char>(command)));
    };

    void waitForAutoPlay() {
        std::this_thread::sleep_for(std::chrono::milliseconds{750});
    }

    void printDebuggerHelp() {
        std::cout
                << "\nDebugger commands:\n"
                << "  n - Move to the next instruction\n"
                << "  p - Move to the previous instruction\n"
                << "  r - Restart from the first instruction\n"
                << "  a - Automatically execute remaining instructions\n"
                << "  i - Inspect the current state's amplitudes\n"
                << "  c - Compare the state before and after the current instruction\n"
                << "  b - Display the current state's Bloch vector\n"
                << "  h - Show this help menu\n"
                << "  q - Quit the debugger\n";
    }
}

namespace quantum_sim::debug {
    std::string gateExplanation(const std::string &gateName) {
        if (gateName == "H") {
            return
                    "Create a superposition by mixing the |0> "
                    "and |1> amplitudes.";
        }

        if (gateName == "CX") {
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
        if (gateName == "Rx") {
            return
                    "Rotates the qubit around the X axis of the Bloch sphere. "
                    "The rotation angle is measured in radians.";
        }
        if (gateName == "Ry") {
            return
                    "Rotates the qubit around the Y axis of the Bloch sphere. "
                    "The rotation angle is measured in radians.";
        }
        if (gateName == "Rz") {
            return
                    "Rotates the qubit around the Z axis of the Bloch sphere. "
                    "This changes phase while preserving basis-state probabilities.";
        }
        return "No explanation is available for this gate yet.";
    }

    void runInteractiveDebugger(const circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState) {
        DebuggerSession session{circuit, initialState};

        std::cout << "Initial state:\n";
        visualization::printProbabilityBars(initialState, std::cout);

        if (session.stepCount() == 0) {
            std::cout << "\nThe circuit contains no instructions.\n";
            return;
        }
        bool autoPlay = false;
        while (true) {
            const circuit::TraceStep &step =
                    session.currentStep();

            const std::size_t currentStep =
                    session.currentStepIndex();

            const circuit::CircuitInstructionInfo &instruction =
                    session.currentInstruction();


            std::cout << "\nCircuit:\n";
            visualization::printCircuitDiagram(circuit, std::cout, currentStep);

            std::cout << "\n========== Step " << (currentStep + 1) << " / " << session.stepCount() << " ==========\n";
            std::cout << step.description << "\n";
            std::cout
                    << "Explanation: "
                    << gateExplanation(instruction.name)
                    << '\n';

            visualization::printProbabilityBars(step.state, std::cout);
            if (autoPlay) {
                waitForAutoPlay();
                if (!session.moveNext()) {
                    break;
                }

                continue;
            }

            const char command =
                    readDebuggerCommand();

            if (command == 'q') {
                std::cout << "\nDebugger closed.\n";
                return;
            }
            if (command == 'n') {
                if (!session.moveNext()) {
                    break;
                }
            } else if (command == 'p') {
                if (!session.movePrevious()) {
                    std::cout
                            << "\nAlready at the first step.\n";
                }
            } else if (command == 'r') {
                session.restart();
            } else if (command == 'a') {
                autoPlay = true;
                if (!session.moveNext()) {
                    break;
                }
            } else if (command == 'i') {
                std::cout << "\nCurrent amplitudes:\n";

                visualization::printAmplitudes(
                    step.state,
                    std::cout
                );
            } else if (command == 'c') {
                const quantum::QuantumRegister &beforeState =
                        session.stateBeforeCurrentStep();

                const quantum::QuantumRegister &afterState =
                        session.currentStep().state;

                std::cout
                        << "\nChanges caused by "
                        << session.currentStep().description
                        << ":\n";

                visualization::printStateComparison(beforeState, afterState, std::cout);
            } else if (command == 'b') {
                const quantum::QuantumRegister &currentState =
                        session.currentStep().state;

                if (currentState.qubitCount() != 1) {
                    std::cout
                            << "\nA Bloch vector can only represent "
                            << "a single-qubit state.\n";
                } else {
                    std::cout << '\n';

                    visualization::printBlochVector(currentState, std::cout);

                    std::cout << '\n';

                    visualization::printAsciiBlochSphere(currentState, std::cout);
                }
            } else if (command == 'h') {
                printDebuggerHelp();
            } else {
                std::cout << "\nUnknown command. Type 'h' to view available commands.\n";
            }
        }
        std::cout << "\nCircuit execution complete.\n";
    }
}

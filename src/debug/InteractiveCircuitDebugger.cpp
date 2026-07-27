#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"
#include "quantum_sim/visualization/ConsoleVisualizer.hpp"

#include <iostream>
#include <limits>
#include <cctype>
#include <chrono>
#include <optional>
#include <thread>
#include <string>

#include "quantum_sim/debug/DebuggerSession.hpp"

namespace {
    char readDebuggerCommand() {
        // Keep command parsing tiny: one lower-cased character drives the loop.
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
        // The GUI and console debugger share these short explanations so gate
        // selection and step inspection speak with the same vocabulary.
        if (gateName == "H") {
            return
                    "Create a superposition by mixing the |0\xE2\x9F\xA9 "
                    "and |1\xE2\x9F\xA9 amplitudes.";
        }

        if (gateName == "CX") {
            return
                    "Flips the target qubit when the control qubit is |1\xE2\x9F\xA9. "
                    "It can create entanglement.";
        }
        if (gateName == "X") {
            return
                    "Flips |0\xE2\x9F\xA9 to |1\xE2\x9F\xA9 and "
                    "|1\xE2\x9F\xA9 to |0\xE2\x9F\xA9. "
                    "It is the quantum equivalent of a classical NOT gate.";
        }
        if (gateName == "Y") {
            return
                    "Flips the qubit like X, while also changing its phase. "
                    "It maps |0\xE2\x9F\xA9 to i|1\xE2\x9F\xA9 and "
                    "|1\xE2\x9F\xA9 to -i|0\xE2\x9F\xA9.";
        }
        if (gateName == "Z") {
            return
                    "Leaves |0\xE2\x9F\xA9 unchanged and multiplies the "
                    "|1\xE2\x9F\xA9 amplitude by -1. "
                    "This changes phase without changing measurement probabilities.";
        }
        if (gateName == "S") {
            return
                    "Leaves |0\xE2\x9F\xA9 unchanged and multiplies the "
                    "|1\xE2\x9F\xA9 amplitude by i. "
                    "This adds a phase of pi/2 radians.";
        }
        if (gateName == "Sdg") {
            return
                    "Applies the inverse S phase: |0\xE2\x9F\xA9 stays unchanged and "
                    "|1\xE2\x9F\xA9 "
                    "receives a phase of -pi/2 radians.";
        }
        if (gateName == "T") {
            return
                    "Leaves |0\xE2\x9F\xA9 unchanged and adds a phase of pi/4 radians "
                    "to the |1\xE2\x9F\xA9 amplitude.";
        }
        if (gateName == "Tdg") {
            return
                    "Applies the inverse T phase: |0\xE2\x9F\xA9 stays unchanged and "
                    "|1\xE2\x9F\xA9 "
                    "receives a phase of -pi/4 radians.";
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

        // Always show the starting probabilities before stepping through gates.
        std::cout << "Initial state:\n";
        visualization::printProbabilityBars(initialState, std::cout);

        if (session.stepCount() == 0) {
            std::cout << "\nThe circuit contains no instructions.\n";
            return;
        }
        bool autoPlay = false;
        while (true) {
            // Snapshot captures all state needed to draw this console frame.
            const DebuggerSnapshot snapshot =
                    session.snapshot();

            const quantum::QuantumRegister &currentState =
                    snapshot.afterState.get();

            const std::size_t currentStepNumber =
                    snapshot.currentStepNumber;

            const std::optional<std::size_t> currentInstructionIndex =
                    currentStepNumber == 0U
                        ? std::nullopt
                        : std::optional<std::size_t>{
                            currentStepNumber - 1U
                        };


            std::cout << "\nCircuit:\n";
            visualization::printCircuitDiagram(
                circuit,
                std::cout,
                currentInstructionIndex
            );

            std::cout
                    << "\n========== Step "
                    << currentStepNumber
                    << " / "
                    << snapshot.stepCount
                    << " ==========\n";

            if (snapshot.instruction.has_value()) {
                const circuit::CircuitInstructionInfo &instruction =
                        snapshot.instruction->get();

                std::cout << instruction.name << "\n";
                std::cout
                        << "Explanation: "
                        << gateExplanation(instruction.name)
                        << '\n';
            } else {
                std::cout << "I on all qubits\n";
                std::cout
                        << "Explanation: Initial register before any "
                           "circuit instruction executes.\n";
            }

            visualization::printProbabilityBars(currentState, std::cout);
            if (autoPlay) {
                // Auto-play advances at a fixed delay until the final step.
                waitForAutoPlay();
                if (!session.moveNext()) {
                    break;
                }

                continue;
            }

            const char command =
                    readDebuggerCommand();

            // Manual commands mutate session navigation or print extra state details.
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
                            << "\nAlready at initial step zero.\n";
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
                    currentState,
                    std::cout
                );
            } else if (command == 'c') {
                const quantum::QuantumRegister &beforeState =
                        snapshot.beforeState.get();

                const quantum::QuantumRegister &afterState =
                        currentState;

                std::cout
                        << "\nChanges caused by "
                        << (
                            snapshot.instruction.has_value()
                                ? snapshot.instruction->get().name
                                : "initial identity step"
                        )
                        << ":\n";

                visualization::printStateComparison(beforeState, afterState, std::cout);
            } else if (command == 'b') {
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

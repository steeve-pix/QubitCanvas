#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"
#include "quantum_sim/gui/GuiApplication.hpp"

#include <cstddef>
#include <iostream>
#include <numbers>
#include <random>
#include <stdexcept>
#include <cctype>

int readAlgorithmChoice();

char readRotationAxis();

double readRotationAngle();

int readRotationInitialState();

quantum_sim::quantum::QuantumRegister createRotationInitialState(const int choice) {
    const double amplitude =
            1.0 / std::sqrt(2.0);

    switch (choice) {
        case 1: return quantum_sim::quantum::QuantumRegister::basisState(1, 0);
        case 2: return quantum_sim::quantum::QuantumRegister::basisState(1, 1);
        case 3:
            return quantum_sim::quantum::QuantumRegister{
                1,
                quantum_sim::math::ComplexVector{
                    std::vector{
                        quantum_sim::math::Complex{amplitude, 0.0},
                        quantum_sim::math::Complex{amplitude, 0.0}
                    }
                }
            };

        case 4:
            return quantum_sim::quantum::QuantumRegister{
                1,
                quantum_sim::math::ComplexVector{
                    std::vector{
                        quantum_sim::math::Complex{amplitude, 0.0},
                        quantum_sim::math::Complex{-amplitude, 0.0}
                    }
                }
            };

        default:
            throw std::invalid_argument{
                "Unsupported rotation initial state."
            };
    }
}

int main() {
    using quantum_sim::circuit::QuantumCircuit;
    using quantum_sim::quantum::QuantumRegister;

    const int choice = readAlgorithmChoice();

    if (choice < 1 || choice > 4) {
        std::cerr << "Invalid choice. Please enter 1, 2, 3 or 4.\n";
        return 1;
    }

    char rotationAxis{};
    double rotationAngle{};
    int rotationInitialStateChoice{};
    if (choice == 4) {
        rotationAxis = readRotationAxis();
        if (rotationAxis != 'x' &&
            rotationAxis != 'y' &&
            rotationAxis != 'z') {
            std::cerr
                    << "Invalid rotation axis. Please enter x, y, or z.\n";

            return 1;
        }
        rotationAngle = readRotationAngle();
        rotationInitialStateChoice = readRotationInitialState();

        if (rotationInitialStateChoice < 1 ||
            rotationInitialStateChoice > 4) {
            std::cerr
                    << "Invalid initial state. Please enter 1, 2, 3, or 4.\n";

            return 1;
        }
    }

    QuantumCircuit circuit = [&]() {
        switch (choice) {
            case 1:
                return quantum_sim::algorithms::bellStateCircuit();
            case 2:
                return quantum_sim::algorithms::equalSuperpositionCircuit(3);
            case 3:
                return quantum_sim::algorithms::ghzStateCircuit();
            case 4:
                switch (rotationAxis) {
                    case 'x': return quantum_sim::algorithms::rxRotationCircuit(rotationAngle);
                    case 'y': return quantum_sim::algorithms::ryRotationCircuit(rotationAngle);
                    case 'z': return quantum_sim::algorithms::rzRotationCircuit(rotationAngle);
                    default: throw std::invalid_argument{"Unsupported rotation axis."};;
                }
            default:
                throw std::invalid_argument{"Unsupported algorithm choice."};
        }
    }();

    const QuantumRegister initialState =
            choice == 4
                ? createRotationInitialState(rotationInitialStateChoice)
                : QuantumRegister::basisState(circuit.qubitCount(), 0);

    quantum_sim::gui::GuiApplication application{circuit, initialState};
    application.run();

    return 0;
}


int readAlgorithmChoice() {
    std::cout
            << "Choose a quantum demonstration:\n"
            << "1. Bell state\n"
            << "2. Equal superposition\n"
            << "3. GHZ state\n"
            << "4. Rotation playground\n"
            << "Choice: ";

    int choice{};
    std::cin >> choice;

    return choice;
}

char readRotationAxis() {
    std::cout
            << "\nChoose a rotation axis:\n"
            << "x. Rx\n"
            << "y. Ry\n"
            << "z. Rz\n"
            << "Choice: ";

    char axis{};
    std::cin >> axis;

    return static_cast<char>(
        std::tolower(static_cast<unsigned char>(axis))
    );
}

double readRotationAngle() {
    std::cout << "Enter the rotation angle in radians: ";

    double angleRadians{};
    std::cin >> angleRadians;

    return angleRadians;
}

int readRotationInitialState() {
    std::cout
            << "\nChoose the initial qubit state:\n"
            << "1. |0>\n"
            << "2. |1>\n"
            << "3. |+>\n"
            << "4. |->\n"
            << "Choice: ";

    int choice{};
    std::cin >> choice;

    return choice;
}

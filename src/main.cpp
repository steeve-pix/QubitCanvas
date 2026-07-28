#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"
#include "quantum_sim/gates/QuantumGates.hpp"
#include "quantum_sim/gui/GuiApplication.hpp"

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <numbers>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <cctype>

namespace {
    struct CommandLineOptions {
        std::size_t qubitCount{4U};
        std::string preset{"qft"};
        quantum_sim::gui::GuiLaunchOptions launch;
    };

    [[nodiscard]] CommandLineOptions parseCommandLine(
        const int argumentCount,
        char *arguments[]
    ) {
        CommandLineOptions options;

        for (int index = 1; index < argumentCount; ++index) {
            const std::string_view argument =
                    arguments[index];

            const auto nextValue =
                    [&]() -> std::string {
                if (index + 1 >= argumentCount) {
                    throw std::invalid_argument{
                        "Missing value after " +
                        std::string{argument}
                    };
                }

                ++index;
                return arguments[index];
            };

            if (argument == "--qubits") {
                options.qubitCount =
                        static_cast<std::size_t>(
                            std::stoul(nextValue())
                        );
            } else if (argument == "--preset") {
                options.preset =
                        nextValue();
            } else if (argument == "--capture") {
                const std::filesystem::path capturePath =
                        std::filesystem::absolute(
                            nextValue()
                        );

                if (capturePath.has_parent_path()) {
                    std::filesystem::create_directories(
                        capturePath.parent_path()
                    );
                }

                options.launch.capturePath =
                        capturePath.string();

                options.launch.hiddenWindow = true;
            } else if (argument == "--algorithm-page") {
                const std::size_t page =
                        static_cast<std::size_t>(
                            std::stoul(nextValue())
                        );

                options.launch.algorithmPage =
                        page > 0U
                            ? page - 1U
                            : 0U;
            } else if (argument == "--gate-page") {
                const std::size_t page =
                        static_cast<std::size_t>(
                            std::stoul(nextValue())
                        );

                options.launch.gatePage =
                        page > 0U
                            ? page - 1U
                            : 0U;
            } else if (argument == "--gate") {
                options.launch.armedGate =
                        nextValue();
            } else if (argument == "--final-step") {
                options.launch.startAtFinalStep = true;
            } else if (argument == "--floor-field") {
                options.launch.startInFloorField = true;
            } else {
                throw std::invalid_argument{
                    "Unknown command-line option: " +
                    std::string{argument}
                };
            }
        }

        if (
            options.qubitCount == 0U ||
            options.qubitCount > 10U
        ) {
            throw std::invalid_argument{
                "The GUI supports between one and ten qubits."
            };
        }

        return options;
    }

    [[nodiscard]] quantum_sim::circuit::QuantumCircuit createPreset(
        const std::string_view preset,
        const std::size_t qubitCount
    ) {
        using namespace quantum_sim;

        if (preset == "qft") {
            return algorithms::qftCircuit(qubitCount);
        }
        if (preset == "grover") {
            return algorithms::groverSearchCircuit(qubitCount);
        }
        if (preset == "w") {
            return algorithms::wStateCircuit(qubitCount);
        }
        if (preset == "dicke") {
            return algorithms::dickeStateCircuit(qubitCount, 2U);
        }
        if (preset == "graph") {
            return algorithms::graphStateCircuit(qubitCount);
        }
        if (preset == "random") {
            return algorithms::randomCircuit(
                qubitCount,
                0x514255424954ULL +
                qubitCount
            );
        }
        if (preset == "weighted") {
            return algorithms::weightedStatePreparationCircuit(
                qubitCount
            );
        }
        if (preset == "bit-flip") {
            return algorithms::bitFlipCodeCircuit(qubitCount);
        }
        if (preset == "steane") {
            return algorithms::steaneCodeCircuit(qubitCount);
        }
        if (preset == "shor-code") {
            return algorithms::shorCodeCircuit(qubitCount);
        }
        if (preset == "phase-flip") {
            return algorithms::phaseFlipCodeCircuit(qubitCount);
        }
        if (preset == "five-qubit-code") {
            return algorithms::fiveQubitCodeCircuit(qubitCount);
        }
        if (preset == "quantum-counting") {
            return algorithms::quantumCountingCircuit(qubitCount);
        }
        if (preset == "amplitude-estimation") {
            return algorithms::amplitudeEstimationCircuit(qubitCount);
        }
        if (preset == "ripple-adder") {
            return algorithms::rippleCarryAdderCircuit(qubitCount);
        }
        if (preset == "draper-adder") {
            return algorithms::draperAdderCircuit(qubitCount);
        }
        if (preset == "iqp") {
            return algorithms::iqpCircuit(qubitCount);
        }
        if (preset == "surface-code") {
            return algorithms::surfaceCodeStabilizerCircuit(qubitCount);
        }
        if (preset == "swap-routing") {
            if (qubitCount < 6U) {
                throw std::invalid_argument{
                    "The swap-routing visual check requires six qubits."
                };
            }

            circuit::QuantumCircuit circuit{qubitCount};
            circuit.addTwoQubitGate(
                "SWAP",
                gates::swapGate(),
                1U,
                4U
            );
            circuit.addThreeQubitGate(
                "CSWAP",
                gates::cSwapGate(),
                0U,
                2U,
                5U
            );
            return circuit;
        }

        throw std::invalid_argument{
            "Unknown preset: " +
            std::string{preset}
        };
    }
}

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

int main(const int argumentCount, char *arguments[]) {
    using quantum_sim::circuit::QuantumCircuit;
    using quantum_sim::quantum::QuantumRegister;

    try {
        if (argumentCount > 0 && arguments[0] != nullptr) {
            const std::filesystem::path executablePath =
                    std::filesystem::absolute(arguments[0]);

            if (executablePath.has_parent_path()) {
                std::filesystem::current_path(
                    executablePath.parent_path()
                );
            }
        }

        const CommandLineOptions options =
                parseCommandLine(
                    argumentCount,
                    arguments
                );

        // Start the GUI with a visually interesting circuit instead of prompting
        // through the console. Users can switch presets inside the app.
        QuantumCircuit circuit =
                createPreset(
                    options.preset,
                    options.qubitCount
                );

        // Presets start from |000...⟩ so the trace is deterministic and easy to inspect.
        const QuantumRegister initialState =
                QuantumRegister::basisState(
                    circuit.qubitCount(),
                    0
                );

        quantum_sim::gui::GuiApplication application{
            circuit,
            initialState,
            options.launch
        };
        application.run();
    } catch (const std::exception &error) {
        std::cerr
                << "QubitCanvas failed to start: "
                << error.what()
                << '\n';

        return 1;
    }

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
            << "1. |0\xE2\x9F\xA9\n"
            << "2. |1\xE2\x9F\xA9\n"
            << "3. |+\xE2\x9F\xA9\n"
            << "4. |-\xE2\x9F\xA9\n"
            << "Choice: ";

    int choice{};
    std::cin >> choice;

    return choice;
}

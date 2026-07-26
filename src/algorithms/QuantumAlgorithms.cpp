#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"

#include "quantum_sim/gates/QuantumGates.hpp"

#include <limits>
#include <numbers>
#include <stdexcept>

namespace quantum_sim::algorithms {
    circuit::QuantumCircuit bellStateCircuit() {
        circuit::QuantumCircuit circuit{2};

        // H creates superposition; CX entangles q1 with q0.
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0);
        circuit.addControlledGate("CX", gates::cxGate(), 0, 1);

        return circuit;
    }

    circuit::QuantumCircuit equalSuperpositionCircuit(std::size_t qubitCount) {
        circuit::QuantumCircuit circuit{qubitCount};

        // Applying H to every qubit creates a uniform distribution over 2^n states.
        for (std::size_t qubit = 0; qubit < qubitCount; ++qubit) {
            circuit.addSingleQubitGate("H", gates::hadamardGate(), qubit);
        }

        return circuit;
    }

    circuit::QuantumCircuit qftCircuit(std::size_t qubitCount) {
        if (qubitCount >= std::numeric_limits<std::size_t>::digits) {
            throw std::invalid_argument{"QFT qubit count is too large."};
        }

        circuit::QuantumCircuit circuit{qubitCount};

        for (std::size_t target = 0; target < qubitCount; ++target) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                target
            );

            for (std::size_t control = target + 1U; control < qubitCount; ++control) {
                const double angleRadians =
                        std::numbers::pi /
                        static_cast<double>(std::size_t{1} << (control - target));

                const double halfAngleRadians =
                        angleRadians * 0.5;

                // Keep the controlled phase decomposed so the debugger exposes every layer.
                circuit.addSingleQubitGate(
                    "Rz",
                    gates::rzGate(halfAngleRadians),
                    control,
                    halfAngleRadians
                );

                circuit.addControlledGate(
                    "CX",
                    gates::cxGate(qubitCount, control, target),
                    control,
                    target
                );

                circuit.addSingleQubitGate(
                    "Rz",
                    gates::rzGate(-halfAngleRadians),
                    target,
                    -halfAngleRadians
                );

                circuit.addControlledGate(
                    "CX",
                    gates::cxGate(qubitCount, control, target),
                    control,
                    target
                );

                circuit.addSingleQubitGate(
                    "Rz",
                    gates::rzGate(halfAngleRadians),
                    target,
                    halfAngleRadians
                );
            }
        }

        for (std::size_t qubit = 0; qubit < qubitCount / 2U; ++qubit) {
            circuit.addControlledGate(
                "SWAP",
                gates::swapGate(qubitCount, qubit, qubitCount - 1U - qubit),
                qubit,
                qubitCount - 1U - qubit
            );
        }

        for (std::size_t qubit = 0; qubit < qubitCount; ++qubit) {
            const double angleRadians =
                    std::numbers::pi *
                    static_cast<double>(qubit + 1U) /
                    16.0;

            circuit.addSingleQubitGate(
                qubit % 2U == 0U ? "S" : "T",
                qubit % 2U == 0U ? gates::sGate() : gates::tGate(),
                qubit
            );

            circuit.addSingleQubitGate(
                "Rz",
                gates::rzGate(angleRadians),
                qubit,
                angleRadians
            );
        }

        return circuit;
    }

    circuit::QuantumCircuit inverseQftCircuit(std::size_t qubitCount) {
        if (qubitCount >= std::numeric_limits<std::size_t>::digits) {
            throw std::invalid_argument{"Inverse QFT qubit count is too large."};
        }

        circuit::QuantumCircuit circuit{qubitCount};

        // Undo the showcase's final phase pass in exact reverse instruction order.
        for (std::size_t qubit = qubitCount; qubit-- > 0U;) {
            const double angleRadians =
                    std::numbers::pi *
                    static_cast<double>(qubit + 1U) /
                    16.0;

            circuit.addSingleQubitGate(
                "Rz",
                gates::rzGate(-angleRadians),
                qubit,
                -angleRadians
            );

            circuit.addSingleQubitGate(
                qubit % 2U == 0U ? "Sdg" : "Tdg",
                qubit % 2U == 0U
                    ? gates::sDaggerGate()
                    : gates::tGate().conjugateTranspose(),
                qubit
            );
        }

        // SWAP is self-inverse, but the layer order must still be reversed.
        for (std::size_t swapIndex = qubitCount / 2U; swapIndex-- > 0U;) {
            circuit.addControlledGate(
                "SWAP",
                gates::swapGate(
                    qubitCount,
                    swapIndex,
                    qubitCount - 1U - swapIndex
                ),
                swapIndex,
                qubitCount - 1U - swapIndex
            );
        }

        for (std::size_t target = qubitCount; target-- > 0U;) {
            for (std::size_t control = qubitCount; control-- > target + 1U;) {
                const double angleRadians =
                        std::numbers::pi /
                        static_cast<double>(
                            std::size_t{1} << (control - target)
                        );

                const double halfAngleRadians =
                        angleRadians * 0.5;

                circuit.addSingleQubitGate(
                    "Rz",
                    gates::rzGate(-halfAngleRadians),
                    target,
                    -halfAngleRadians
                );

                circuit.addControlledGate(
                    "CX",
                    gates::cxGate(qubitCount, control, target),
                    control,
                    target
                );

                circuit.addSingleQubitGate(
                    "Rz",
                    gates::rzGate(halfAngleRadians),
                    target,
                    halfAngleRadians
                );

                circuit.addControlledGate(
                    "CX",
                    gates::cxGate(qubitCount, control, target),
                    control,
                    target
                );

                circuit.addSingleQubitGate(
                    "Rz",
                    gates::rzGate(-halfAngleRadians),
                    control,
                    -halfAngleRadians
                );
            }

            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                target
            );
        }

        return circuit;
    }

    circuit::QuantumCircuit ghzStateCircuit() {
        circuit::QuantumCircuit circuit{3};

        // GHZ chains two CX gates from a superposed first qubit.
        circuit.addSingleQubitGate(
            "H",
            gates::hadamardGate(),
            0
        );

        circuit.addControlledGate(
            "CX",
            gates::cxGate(3, 0, 1),
            0,
            1
        );

        circuit.addControlledGate(
            "CX",
            gates::cxGate(3, 1, 2),
            1,
            2
        );

        return circuit;
    }

    circuit::QuantumCircuit groverSearchCircuit() {
        circuit::QuantumCircuit circuit{2};

        // Uniform preparation followed by a CZ oracle that marks |11>.
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1);
        circuit.addControlledGate("CZ", gates::czGate(), 0, 1);

        // H-X-CZ-X-H is the two-qubit inversion-about-the-mean operator.
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1);
        circuit.addSingleQubitGate("X", gates::xGate(), 0);
        circuit.addSingleQubitGate("X", gates::xGate(), 1);
        circuit.addControlledGate("CZ", gates::czGate(), 0, 1);
        circuit.addSingleQubitGate("X", gates::xGate(), 0);
        circuit.addSingleQubitGate("X", gates::xGate(), 1);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1);

        return circuit;
    }

    circuit::QuantumCircuit deutschJozsaCircuit() {
        circuit::QuantumCircuit circuit{3};
        constexpr std::size_t ancilla = 2U;

        // Prepare |00>|1>, then place all three qubits into the oracle basis.
        circuit.addSingleQubitGate("X", gates::xGate(), ancilla);

        for (std::size_t qubit = 0; qubit < 3U; ++qubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                qubit
            );
        }

        // These two controls implement the balanced function x0 XOR x1.
        circuit.addControlledGate(
            "CX",
            gates::cxGate(3, 0, ancilla),
            0,
            ancilla
        );

        circuit.addControlledGate(
            "CX",
            gates::cxGate(3, 1, ancilla),
            1,
            ancilla
        );

        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1);

        return circuit;
    }

    circuit::QuantumCircuit bernsteinVaziraniCircuit(
        std::size_t inputQubitCount,
        std::size_t hiddenValue
    ) {
        if (
            inputQubitCount == 0U ||
            inputQubitCount >= std::numeric_limits<std::size_t>::digits - 1U
        ) {
            throw std::invalid_argument{
                "Bernstein-Vazirani input qubit count is invalid."
            };
        }

        const std::size_t hiddenValueLimit =
                std::size_t{1} << inputQubitCount;

        if (hiddenValue >= hiddenValueLimit) {
            throw std::invalid_argument{
                "Bernstein-Vazirani hidden value does not fit the input register."
            };
        }

        const std::size_t qubitCount =
                inputQubitCount + 1U;

        const std::size_t ancilla =
                qubitCount - 1U;

        circuit::QuantumCircuit circuit{qubitCount};

        circuit.addSingleQubitGate("X", gates::xGate(), ancilla);

        for (std::size_t qubit = 0; qubit < qubitCount; ++qubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                qubit
            );
        }

        for (std::size_t qubit = 0; qubit < inputQubitCount; ++qubit) {
            const std::size_t hiddenBit =
                    inputQubitCount - 1U - qubit;

            if ((hiddenValue & (std::size_t{1} << hiddenBit)) == 0U) {
                continue;
            }

            circuit.addControlledGate(
                "CX",
                gates::cxGate(qubitCount, qubit, ancilla),
                qubit,
                ancilla
            );
        }

        for (std::size_t qubit = 0; qubit < inputQubitCount; ++qubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                qubit
            );
        }

        return circuit;
    }

    circuit::QuantumCircuit toffoliDemoCircuit() {
        circuit::QuantumCircuit circuit{3};

        // Start both controls in |1> so the decomposition visibly flips q2.
        circuit.addSingleQubitGate("X", gates::xGate(), 0);
        circuit.addSingleQubitGate("X", gates::xGate(), 1);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 2);
        circuit.addControlledGate("CX", gates::cxGate(3, 1, 2), 1, 2);
        circuit.addSingleQubitGate(
            "Tdg",
            gates::tDaggerGate(),
            2
        );
        circuit.addControlledGate("CX", gates::cxGate(3, 0, 2), 0, 2);
        circuit.addSingleQubitGate("T", gates::tGate(), 2);
        circuit.addControlledGate("CX", gates::cxGate(3, 1, 2), 1, 2);
        circuit.addSingleQubitGate(
            "Tdg",
            gates::tDaggerGate(),
            2
        );
        circuit.addControlledGate("CX", gates::cxGate(3, 0, 2), 0, 2);
        circuit.addSingleQubitGate("T", gates::tGate(), 1);
        circuit.addSingleQubitGate("T", gates::tGate(), 2);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 2);
        circuit.addControlledGate("CX", gates::cxGate(3, 0, 1), 0, 1);
        circuit.addSingleQubitGate("T", gates::tGate(), 0);
        circuit.addSingleQubitGate(
            "Tdg",
            gates::tDaggerGate(),
            1
        );
        circuit.addControlledGate("CX", gates::cxGate(3, 0, 1), 0, 1);

        return circuit;
    }

    circuit::QuantumCircuit phaseKickbackCircuit() {
        circuit::QuantumCircuit circuit{2};

        // The target |-> is a -1 eigenstate of X, so CX kicks phase to q0.
        circuit.addSingleQubitGate("X", gates::xGate(), 1);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1);
        circuit.addControlledGate("CX", gates::cxGate(), 0, 1);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1);

        return circuit;
    }

    circuit::QuantumCircuit teleportationCircuit() {
        circuit::QuantumCircuit circuit{3};

        constexpr double inputTheta =
                std::numbers::pi / 3.0;

        constexpr double inputPhi =
                std::numbers::pi / 5.0;

        // Prepare a non-trivial source state so both magnitude and phase transfer.
        circuit.addSingleQubitGate(
            "Ry",
            gates::ryGate(inputTheta),
            0,
            inputTheta
        );

        circuit.addSingleQubitGate(
            "Rz",
            gates::rzGate(inputPhi),
            0,
            inputPhi
        );

        // Create the Bell resource between q1 and q2.
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1);
        circuit.addControlledGate("CX", gates::cxGate(3, 1, 2), 1, 2);

        // Bell-basis transform of the source and sender resource qubit.
        circuit.addControlledGate("CX", gates::cxGate(3, 0, 1), 0, 1);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0);

        // Coherent controls model the classical X/Z corrections without measurement.
        circuit.addControlledGate("CX", gates::cxGate(3, 1, 2), 1, 2);
        circuit.addControlledGate("CZ", gates::czGate(3, 0, 2), 0, 2);

        return circuit;
    }

    circuit::QuantumCircuit scrambleCircuit(std::size_t qubitCount) {
        circuit::QuantumCircuit circuit{qubitCount};

        for (std::size_t qubit = 0; qubit < qubitCount; ++qubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                qubit
            );

            if (qubit % 3U == 0U) {
                circuit.addSingleQubitGate(
                    "X",
                    gates::xGate(),
                    qubit
                );
            } else if (qubit % 3U == 1U) {
                circuit.addSingleQubitGate(
                    "S",
                    gates::sGate(),
                    qubit
                );
            } else {
                const double angleRadians =
                        std::numbers::pi / 3.0;

                circuit.addSingleQubitGate(
                    "Ry",
                    gates::ryGate(angleRadians),
                    qubit,
                    angleRadians
                );
            }
        }

        for (std::size_t qubit = 0; qubit + 1U < qubitCount; ++qubit) {
            circuit.addControlledGate(
                qubit % 2U == 0U ? "CX" : "CZ",
                qubit % 2U == 0U
                    ? gates::cxGate(qubitCount, qubit, qubit + 1U)
                    : gates::czGate(qubitCount, qubit, qubit + 1U),
                qubit,
                qubit + 1U
            );
        }

        if (qubitCount > 2U) {
            circuit.addControlledGate(
                "SWAP",
                gates::swapGate(qubitCount, 0, qubitCount - 1U),
                0,
                qubitCount - 1U
            );
        }

        return circuit;
    }

    circuit::QuantumCircuit rxRotationCircuit(double angleRadians) {
        circuit::QuantumCircuit circuit{1};

        circuit.addSingleQubitGate(
            "Rx",
            gates::rxGate(angleRadians),
            0,
            angleRadians
        );

        return circuit;
    }

    circuit::QuantumCircuit ryRotationCircuit(double angleRadians) {
        circuit::QuantumCircuit circuit{1};

        circuit.addSingleQubitGate(
            "Ry",
            gates::ryGate(angleRadians),
            0,
            angleRadians
        );

        return circuit;
    }

    circuit::QuantumCircuit rzRotationCircuit(double angleRadians) {
        circuit::QuantumCircuit circuit{1};

        circuit.addSingleQubitGate(
            "Rz",
            gates::rzGate(angleRadians),
            0,
            angleRadians
        );

        return circuit;
    }
}

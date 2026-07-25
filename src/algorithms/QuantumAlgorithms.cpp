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

                // Keep the controlled phase decomposed so the debugger exposes every layer.
                circuit.addSingleQubitGate(
                    "Rz",
                    gates::rzGate(angleRadians * 0.5),
                    control
                );

                circuit.addControlledGate(
                    "CX",
                    gates::cxGate(qubitCount, control, target),
                    control,
                    target
                );

                circuit.addSingleQubitGate(
                    "Rz",
                    gates::rzGate(-angleRadians * 0.5),
                    target
                );

                circuit.addControlledGate(
                    "CX",
                    gates::cxGate(qubitCount, control, target),
                    control,
                    target
                );

                circuit.addSingleQubitGate(
                    "Rz",
                    gates::rzGate(angleRadians * 0.5),
                    target
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
            circuit.addSingleQubitGate(
                qubit % 2U == 0U ? "S" : "T",
                qubit % 2U == 0U ? gates::sGate() : gates::tGate(),
                qubit
            );

            circuit.addSingleQubitGate(
                "Rz",
                gates::rzGate(
                    std::numbers::pi *
                    static_cast<double>(qubit + 1U) /
                    16.0
                ),
                qubit
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

    circuit::QuantumCircuit rxRotationCircuit(double angleRadians) {
        circuit::QuantumCircuit circuit{1};

        circuit.addSingleQubitGate("Rx", gates::rxGate(angleRadians), 0);

        return circuit;
    }

    circuit::QuantumCircuit ryRotationCircuit(double angleRadians) {
        circuit::QuantumCircuit circuit{1};

        circuit.addSingleQubitGate("Ry", gates::ryGate(angleRadians), 0);

        return circuit;
    }

    circuit::QuantumCircuit rzRotationCircuit(double angleRadians) {
        circuit::QuantumCircuit circuit{1};

        circuit.addSingleQubitGate("Rz", gates::rzGate(angleRadians), 0);

        return circuit;
    }
}

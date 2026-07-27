#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"

#include "quantum_sim/gates/QuantumGates.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <random>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace {
    using quantum_sim::circuit::QuantumCircuit;

    void requireMinimumQubitCount(
        const std::size_t qubitCount,
        const std::size_t minimum,
        const char *errorMessage
    ) {
        if (qubitCount < minimum) {
            throw std::invalid_argument{errorMessage};
        }
    }

    void appendControlledPhase(
        QuantumCircuit &circuit,
        const std::size_t controlQubit,
        const std::size_t targetQubit,
        const double angleRadians
    ) {
        const double halfAngle =
                angleRadians * 0.5;

        circuit.addSingleQubitGate(
            "Rz",
            quantum_sim::gates::rzGate(halfAngle),
            controlQubit,
            halfAngle
        );

        circuit.addTwoQubitGate(
            "CX",
            quantum_sim::gates::cxGate(),
            controlQubit,
            targetQubit
        );

        circuit.addSingleQubitGate(
            "Rz",
            quantum_sim::gates::rzGate(-halfAngle),
            targetQubit,
            -halfAngle
        );

        circuit.addTwoQubitGate(
            "CX",
            quantum_sim::gates::cxGate(),
            controlQubit,
            targetQubit
        );

        circuit.addSingleQubitGate(
            "Rz",
            quantum_sim::gates::rzGate(halfAngle),
            targetQubit,
            halfAngle
        );
    }

    void appendForwardQft(
        QuantumCircuit &circuit,
        const std::size_t firstQubit,
        const std::size_t qubitCount
    ) {
        for (std::size_t targetOffset = 0U; targetOffset < qubitCount; ++targetOffset) {
            const std::size_t target =
                    firstQubit + targetOffset;

            circuit.addSingleQubitGate(
                "H",
                quantum_sim::gates::hadamardGate(),
                target
            );

            for (
                std::size_t controlOffset =
                        targetOffset + 1U;
                controlOffset < qubitCount;
                ++controlOffset
            ) {
                const double angle =
                        std::numbers::pi /
                        static_cast<double>(
                            std::size_t{1}
                            << (controlOffset - targetOffset)
                        );

                appendControlledPhase(
                    circuit,
                    firstQubit + controlOffset,
                    target,
                    angle
                );
            }
        }

        for (std::size_t offset = 0U; offset < qubitCount / 2U; ++offset) {
            circuit.addTwoQubitGate(
                "SWAP",
                quantum_sim::gates::swapGate(),
                firstQubit + offset,
                firstQubit + qubitCount - 1U - offset
            );
        }
    }

    void appendInverseQft(
        QuantumCircuit &circuit,
        const std::size_t firstQubit,
        const std::size_t qubitCount
    ) {
        for (std::size_t offset = 0U; offset < qubitCount / 2U; ++offset) {
            circuit.addTwoQubitGate(
                "SWAP",
                quantum_sim::gates::swapGate(),
                firstQubit + offset,
                firstQubit + qubitCount - 1U - offset
            );
        }

        for (std::size_t targetOffset = qubitCount; targetOffset-- > 0U;) {
            const std::size_t target =
                    firstQubit + targetOffset;

            for (
                std::size_t controlOffset = qubitCount;
                controlOffset-- > targetOffset + 1U;
            ) {
                const double angle =
                        -std::numbers::pi /
                        static_cast<double>(
                            std::size_t{1}
                            << (controlOffset - targetOffset)
                        );

                appendControlledPhase(
                    circuit,
                    firstQubit + controlOffset,
                    target,
                    angle
                );
            }

            circuit.addSingleQubitGate(
                "H",
                quantum_sim::gates::hadamardGate(),
                target
            );
        }
    }

    void appendToffoli(
        QuantumCircuit &circuit,
        const std::size_t firstControl,
        const std::size_t secondControl,
        const std::size_t target
    ) {
        circuit.addSingleQubitGate("H", quantum_sim::gates::hadamardGate(), target);
        circuit.addTwoQubitGate("CX", quantum_sim::gates::cxGate(), secondControl, target);
        circuit.addSingleQubitGate("Tdg", quantum_sim::gates::tDaggerGate(), target);
        circuit.addTwoQubitGate("CX", quantum_sim::gates::cxGate(), firstControl, target);
        circuit.addSingleQubitGate("T", quantum_sim::gates::tGate(), target);
        circuit.addTwoQubitGate("CX", quantum_sim::gates::cxGate(), secondControl, target);
        circuit.addSingleQubitGate("Tdg", quantum_sim::gates::tDaggerGate(), target);
        circuit.addTwoQubitGate("CX", quantum_sim::gates::cxGate(), firstControl, target);
        circuit.addSingleQubitGate("T", quantum_sim::gates::tGate(), secondControl);
        circuit.addSingleQubitGate("T", quantum_sim::gates::tGate(), target);
        circuit.addSingleQubitGate("H", quantum_sim::gates::hadamardGate(), target);
        circuit.addTwoQubitGate("CX", quantum_sim::gates::cxGate(), firstControl, secondControl);
        circuit.addSingleQubitGate("T", quantum_sim::gates::tGate(), firstControl);
        circuit.addSingleQubitGate("Tdg", quantum_sim::gates::tDaggerGate(), secondControl);
        circuit.addTwoQubitGate("CX", quantum_sim::gates::cxGate(), firstControl, secondControl);
    }

    void appendControlledRy(
        QuantumCircuit &circuit,
        const std::size_t controlQubit,
        const std::size_t targetQubit,
        const double angleRadians
    ) {
        const double halfAngle =
                angleRadians * 0.5;

        circuit.addSingleQubitGate(
            "Ry",
            quantum_sim::gates::ryGate(halfAngle),
            targetQubit,
            halfAngle
        );

        circuit.addTwoQubitGate(
            "CX",
            quantum_sim::gates::cxGate(),
            controlQubit,
            targetQubit
        );

        circuit.addSingleQubitGate(
            "Ry",
            quantum_sim::gates::ryGate(-halfAngle),
            targetQubit,
            -halfAngle
        );

        circuit.addTwoQubitGate(
            "CX",
            quantum_sim::gates::cxGate(),
            controlQubit,
            targetQubit
        );
    }

    void appendControlledSwap(
        QuantumCircuit &circuit,
        const std::size_t controlQubit,
        const std::size_t firstTarget,
        const std::size_t secondTarget
    ) {
        circuit.addTwoQubitGate(
            "CX",
            quantum_sim::gates::cxGate(),
            secondTarget,
            firstTarget
        );

        appendToffoli(
            circuit,
            controlQubit,
            firstTarget,
            secondTarget
        );

        circuit.addTwoQubitGate(
            "CX",
            quantum_sim::gates::cxGate(),
            secondTarget,
            firstTarget
        );
    }

    [[nodiscard]] quantum_sim::math::ComplexVector statePreparationAxis(
        const std::vector<double> &targetAmplitudes
    ) {
        std::vector<quantum_sim::math::Complex> axis;
        axis.reserve(targetAmplitudes.size());

        for (std::size_t index = 0U; index < targetAmplitudes.size(); ++index) {
            axis.emplace_back(
                (
                    index == 0U
                        ? 1.0
                        : 0.0
                ) -
                targetAmplitudes[index],
                0.0
            );
        }

        return quantum_sim::math::ComplexVector{
            std::move(axis)
        }.normalized();
    }

    [[nodiscard]] quantum_sim::math::ComplexVector basisReflectionAxis(
        const std::size_t stateCount,
        const std::size_t markedState
    ) {
        std::vector<quantum_sim::math::Complex> axis(
            stateCount,
            quantum_sim::math::Complex{}
        );

        axis.at(markedState) =
                quantum_sim::math::Complex{1.0, 0.0};

        return quantum_sim::math::ComplexVector{
            std::move(axis)
        };
    }

    [[nodiscard]] quantum_sim::math::ComplexVector uniformReflectionAxis(
        const std::size_t stateCount
    ) {
        const double amplitude =
                1.0 /
                std::sqrt(
                    static_cast<double>(stateCount)
                );

        return quantum_sim::math::ComplexVector{
            std::vector(
                stateCount,
                quantum_sim::math::Complex{
                    amplitude,
                    0.0
                }
            )
        };
    }

    void appendControlledGroverIteration(
        QuantumCircuit &circuit,
        const std::size_t controlQubit,
        const std::size_t firstSearchQubit,
        const std::size_t secondSearchQubit
    ) {
        // H-CCX-H is a controlled-CZ that marks |11> only when enabled.
        circuit.addSingleQubitGate(
            "H",
            quantum_sim::gates::hadamardGate(),
            secondSearchQubit
        );
        circuit.addThreeQubitGate(
            "CCX",
            quantum_sim::gates::ccxGate(),
            controlQubit,
            firstSearchQubit,
            secondSearchQubit
        );
        circuit.addSingleQubitGate(
            "H",
            quantum_sim::gates::hadamardGate(),
            secondSearchQubit
        );

        // Every diffusion operation is itself controlled, so a disabled power
        // leaves the complete two-qubit search register unchanged.
        for (const std::size_t searchQubit : {
            firstSearchQubit,
            secondSearchQubit
        }) {
            circuit.addTwoQubitGate(
                "CH",
                quantum_sim::gates::chGate(),
                controlQubit,
                searchQubit
            );
            circuit.addTwoQubitGate(
                "CX",
                quantum_sim::gates::cxGate(),
                controlQubit,
                searchQubit
            );
        }

        circuit.addSingleQubitGate(
            "H",
            quantum_sim::gates::hadamardGate(),
            secondSearchQubit
        );
        circuit.addThreeQubitGate(
            "CCX",
            quantum_sim::gates::ccxGate(),
            controlQubit,
            firstSearchQubit,
            secondSearchQubit
        );
        circuit.addSingleQubitGate(
            "H",
            quantum_sim::gates::hadamardGate(),
            secondSearchQubit
        );

        for (const std::size_t searchQubit : {
            firstSearchQubit,
            secondSearchQubit
        }) {
            circuit.addTwoQubitGate(
                "CX",
                quantum_sim::gates::cxGate(),
                controlQubit,
                searchQubit
            );
            circuit.addTwoQubitGate(
                "CH",
                quantum_sim::gates::chGate(),
                controlQubit,
                searchQubit
            );
        }
    }
}

namespace quantum_sim::algorithms {
    circuit::QuantumCircuit bellStateCircuit(const std::size_t qubitCount) {
        requireMinimumQubitCount(
            qubitCount,
            2U,
            "A Bell-state circuit requires at least two qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        // H creates superposition; CX entangles q1 with q0.
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0);
        circuit.addTwoQubitGate(
            "CX",
            gates::cxGate(),
            0,
            1
        );

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

                circuit.addTwoQubitGate(
                    "CX",
                    gates::cxGate(),
                    control,
                    target
                );

                circuit.addSingleQubitGate(
                    "Rz",
                    gates::rzGate(-halfAngleRadians),
                    target,
                    -halfAngleRadians
                );

                circuit.addTwoQubitGate(
                    "CX",
                    gates::cxGate(),
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
            circuit.addTwoQubitGate(
                "SWAP",
                gates::swapGate(),
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
            circuit.addTwoQubitGate(
                "SWAP",
                gates::swapGate(),
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

                circuit.addTwoQubitGate(
                    "CX",
                    gates::cxGate(),
                    control,
                    target
                );

                circuit.addSingleQubitGate(
                    "Rz",
                    gates::rzGate(halfAngleRadians),
                    target,
                    halfAngleRadians
                );

                circuit.addTwoQubitGate(
                    "CX",
                    gates::cxGate(),
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

    circuit::QuantumCircuit ghzStateCircuit(const std::size_t qubitCount) {
        circuit::QuantumCircuit circuit{qubitCount};

        // A Hadamard seed followed by a CX chain entangles the full register.
        circuit.addSingleQubitGate(
            "H",
            gates::hadamardGate(),
            0
        );

        for (std::size_t target = 1U; target < qubitCount; ++target) {
            const std::size_t control =
                    target - 1U;

            circuit.addTwoQubitGate(
                "CX",
                gates::cxGate(),
                control,
                target
            );
        }

        return circuit;
    }

    circuit::QuantumCircuit groverSearchCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            2U,
            "The Grover demonstration requires at least two qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        for (std::size_t qubit = 0U; qubit < qubitCount; ++qubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                qubit
            );
        }

        const std::size_t stateCount =
                std::size_t{1} << qubitCount;

        const std::size_t markedState =
                stateCount - 1U;

        const std::size_t iterationCount =
                std::max(
                    std::size_t{1},
                    static_cast<std::size_t>(
                        std::floor(
                            std::numbers::pi *
                            std::sqrt(
                                static_cast<double>(stateCount)
                            ) /
                            4.0
                        )
                    )
                );

        const math::ComplexVector oracleAxis =
                basisReflectionAxis(
                    stateCount,
                    markedState
                );

        const math::ComplexVector diffusionAxis =
                uniformReflectionAxis(
                    stateCount
                );

        for (std::size_t iteration = 0U; iteration < iterationCount; ++iteration) {
            circuit.addReflection(
                "Oracle",
                oracleAxis
            );

            // The sign differs from conventional diffusion only by a global phase.
            circuit.addReflection(
                "Diffuse",
                diffusionAxis
            );
        }

        return circuit;
    }

    circuit::QuantumCircuit deutschJozsaCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            2U,
            "Deutsch-Jozsa requires at least one input qubit and one ancilla."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        const std::size_t ancilla =
                qubitCount - 1U;

        // Prepare the ancilla in |1⟩, then move the full register into the oracle basis.
        circuit.addSingleQubitGate("X", gates::xGate(), ancilla);

        for (std::size_t qubit = 0; qubit < qubitCount; ++qubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                qubit
            );
        }

        // One oracle control per input computes the balanced parity function.
        for (std::size_t input = 0; input < ancilla; ++input) {
            circuit.addTwoQubitGate(
                "CX",
                gates::cxGate(),
                input,
                ancilla
            );
        }

        for (std::size_t input = 0; input < ancilla; ++input) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                input
            );
        }

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

            circuit.addTwoQubitGate(
                "CX",
                gates::cxGate(),
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

    circuit::QuantumCircuit toffoliDemoCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            3U,
            "The Toffoli demonstration requires at least three qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        // Start both controls in |1⟩ so the decomposition visibly flips q2.
        circuit.addSingleQubitGate("X", gates::xGate(), 0);
        circuit.addSingleQubitGate("X", gates::xGate(), 1);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 2);
        circuit.addTwoQubitGate("CX", gates::cxGate(), 1, 2);
        circuit.addSingleQubitGate(
            "Tdg",
            gates::tDaggerGate(),
            2
        );
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0, 2);
        circuit.addSingleQubitGate("T", gates::tGate(), 2);
        circuit.addTwoQubitGate("CX", gates::cxGate(), 1, 2);
        circuit.addSingleQubitGate(
            "Tdg",
            gates::tDaggerGate(),
            2
        );
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0, 2);
        circuit.addSingleQubitGate("T", gates::tGate(), 1);
        circuit.addSingleQubitGate("T", gates::tGate(), 2);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 2);
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0, 1);
        circuit.addSingleQubitGate("T", gates::tGate(), 0);
        circuit.addSingleQubitGate(
            "Tdg",
            gates::tDaggerGate(),
            1
        );
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0, 1);

        return circuit;
    }

    circuit::QuantumCircuit phaseKickbackCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            2U,
            "The phase-kickback demonstration requires at least two qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        // The target |-⟩ is a -1 eigenstate of X, so CX kicks phase to q0.
        circuit.addSingleQubitGate("X", gates::xGate(), 1);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1);
        circuit.addTwoQubitGate(
            "CX",
            gates::cxGate(),
            0,
            1
        );
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1);

        return circuit;
    }

    circuit::QuantumCircuit teleportationCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            3U,
            "The teleportation demonstration requires at least three qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

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
        circuit.addTwoQubitGate("CX", gates::cxGate(), 1, 2);

        // Bell-basis transform of the source and sender resource qubit.
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0, 1);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0);

        // Coherent controls model the classical X/Z corrections without measurement.
        circuit.addTwoQubitGate("CX", gates::cxGate(), 1, 2);
        circuit.addTwoQubitGate("CZ", gates::czGate(), 0, 2);

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
            circuit.addTwoQubitGate(
                qubit % 2U == 0U ? "CX" : "CZ",
                qubit % 2U == 0U
                    ? gates::cxGate()
                    : gates::czGate(),
                qubit,
                qubit + 1U
            );
        }

        if (qubitCount > 2U) {
            circuit.addTwoQubitGate(
                "SWAP",
                gates::swapGate(),
                0,
                qubitCount - 1U
            );
        }

        return circuit;
    }

    circuit::QuantumCircuit simonCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            4U,
            "The Simon demonstration requires two input and two oracle qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0U);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1U);

        // f(x0, x1) = (x0 xor x1, x0 xor x1) has hidden period s = 11.
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 2U);
        circuit.addTwoQubitGate("CX", gates::cxGate(), 1U, 2U);
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 3U);
        circuit.addTwoQubitGate("CX", gates::cxGate(), 1U, 3U);

        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0U);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1U);

        return circuit;
    }

    circuit::QuantumCircuit shorPeriodFindingCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            4U,
            "The compiled Shor period-finding demonstration requires four qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        // q3 represents the two-state work orbit of multiplication by 4 mod 15.
        circuit.addSingleQubitGate("X", gates::xGate(), 3U);

        for (std::size_t countingQubit = 0U; countingQubit < 3U; ++countingQubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                countingQubit
            );
        }

        // U^1 toggles the work orbit; U^2 and U^4 are identities for period 2.
        circuit.addTwoQubitGate(
            "CX",
            gates::cxGate(),
            2U,
            3U
        );

        appendInverseQft(
            circuit,
            0U,
            3U
        );

        return circuit;
    }

    circuit::QuantumCircuit quantumPhaseEstimationCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            3U,
            "Quantum phase estimation requires two counting qubits and one eigenstate qubit."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        circuit.addSingleQubitGate("X", gates::xGate(), 2U);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0U);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1U);

        // The |1> eigenstate has phase 1/4, so the two powers contribute pi and pi/2.
        appendControlledPhase(
            circuit,
            0U,
            2U,
            std::numbers::pi
        );

        appendControlledPhase(
            circuit,
            1U,
            2U,
            std::numbers::pi / 2.0
        );

        appendInverseQft(
            circuit,
            0U,
            2U
        );

        return circuit;
    }

    circuit::QuantumCircuit vqeAnsatzCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            2U,
            "The VQE ansatz requires at least two qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        constexpr double theta0 =
                0.37 * std::numbers::pi;

        constexpr double theta1 =
                -0.21 * std::numbers::pi;

        constexpr double correlationPhase =
                0.18 * std::numbers::pi;

        // Hartree-Fock seed |01>, followed by one hardware-efficient ansatz layer.
        circuit.addSingleQubitGate("X", gates::xGate(), 1U);
        circuit.addSingleQubitGate("Ry", gates::ryGate(theta0), 0U, theta0);
        circuit.addSingleQubitGate("Ry", gates::ryGate(theta1), 1U, theta1);
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 1U);
        circuit.addSingleQubitGate(
            "Rz",
            gates::rzGate(correlationPhase),
            1U,
            correlationPhase
        );
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 1U);

        return circuit;
    }

    circuit::QuantumCircuit qaoaMaxCutCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            2U,
            "QAOA Max-Cut requires at least two graph vertices."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        constexpr double gamma =
                0.28 * std::numbers::pi;

        constexpr double beta =
                0.19 * std::numbers::pi;

        for (std::size_t qubit = 0U; qubit < qubitCount; ++qubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                qubit
            );
        }

        const auto appendCostEdge =
                [&](const std::size_t first, const std::size_t second) {
            circuit.addTwoQubitGate(
                "CX",
                gates::cxGate(),
                first,
                second
            );

            circuit.addSingleQubitGate(
                "Rz",
                gates::rzGate(2.0 * gamma),
                second,
                2.0 * gamma
            );

            circuit.addTwoQubitGate(
                "CX",
                gates::cxGate(),
                first,
                second
            );
        };

        for (std::size_t qubit = 0U; qubit + 1U < qubitCount; ++qubit) {
            appendCostEdge(
                qubit,
                qubit + 1U
            );
        }

        if (qubitCount > 2U) {
            appendCostEdge(
                qubitCount - 1U,
                0U
            );
        }

        for (std::size_t qubit = 0U; qubit < qubitCount; ++qubit) {
            circuit.addSingleQubitGate(
                "Rx",
                gates::rxGate(2.0 * beta),
                qubit,
                2.0 * beta
            );
        }

        return circuit;
    }

    circuit::QuantumCircuit hhlDemoCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            4U,
            "The HHL demonstration requires two phase qubits, one system qubit, and one ancilla."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        constexpr double inputAngle =
                std::numbers::pi / 3.0;

        constexpr double reciprocalRotation =
                std::numbers::pi / 2.5;

        circuit.addSingleQubitGate(
            "Ry",
            gates::ryGate(inputAngle),
            2U,
            inputAngle
        );

        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0U);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1U);

        appendControlledPhase(circuit, 0U, 2U, std::numbers::pi);
        appendControlledPhase(circuit, 1U, 2U, std::numbers::pi / 2.0);
        appendInverseQft(circuit, 0U, 2U);

        // q1 carries the resolved non-zero eigenvalue in this fixed toy instance.
        appendControlledRy(
            circuit,
            1U,
            3U,
            reciprocalRotation
        );

        // Reverse phase estimation while retaining the solution ancilla.
        appendForwardQft(circuit, 0U, 2U);
        appendControlledPhase(circuit, 1U, 2U, -std::numbers::pi / 2.0);
        appendControlledPhase(circuit, 0U, 2U, -std::numbers::pi);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1U);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0U);

        return circuit;
    }

    circuit::QuantumCircuit swapTestCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            3U,
            "The SWAP test requires one ancilla and two state qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1U);
        circuit.addSingleQubitGate("X", gates::xGate(), 2U);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0U);

        appendControlledSwap(
            circuit,
            0U,
            1U,
            2U
        );

        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0U);

        return circuit;
    }

    circuit::QuantumCircuit quantumWalkCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            3U,
            "The coined quantum walk requires one coin and two position qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        for (std::size_t walkStep = 0U; walkStep < 2U; ++walkStep) {
            circuit.addSingleQubitGate("H", gates::hadamardGate(), 0U);

            // Coin |1> increments the two-bit position modulo four.
            appendToffoli(circuit, 0U, 2U, 1U);
            circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 2U);

            // Invert the coin and low position bit to control the decrement branch.
            circuit.addSingleQubitGate("X", gates::xGate(), 0U);
            circuit.addSingleQubitGate("X", gates::xGate(), 2U);
            appendToffoli(circuit, 0U, 2U, 1U);
            circuit.addSingleQubitGate("X", gates::xGate(), 2U);
            circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 2U);
            circuit.addSingleQubitGate("X", gates::xGate(), 0U);
        }

        return circuit;
    }

    circuit::QuantumCircuit bb84DemoCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            2U,
            "The BB84 basis demonstration requires two signal qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        // Alice encodes bit 1 diagonally on q0; Bob selects the matching basis.
        circuit.addSingleQubitGate("X", gates::xGate(), 0U);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0U);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0U);

        // q1 carries bit 0 in Z, but Bob chooses X and obtains a random result.
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 1U);

        return circuit;
    }

    circuit::QuantumCircuit superdenseCodingCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            2U,
            "Superdense coding requires a shared two-qubit Bell pair."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0U);
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 1U);

        // Message 11 applies both Z and X to the sender's half.
        circuit.addSingleQubitGate("Z", gates::zGate(), 0U);
        circuit.addSingleQubitGate("X", gates::xGate(), 0U);

        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 1U);
        circuit.addSingleQubitGate("H", gates::hadamardGate(), 0U);

        return circuit;
    }

    circuit::QuantumCircuit wStateCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            2U,
            "A W state requires at least two qubits."
        );

        const std::size_t stateCount =
                std::size_t{1} << qubitCount;

        const double amplitude =
                1.0 /
                std::sqrt(
                    static_cast<double>(qubitCount)
                );

        std::vector<double> targetAmplitudes(
            stateCount,
            0.0
        );

        for (std::size_t basisState = 0U; basisState < stateCount; ++basisState) {
            if (std::popcount(basisState) == 1) {
                targetAmplitudes[basisState] =
                        amplitude;
            }
        }

        circuit::QuantumCircuit circuit{qubitCount};
        circuit.addReflection(
            "W",
            statePreparationAxis(targetAmplitudes)
        );

        return circuit;
    }

    circuit::QuantumCircuit dickeStateCircuit(
        const std::size_t qubitCount,
        const std::size_t excitationCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            2U,
            "A Dicke state requires at least two qubits."
        );

        if (
            excitationCount == 0U ||
            excitationCount > qubitCount
        ) {
            throw std::invalid_argument{
                "Dicke excitation count must be between one and the register size."
            };
        }

        const std::size_t stateCount =
                std::size_t{1} << qubitCount;

        std::size_t populatedStateCount{};

        for (std::size_t basisState = 0U; basisState < stateCount; ++basisState) {
            if (
                std::popcount(basisState) ==
                static_cast<int>(excitationCount)
            ) {
                ++populatedStateCount;
            }
        }

        const double amplitude =
                1.0 /
                std::sqrt(
                    static_cast<double>(
                        populatedStateCount
                    )
                );

        std::vector<double> targetAmplitudes(
            stateCount,
            0.0
        );

        for (std::size_t basisState = 0U; basisState < stateCount; ++basisState) {
            if (
                std::popcount(basisState) ==
                static_cast<int>(excitationCount)
            ) {
                targetAmplitudes[basisState] =
                        amplitude;
            }
        }

        circuit::QuantumCircuit circuit{qubitCount};
        circuit.addReflection(
            "Dicke",
            statePreparationAxis(targetAmplitudes)
        );

        return circuit;
    }

    circuit::QuantumCircuit graphStateCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            2U,
            "A graph state requires at least two qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        for (std::size_t qubit = 0U; qubit < qubitCount; ++qubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                qubit
            );
        }

        for (std::size_t qubit = 0U; qubit + 1U < qubitCount; ++qubit) {
            circuit.addTwoQubitGate(
                "CZ",
                gates::czGate(),
                qubit,
                qubit + 1U
            );
        }

        return circuit;
    }

    circuit::QuantumCircuit randomCircuit(
        const std::size_t qubitCount,
        const std::uint64_t seed,
        const std::size_t layerCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            2U,
            "A random circuit requires at least two qubits."
        );

        if (layerCount == 0U) {
            throw std::invalid_argument{
                "A random circuit requires at least one layer."
            };
        }

        std::mt19937_64 randomEngine{seed};

        std::uniform_real_distribution<double> angleDistribution{
            -std::numbers::pi,
            std::numbers::pi
        };

        circuit::QuantumCircuit circuit{qubitCount};

        for (std::size_t layer = 0U; layer < layerCount; ++layer) {
            for (std::size_t qubit = 0U; qubit < qubitCount; ++qubit) {
                const double ryAngle =
                        angleDistribution(randomEngine);

                const double rzAngle =
                        angleDistribution(randomEngine);

                circuit.addSingleQubitGate(
                    "Ry",
                    gates::ryGate(ryAngle),
                    qubit,
                    ryAngle
                );

                circuit.addSingleQubitGate(
                    "Rz",
                    gates::rzGate(rzAngle),
                    qubit,
                    rzAngle
                );
            }

            for (std::size_t edge = 0U; edge < qubitCount; ++edge) {
                const std::size_t control =
                        (
                            edge +
                            layer
                        ) %
                        qubitCount;

                const std::size_t target =
                        (
                            control +
                            1U
                        ) %
                        qubitCount;

                circuit.addTwoQubitGate(
                    "CX",
                    gates::cxGate(),
                    control,
                    target
                );
            }
        }

        return circuit;
    }

    circuit::QuantumCircuit weightedStatePreparationCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            1U,
            "Weighted state preparation requires at least one qubit."
        );

        const std::size_t stateCount =
                std::size_t{1} << qubitCount;

        std::vector<double> weights(
            stateCount,
            0.0
        );

        double totalWeight{};

        for (std::size_t state = 0U; state < stateCount; ++state) {
            const double weight =
                    1.0 +
                    static_cast<double>(
                        (
                            state * 37U +
                            11U
                        ) %
                        97U
                    );

            weights[state] = weight;
            totalWeight += weight;
        }

        std::vector<double> targetAmplitudes(
            stateCount,
            0.0
        );

        for (std::size_t state = 0U; state < stateCount; ++state) {
            targetAmplitudes[state] =
                    std::sqrt(
                        weights[state] /
                        totalWeight
                    );
        }

        circuit::QuantumCircuit circuit{qubitCount};
        circuit.addReflection(
            "Weighted",
            statePreparationAxis(targetAmplitudes)
        );

        return circuit;
    }

    circuit::QuantumCircuit bitFlipCodeCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            3U,
            "The bit-flip code requires three qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        constexpr double inputAngle =
                std::numbers::pi / 3.0;

        circuit.addSingleQubitGate(
            "Ry",
            gates::ryGate(inputAngle),
            0U,
            inputAngle
        );

        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 1U);
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 2U);
        circuit.addSingleQubitGate("X", gates::xGate(), 1U);
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 1U);
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 2U);
        appendToffoli(circuit, 1U, 2U, 0U);

        return circuit;
    }

    circuit::QuantumCircuit steaneCodeCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            7U,
            "The Steane code requires seven qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        for (const std::size_t pivot : {0U, 1U, 3U}) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                pivot
            );
        }

        for (const auto [control, target] : {
            std::pair{0U, 2U},
            std::pair{0U, 4U},
            std::pair{0U, 6U},
            std::pair{1U, 2U},
            std::pair{1U, 5U},
            std::pair{1U, 6U},
            std::pair{3U, 4U},
            std::pair{3U, 5U},
            std::pair{3U, 6U}
        }) {
            circuit.addTwoQubitGate(
                "CX",
                gates::cxGate(),
                control,
                target
            );
        }

        return circuit;
    }

    circuit::QuantumCircuit shorCodeCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            9U,
            "The Shor code requires nine qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        constexpr double inputAngle =
                0.37 *
                std::numbers::pi;

        circuit.addSingleQubitGate(
            "Ry",
            gates::ryGate(inputAngle),
            0U,
            inputAngle
        );

        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 3U);
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 6U);

        for (const std::size_t leader : {0U, 3U, 6U}) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                leader
            );

            circuit.addTwoQubitGate(
                "CX",
                gates::cxGate(),
                leader,
                leader + 1U
            );

            circuit.addTwoQubitGate(
                "CX",
                gates::cxGate(),
                leader,
                leader + 2U
            );
        }

        return circuit;
    }

    circuit::QuantumCircuit phaseFlipCodeCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            3U,
            "The phase-flip code requires three qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        constexpr double inputAngle =
                std::numbers::pi / 3.0;

        circuit.addSingleQubitGate(
            "Ry",
            gates::ryGate(inputAngle),
            0U,
            inputAngle
        );

        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 1U);
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 2U);

        for (std::size_t qubit = 0U; qubit < 3U; ++qubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                qubit
            );
        }

        circuit.addSingleQubitGate("Z", gates::zGate(), 1U);

        for (std::size_t qubit = 0U; qubit < 3U; ++qubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                qubit
            );
        }

        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 1U);
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 2U);
        circuit.addThreeQubitGate(
            "CCX",
            gates::ccxGate(),
            1U,
            2U,
            0U
        );

        return circuit;
    }

    circuit::QuantumCircuit fiveQubitCodeCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            5U,
            "The five-qubit perfect code requires five qubits."
        );

        const std::size_t stateCount =
                std::size_t{1} << qubitCount;

        std::vector<double> targetAmplitudes(
            stateCount,
            0.0
        );

        constexpr std::pair<std::size_t, double> logicalZeroTerms[]{
            {0b00000U, 1.0},
            {0b10010U, 1.0},
            {0b01001U, 1.0},
            {0b10100U, 1.0},
            {0b01010U, 1.0},
            {0b11011U, -1.0},
            {0b00110U, -1.0},
            {0b11000U, -1.0},
            {0b11101U, -1.0},
            {0b00011U, -1.0},
            {0b11110U, -1.0},
            {0b01111U, -1.0},
            {0b10001U, -1.0},
            {0b01100U, -1.0},
            {0b10111U, -1.0},
            {0b00101U, 1.0}
        };

        const std::size_t trailingQubitCount =
                qubitCount - 5U;

        for (const auto [codeword, sign] : logicalZeroTerms) {
            targetAmplitudes.at(
                codeword << trailingQubitCount
            ) =
                    sign * 0.25;
        }

        circuit::QuantumCircuit circuit{qubitCount};
        circuit.addReflection(
            "5Q Code",
            statePreparationAxis(targetAmplitudes)
        );

        return circuit;
    }

    circuit::QuantumCircuit quantumCountingCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            5U,
            "Quantum counting requires three counting qubits and two search qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        for (std::size_t qubit = 0U; qubit < 5U; ++qubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                qubit
            );
        }

        for (std::size_t countingQubit = 0U; countingQubit < 3U; ++countingQubit) {
            const std::size_t power =
                    std::size_t{1} <<
                    (2U - countingQubit);

            for (std::size_t repetition = 0U; repetition < power; ++repetition) {
                appendControlledGroverIteration(
                    circuit,
                    countingQubit,
                    3U,
                    4U
                );
            }
        }

        appendInverseQft(circuit, 0U, 3U);
        return circuit;
    }

    circuit::QuantumCircuit amplitudeEstimationCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            4U,
            "Amplitude estimation requires three counting qubits and one state qubit."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        constexpr double targetProbability = 0.30;
        const double preparationAngle =
                2.0 *
                std::asin(
                    std::sqrt(targetProbability)
                );

        circuit.addSingleQubitGate(
            "Ry",
            gates::ryGate(preparationAngle),
            3U,
            preparationAngle
        );

        for (std::size_t qubit = 0U; qubit < 3U; ++qubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                qubit
            );
        }

        for (std::size_t countingQubit = 0U; countingQubit < 3U; ++countingQubit) {
            const double poweredAngle =
                    4.0 *
                    std::asin(
                        std::sqrt(targetProbability)
                    ) *
                    static_cast<double>(
                        std::size_t{1} <<
                        (2U - countingQubit)
                    );

            circuit.addTwoQubitGate(
                "CRy",
                gates::cryGate(poweredAngle),
                countingQubit,
                3U,
                poweredAngle
            );
        }

        appendInverseQft(circuit, 0U, 3U);
        return circuit;
    }

    circuit::QuantumCircuit rippleCarryAdderCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            4U,
            "The two-bit ripple-carry adder requires four qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        // A=01 occupies q0/q1 and B=10 occupies q2/q3.
        circuit.addSingleQubitGate("X", gates::xGate(), 1U);
        circuit.addSingleQubitGate("X", gates::xGate(), 2U);

        circuit.addThreeQubitGate(
            "CCX",
            gates::ccxGate(),
            1U,
            3U,
            2U
        );
        circuit.addTwoQubitGate("CX", gates::cxGate(), 0U, 2U);
        circuit.addTwoQubitGate("CX", gates::cxGate(), 1U, 3U);

        return circuit;
    }

    circuit::QuantumCircuit draperAdderCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            4U,
            "The two-bit Draper adder requires four qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        circuit.addSingleQubitGate("X", gates::xGate(), 1U);
        circuit.addSingleQubitGate("X", gates::xGate(), 2U);

        appendForwardQft(circuit, 2U, 2U);

        for (const auto [control, target, angle] : {
            std::tuple{0U, 3U, std::numbers::pi},
            std::tuple{1U, 2U, std::numbers::pi},
            std::tuple{1U, 3U, std::numbers::pi / 2.0}
        }) {
            circuit.addTwoQubitGate(
                "CP",
                gates::controlledPhaseGate(angle),
                control,
                target,
                angle
            );
        }

        appendInverseQft(circuit, 2U, 2U);
        return circuit;
    }

    circuit::QuantumCircuit iqpCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            2U,
            "An IQP circuit requires at least two qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        for (std::size_t qubit = 0U; qubit < qubitCount; ++qubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                qubit
            );
        }

        for (std::size_t qubit = 0U; qubit < qubitCount; ++qubit) {
            const double angle =
                    (
                        0.11 +
                        0.07 *
                        static_cast<double>(qubit)
                    ) *
                    std::numbers::pi;

            circuit.addSingleQubitGate(
                "Rz",
                gates::rzGate(angle),
                qubit,
                angle
            );
        }

        for (std::size_t qubit = 0U; qubit + 1U < qubitCount; ++qubit) {
            const double angle =
                    (
                        0.23 +
                        0.04 *
                        static_cast<double>(qubit)
                    ) *
                    std::numbers::pi;

            circuit.addTwoQubitGate(
                "RZZ",
                gates::rzzGate(angle),
                qubit,
                qubit + 1U,
                angle
            );
        }

        if (qubitCount > 2U) {
            constexpr double closingAngle =
                    0.31 *
                    std::numbers::pi;

            circuit.addTwoQubitGate(
                "RZZ",
                gates::rzzGate(closingAngle),
                qubitCount - 1U,
                0U,
                closingAngle
            );
        }

        for (std::size_t qubit = 0U; qubit < qubitCount; ++qubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                qubit
            );
        }

        return circuit;
    }

    circuit::QuantumCircuit surfaceCodeStabilizerCircuit(
        const std::size_t qubitCount
    ) {
        requireMinimumQubitCount(
            qubitCount,
            10U,
            "The surface-code stabilizer demonstration requires ten qubits."
        );

        circuit::QuantumCircuit circuit{qubitCount};

        for (std::size_t dataQubit = 0U; dataQubit < 9U; ++dataQubit) {
            circuit.addSingleQubitGate(
                "H",
                gates::hadamardGate(),
                dataQubit
            );
        }

        constexpr std::pair<std::size_t, std::size_t> patchEdges[]{
            {0U, 1U}, {1U, 2U},
            {3U, 4U}, {4U, 5U},
            {6U, 7U}, {7U, 8U},
            {0U, 3U}, {3U, 6U},
            {1U, 4U}, {4U, 7U},
            {2U, 5U}, {5U, 8U}
        };

        for (const auto [first, second] : patchEdges) {
            circuit.addTwoQubitGate(
                "CZ",
                gates::czGate(),
                first,
                second
            );
        }

        circuit.addSingleQubitGate(
            "H",
            gates::hadamardGate(),
            9U
        );

        for (const std::size_t dataQubit : {1U, 3U, 5U, 7U}) {
            circuit.addTwoQubitGate(
                "CZ",
                gates::czGate(),
                9U,
                dataQubit
            );
        }

        circuit.addSingleQubitGate(
            "H",
            gates::hadamardGate(),
            9U
        );

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

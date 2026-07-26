#include "quantum_sim/gates/QuantumGates.hpp"

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <limits>

namespace quantum_sim::gates {
    math::ComplexMatrix xGate() {
        // X swaps the |0> and |1> amplitudes.
        return math::ComplexMatrix{
            2, 2,
            std::vector{
                math::Complex{},
                math::Complex{1.0, 0.0},
                math::Complex{1.0, 0.0},
                math::Complex{}
            }
        };
    }

    math::ComplexMatrix rxGate(const double angleRadians) {
        // Quantum rotations use half-angles in the matrix representation.
        const double cosine =
                std::cos(angleRadians / 2.0);

        const double sine =
                std::sin(angleRadians / 2.0);

        return math::ComplexMatrix{
            2, 2,
            std::vector{
                math::Complex{cosine, 0.0},
                math::Complex{0.0, -sine},
                math::Complex{0.0, -sine},
                math::Complex{cosine, 0.0},
            }
        };
    }

    math::ComplexMatrix yGate() {
        // Y is an X-like flip with +/-i phase on the swapped amplitudes.
        return math::ComplexMatrix{
            2, 2,
            std::vector{
                math::Complex{},
                math::Complex{0.0, -1.0},
                math::Complex{0.0, 1.0},
                math::Complex{},
            }
        };
    }

    math::ComplexMatrix ryGate(const double angleRadians) {
        // Ry is the real-valued rotation around the Bloch Y axis.
        const double cosine =
                std::cos(angleRadians / 2.0);

        const double sine =
                std::sin(angleRadians / 2.0);

        return math::ComplexMatrix{
            2, 2,
            std::vector{
                math::Complex{cosine, 0.0},
                math::Complex{-sine, 0.0},
                math::Complex{sine, 0.0},
                math::Complex{cosine, 0.0}
            }
        };
    }

    math::ComplexMatrix zGate() {
        return math::ComplexMatrix{
            2, 2,
            std::vector{
                math::Complex{1.0, 0.0},
                math::Complex{},
                math::Complex{},
                math::Complex{-1.0, 0.0},
            }
        };
    }

    math::ComplexMatrix rzGate(const double angleRadians) {
        // Rz is diagonal, so it changes phase without moving probability.
        const double cosine =
                std::cos(angleRadians / 2.0);

        const double sine =
                std::sin(angleRadians / 2.0);

        return math::ComplexMatrix{
            2,
            2,
            std::vector{
                math::Complex{cosine, -sine},
                math::Complex{},
                math::Complex{},
                math::Complex{cosine, sine}
            }
        };
    }

    math::ComplexMatrix sGate() {
        return math::ComplexMatrix{
            2, 2,
            std::vector{
                math::Complex{1.0, 0.0},
                math::Complex{},
                math::Complex{},
                math::Complex{0.0, 1.0},
            }
        };
    };

    math::ComplexMatrix tGate() {
        // T is the fourth root of Z: phase angle pi/4 on |1>.
        const double phase =
                std::numbers::pi / 4.0;

        return math::ComplexMatrix{
            2, 2,
            std::vector{
                math::Complex{1.0, 0.0},
                math::Complex{},
                math::Complex{},
                math::Complex{std::cos(phase), std::sin(phase)},
            }
        };
    };

    math::ComplexMatrix tDaggerGate() {
        // T-dagger reverses T with the conjugate phase exp(-i*pi/4).
        const double phase =
                -std::numbers::pi / 4.0;

        return math::ComplexMatrix{
            2, 2,
            std::vector{
                math::Complex{1.0, 0.0},
                math::Complex{},
                math::Complex{},
                math::Complex{std::cos(phase), std::sin(phase)},
            }
        };
    }

    math::ComplexMatrix hadamardGate() {
        // 1/sqrt(2) keeps the Hadamard columns normalized.
        double invSqrt2 = 1.0 / std::sqrt(2.0);
        return math::ComplexMatrix{
            2, 2,
            std::vector{
                math::Complex{invSqrt2, 0.0},
                math::Complex{invSqrt2, 0.0},
                math::Complex{invSqrt2, 0.0},
                math::Complex{-invSqrt2, 0.0}
            }
        };
    }

    math::ComplexMatrix cxGate() {
        return math::ComplexMatrix{
            4, 4,
            std::vector{
                math::Complex{1.0, 0.0},
                math::Complex{},
                math::Complex{},
                math::Complex{},

                math::Complex{},
                math::Complex{1.0, 0.0},
                math::Complex{},
                math::Complex{},

                math::Complex{},
                math::Complex{},
                math::Complex{},
                math::Complex{1.0, 0.0},

                math::Complex{},
                math::Complex{},
                math::Complex{1.0, 0.0},
                math::Complex{},
            }
        };
    }

    math::ComplexMatrix cyGate() {
        return math::ComplexMatrix{
            4, 4,
            std::vector{
                math::Complex{1.0, 0.0},
                math::Complex{},
                math::Complex{},
                math::Complex{},

                math::Complex{},
                math::Complex{1.0, 0.0},
                math::Complex{},
                math::Complex{},

                math::Complex{},
                math::Complex{},
                math::Complex{},
                math::Complex{0.0, -1.0},

                math::Complex{},
                math::Complex{},
                math::Complex{0.0, 1.0},
                math::Complex{}
            }
        };
    }

    math::ComplexMatrix czGate() {
        return math::ComplexMatrix{
            4, 4,
            std::vector{
                math::Complex{1.0, 0.0},
                math::Complex{},
                math::Complex{},
                math::Complex{},

                math::Complex{},
                math::Complex{1.0, 0.0},
                math::Complex{},
                math::Complex{},

                math::Complex{},
                math::Complex{},
                math::Complex{1.0, 0.0},
                math::Complex{},

                math::Complex{},
                math::Complex{},
                math::Complex{},
                math::Complex{-1.0, 0.0}
            }
        };
    }

    math::ComplexMatrix swapGate() {
        return math::ComplexMatrix{
            4, 4,
            std::vector{
                math::Complex{1.0, 0.0},
                math::Complex{},
                math::Complex{},
                math::Complex{},

                math::Complex{},
                math::Complex{},
                math::Complex{1.0, 0.0},
                math::Complex{},

                math::Complex{},
                math::Complex{1.0, 0.0},
                math::Complex{},
                math::Complex{},

                math::Complex{},
                math::Complex{},
                math::Complex{},
                math::Complex{1.0, 0.0}
            }
        };
    }

    math::ComplexMatrix swapGate(std::size_t qubitCount, std::size_t firstQubit, std::size_t secondQubit) {
        if (qubitCount == 0) {
            throw std::invalid_argument{"A SWAP gate requires at least one qubit."};
        }

        if (qubitCount >= std::numeric_limits<std::size_t>::digits) {
            throw std::invalid_argument{"Qubit count is too large."};
        }

        if (firstQubit >= qubitCount ||
            secondQubit >= qubitCount) {
            throw std::out_of_range{"SWAP qubit index is outside the register."};
        }

        if (firstQubit == secondQubit) {
            throw std::invalid_argument{"SWAP qubits must be different."};
        }

        const std::size_t stateCount =
                std::size_t{1} << qubitCount;

        // Full-register gates are represented as permutation matrices.
        std::vector values(
            stateCount * stateCount,
            math::Complex{}
        );

        const std::size_t firstMask =
                std::size_t{1}
                << (qubitCount - 1 - firstQubit);

        const std::size_t secondMask =
                std::size_t{1}
                << (qubitCount - 1 - secondQubit);

        // For each input basis state, compute the output basis state after
        // exchanging the two selected bits.
        for (std::size_t inputState{}; inputState < stateCount; ++inputState) {
            const bool firstBit =
                    (inputState & firstMask) != 0;

            const bool secondBit =
                    (inputState & secondMask) != 0;

            std::size_t outputState =
                    inputState;

            if (firstBit != secondBit) {
                outputState ^= firstMask | secondMask;
            }

            values[outputState * stateCount + inputState] = math::Complex{1.0, 0.0};
        }

        return math::ComplexMatrix{
            stateCount,
            stateCount,
            std::move(values)
        };
    }

    math::ComplexMatrix cxGate(std::size_t qubitCount, std::size_t controlQubit, std::size_t targetQubit) {
        if (qubitCount == 0) {
            throw std::invalid_argument{"A CX gate requires at least one qubit."};
        }

        if (qubitCount >= std::numeric_limits<std::size_t>::digits) {
            throw std::invalid_argument{"Qubit count is too large."};
        }

        if (controlQubit >= qubitCount || targetQubit >= qubitCount) {
            throw std::out_of_range{"CX qubit index is outside the register."};
        }

        if (controlQubit == targetQubit) {
            throw std::invalid_argument{"CX control and target qubits must be different."};
        }

        const std::size_t stateCount =
                std::size_t{1} << qubitCount;

        std::vector<math::Complex> values{
            stateCount * stateCount, math::Complex{}
        };

        const std::size_t controlMask =
                std::size_t{1} << (qubitCount - 1 - controlQubit);

        const std::size_t targetMask =
                std::size_t{1} << (qubitCount - 1 - targetQubit);

        // Controlled-X is a permutation: flip the target bit only when control is 1.
        for (std::size_t inputState = 0; inputState < stateCount; ++inputState) {
            std::size_t outputState = inputState;

            const bool controlIsOne = (inputState & controlMask) != 0;

            if (controlIsOne) {
                outputState ^= targetMask;
            }

            values[outputState * stateCount + inputState] = math::Complex{1.0, 0.0};
        }

        return math::ComplexMatrix{stateCount, stateCount, std::move(values)};
    }

    math::ComplexMatrix cyGate(std::size_t qubitCount, std::size_t controlQubit, std::size_t targetQubit) {
        if (qubitCount == 0) {
            throw std::invalid_argument{"A CY gate requires at least one qubit."};
        }

        if (qubitCount >= std::numeric_limits<std::size_t>::digits) {
            throw std::invalid_argument{"Qubit count is too large."};
        }

        if (controlQubit >= qubitCount ||
            targetQubit >= qubitCount) {
            throw std::out_of_range{"CY qubit index is outside the register."};
        }

        if (controlQubit == targetQubit) {
            throw std::invalid_argument{"CY control and target qubits must be different."};
        }

        const std::size_t stateCount =
                std::size_t{1} << qubitCount;

        std::vector values{
            stateCount * stateCount,
            math::Complex{}
        };

        const std::size_t controlMask =
                std::size_t{1}
                << (qubitCount - 1 - controlQubit);

        const std::size_t targetMask =
                std::size_t{1}
                << (qubitCount - 1 - targetQubit);

        // Controlled-Y flips the target like CX and adds a phase based on the
        // target's original value.
        for (std::size_t inputState = 0; inputState < stateCount; ++inputState) {
            const bool controlIsOne =
                    (inputState & controlMask) != 0;

            if (!controlIsOne) {
                values[
                    inputState * stateCount + inputState
                ] = math::Complex{1.0, 0.0};

                continue;
            }

            const bool targetIsOne =
                    (inputState & targetMask) != 0;

            const std::size_t outputState =
                    inputState ^ targetMask;

            const math::Complex phase =
                    targetIsOne
                        ? math::Complex{0.0, -1.0}
                        : math::Complex{0.0, 1.0};

            values[
                outputState * stateCount + inputState
            ] = phase;
        }

        return math::ComplexMatrix{
            stateCount,
            stateCount,
            std::move(values)
        };
    };

    math::ComplexMatrix czGate(std::size_t qubitCount, std::size_t controlQubit, std::size_t targetQubit) {
        if (qubitCount == 0) {
            throw std::invalid_argument{
                "A CZ gate requires at least one qubit."
            };
        }

        if (qubitCount >= std::numeric_limits<std::size_t>::digits) {
            throw std::invalid_argument{
                "Qubit count is too large."
            };
        }

        if (
            controlQubit >= qubitCount ||
            targetQubit >= qubitCount
        ) {
            throw std::out_of_range{
                "CZ qubit index is outside the register."
            };
        }

        if (controlQubit == targetQubit) {
            throw std::invalid_argument{
                "CZ control and target qubits must be different."
            };
        }

        const std::size_t stateCount =
                std::size_t{1} << qubitCount;

        std::vector<math::Complex> values(
            stateCount * stateCount,
            math::Complex{}
        );

        const std::size_t controlMask =
                std::size_t{1}
                << (qubitCount - 1 - controlQubit);

        const std::size_t targetMask =
                std::size_t{1}
                << (qubitCount - 1 - targetQubit);

        // Controlled-Z is diagonal: only |control=1,target=1> gets a -1 phase.
        for (
            std::size_t state = 0;
            state < stateCount;
            ++state
        ) {
            const bool controlIsOne =
                    (state & controlMask) != 0;

            const bool targetIsOne =
                    (state & targetMask) != 0;

            const double phase =
                    controlIsOne && targetIsOne
                        ? -1.0
                        : 1.0;

            values[
                state * stateCount + state
            ] = math::Complex{phase, 0.0};
        }

        return math::ComplexMatrix{
            stateCount,
            stateCount,
            std::move(values)
        };
    }

    math::ComplexMatrix iSwapGate() {
        return math::ComplexMatrix{
            4, 4,
            std::vector{
                math::Complex{1.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},

                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{0.0, 1.0},
                math::Complex{0.0, 0.0},

                math::Complex{0.0, 0.0},
                math::Complex{0.0, 1.0},
                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},

                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{1.0, 0.0}
            }
        };
    }

    math::ComplexMatrix iSwapGate(std::size_t qubitCount, std::size_t firstQubit,
                                  std::size_t secondQubit) {
        if (qubitCount == 0) {
            throw std::invalid_argument{
                "An iSWAP gate requires at least one qubit."
            };
        }

        if (qubitCount >= std::numeric_limits<std::size_t>::digits) {
            throw std::invalid_argument{
                "Qubit count is too large."
            };
        }

        if (
            firstQubit >= qubitCount ||
            secondQubit >= qubitCount
        ) {
            throw std::out_of_range{
                "iSWAP qubit index is outside the register."
            };
        }

        if (firstQubit == secondQubit) {
            throw std::invalid_argument{
                "iSWAP qubits must be different."
            };
        }

        const std::size_t stateCount =
                std::size_t{1} << qubitCount;

        std::vector<math::Complex> values(
            stateCount * stateCount,
            math::Complex{}
        );

        const std::size_t firstMask =
                std::size_t{1}
                << (qubitCount - 1 - firstQubit);

        const std::size_t secondMask =
                std::size_t{1}
                << (qubitCount - 1 - secondQubit);

        // iSWAP exchanges the selected bits and adds i to the exchanged states.
        for (
            std::size_t inputState = 0;
            inputState < stateCount;
            ++inputState
        ) {
            const bool firstBit =
                    (inputState & firstMask) != 0;

            const bool secondBit =
                    (inputState & secondMask) != 0;

            std::size_t outputState = inputState;

            math::Complex amplitude{1.0, 0.0};

            if (firstBit != secondBit) {
                outputState ^= firstMask | secondMask;
                amplitude = math::Complex{0.0, 1.0};
            }

            values[
                outputState * stateCount + inputState
            ] = amplitude;
        }

        return math::ComplexMatrix{
            stateCount,
            stateCount,
            std::move(values)
        };
    }
}

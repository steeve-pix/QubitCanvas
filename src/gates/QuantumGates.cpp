#include "quantum_sim/gates/QuantumGates.hpp"

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <limits>

namespace quantum_sim::gates {
    math::ComplexMatrix xGate() {
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
        double pi_value = std::numbers::pi;

        return math::ComplexMatrix{
            2, 2,
            std::vector{
                math::Complex{1.0, 0.0},
                math::Complex{},
                math::Complex{},
                math::Complex{std::cos(pi_value / 4), std::sin(pi_value / 4)},
            }
        };
    };

    math::ComplexMatrix hadamardGate() {
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

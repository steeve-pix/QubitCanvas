#include "quantum_sim/gates/QuantumGates.hpp"

#include <cmath>
#include <numbers>
#include <stdexcept>

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

    math::ComplexMatrix yGate() {
        return math::ComplexMatrix{
            2, 2,
            std::vector{
                math::Complex{},
                math::Complex{0.0, 1.0},
                math::Complex{0.0, -1.0},
                math::Complex{},
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

    math::ComplexMatrix cnotGate() {
        return math::ComplexMatrix{
            4, 4,
            std::vector{
                math::Complex{1.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},

                math::Complex{0.0, 0.0},
                math::Complex{1.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},

                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{1.0, 0.0},

                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{1.0, 0.0},
                math::Complex{0.0, 0.0},
            }
        };
    }

    math::ComplexMatrix swapGate() {
        return math::ComplexMatrix{
            4, 4,
            std::vector{
                math::Complex{1.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},

                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{1.0, 0.0},
                math::Complex{0.0, 0.0},

                math::Complex{0.0, 0.0},
                math::Complex{1.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},

                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{0.0, 0.0},
                math::Complex{1.0, 0.0}
            }
        };
    }

    math::ComplexMatrix cnotGate(std::size_t qubitCount, std::size_t controlQubit, std::size_t targetQubit) {
        if (qubitCount == 0) {
            throw std::invalid_argument{"A CNOT gate requires at least one qubit."};
        }

        if (qubitCount >= std::numeric_limits<std::size_t>::digits) {
            throw std::invalid_argument{"Qubit count is too large."};
        }

        if (controlQubit >= qubitCount || targetQubit >= qubitCount) {
            throw std::out_of_range{"CNOT qubit index is outside the register."};
        }

        if (controlQubit == targetQubit) {
            throw std::invalid_argument{"CNOT control and target qubits must be different."};
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
    };
}

#include "quantum_sim/gates/QuantumGates.hpp"

#include <cmath>
#include <numbers>

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
}

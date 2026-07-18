#include "quantum_sim/gates/SingleQubitGates.hpp"

#include <cmath>

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

    math::ComplexMatrix yGate() {
        return math::ComplexMatrix{
            2, 2,
            std::vector{
                math::Complex{},
                math::Complex{0.0,1.0},
                math::Complex{0.0,-1.0},
                math::Complex{},
            }
        };
    }
}

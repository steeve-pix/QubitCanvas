#include "quantum_sim/gates/SingleQubitGates.hpp"

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
}

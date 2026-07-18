#pragma once
#include "../math/ComplexMatrix.hpp"

namespace quantum_sim::gates {
    [[nodiscard]] math::ComplexMatrix xGate();

    [[nodiscard]] math::ComplexMatrix hadamardGate();

    [[nodiscard]] math::ComplexMatrix zGate();

    [[nodiscard]] math::ComplexMatrix yGate();
}

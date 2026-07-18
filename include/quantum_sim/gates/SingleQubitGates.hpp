#pragma once
#include "../math/ComplexMatrix.hpp"

namespace quantum_sim::gates {
    [[nodiscard]] math::ComplexMatrix xGate();

    [[nodiscard]] math::ComplexMatrix yGate();

    [[nodiscard]] math::ComplexMatrix zGate();

    [[nodiscard]] math::ComplexMatrix sGate();

    [[nodiscard]] math::ComplexMatrix tGate();

    [[nodiscard]] math::ComplexMatrix hadamardGate();
}

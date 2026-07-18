#pragma once

#include "quantum_sim/math/ComplexVector.hpp"

#include<cstddef>
#include <vector>

namespace quantum_sim::quantum {
    class QuantumRegister final {
    public:
        QuantumRegister(std::size_t qubitCount, math::ComplexVector amplitudes);

        [[nodiscard]] std::size_t qubitCount() const noexcept;

        [[nodiscard]] std::size_t stateCount() const noexcept;

        [[nodiscard]] const math::Complex &amplitude(std::size_t stateIndex) const;

    private:
        std::size_t qubitCount_;
        math::ComplexVector amplitudes_;
    };
}

#pragma once

#include "../math/ComplexVector.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"

namespace quantum_sim::quantum {
    class Quibit final {
    public:
        Quibit(math::Complex alpha, math::Complex beta);

        [[nodiscard]] const math::Complex &zeroAmplitude() const noexcept;

        [[nodiscard]] const math::Complex &oneAmplitude() const noexcept;

        [[nodiscard]] Quibit apply(const math::ComplexMatrix &gate) const;

        [[nodiscard]] double probabilityOfZero() const noexcept;

        [[nodiscard]] double probabilityOfOne() const noexcept;

    private:
        math::Complex alpha_;
        math::Complex beta_;
    };
}

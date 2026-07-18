#pragma once

#include "../math/ComplexVector.hpp"

namespace quantum_sim::quantum {
    class Quibit final {
    public:
        Quibit(math::Complex alpha, math::Complex beta);

        [[nodiscard]] const math::Complex &zeroAmplitude() const noexcept;

        [[nodiscard]] const math::Complex &oneAmplitude() const noexcept;

    private:
        math::Complex alpha_;
        math::Complex beta_;
    };
}

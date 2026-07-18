#pragma once

#include "quantum_sim/math/ComplexMatrix.hpp"
#include <random>

namespace quantum_sim::quantum {
    enum class MeasurementResult {
        Zero,
        One
    };

    class Qubit final {
    public:
        Qubit(math::Complex alpha, math::Complex beta);

        [[nodiscard]] const math::Complex &zeroAmplitude() const noexcept;

        [[nodiscard]] const math::Complex &oneAmplitude() const noexcept;

        [[nodiscard]] Qubit apply(const math::ComplexMatrix &gate) const;

        [[nodiscard]] double probabilityOfZero() const noexcept;

        [[nodiscard]] double probabilityOfOne() const noexcept;

        [[nodiscard]] MeasurementResult measure(std::mt19937& randomEngine);

    private:
        math::Complex alpha_;
        math::Complex beta_;
    };
}

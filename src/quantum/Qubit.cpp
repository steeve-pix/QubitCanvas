#include "quantum_sim/quantum/Qubit.hpp"

#include <cmath>
#include <stdexcept>

namespace quantum_sim::quantum {
    Quibit::Quibit(math::Complex alpha, math::Complex beta)
        : alpha_(alpha), beta_(beta) {
        constexpr double epsilon = 1e-9;

        const double totalProbability =
                alpha_.magnitudeSquared() + beta_.magnitudeSquared();

        if (std::abs(totalProbability - 1.0) >= epsilon) {
            throw std::invalid_argument{"Qubit amplitudes must be normalized."};
        }
    }

    const math::Complex &Quibit::zeroAmplitude() const noexcept {
        return alpha_;
    }

    const math::Complex &Quibit::oneAmplitude() const noexcept {
        return beta_;
    }
}

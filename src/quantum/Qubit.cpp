#include "quantum_sim/quantum/Qubit.hpp"

#include <cmath>
#include <vector>
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

    Quibit Quibit::apply(const math::ComplexMatrix &gate) const {
        if (gate.rows() != 2 || gate.columns() != 2)
            throw std::invalid_argument{
                "A single-qubit gate must be a 2 by 2 matrix."
            };

        if (!gate.isUnitary())
            throw std::invalid_argument{
                "A quantum gate must be unitary."
            };

        const math::ComplexVector state{
            std::vector{alpha_, beta_}
        };

        const math::ComplexVector transformed = gate * state;

        return Quibit{
            transformed.at(0),
            transformed.at(1)
        };
    }

    double Quibit::probabilityOfZero() const noexcept {
        return alpha_.magnitudeSquared();
    }

    double Quibit::probabilityOfOne() const noexcept {
        return beta_.magnitudeSquared();
    }
}

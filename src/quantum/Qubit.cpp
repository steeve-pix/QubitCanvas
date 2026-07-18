#include "quantum_sim/quantum/Qubit.hpp"

#include <cmath>
#include <vector>
#include <stdexcept>

namespace quantum_sim::quantum {
    Qubit::Qubit(math::Complex alpha, math::Complex beta)
        : alpha_(alpha), beta_(beta) {
        constexpr double epsilon = 1e-9;

        const double totalProbability =
                alpha_.magnitudeSquared() + beta_.magnitudeSquared();

        if (std::abs(totalProbability - 1.0) >= epsilon) {
            throw std::invalid_argument{"Qubit amplitudes must be normalized."};
        }
    }

    const math::Complex &Qubit::zeroAmplitude() const noexcept {
        return alpha_;
    }

    const math::Complex &Qubit::oneAmplitude() const noexcept {
        return beta_;
    }

    Qubit Qubit::apply(const math::ComplexMatrix &gate) const {
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

        return Qubit{
            transformed.at(0),
            transformed.at(1)
        };
    }

    double Qubit::probabilityOfZero() const noexcept {
        return alpha_.magnitudeSquared();
    }

    double Qubit::probabilityOfOne() const noexcept {
        return beta_.magnitudeSquared();
    }

    MeasurementResult Qubit::measure(std::mt19937 &randomEngine) {
        std::uniform_real_distribution distribution{0.0, 1.0};
        const double sample = distribution(randomEngine);

        if (sample < probabilityOfZero()) {
            alpha_ = math::Complex{1.0, 0.0};
            beta_ = math::Complex{};
            return MeasurementResult::Zero;
        }
        alpha_ = math::Complex{};
        beta_ = math::Complex{1.0, 0.0};
        return MeasurementResult::One;
    }
}

#include "quantum_sim/analysis/StateMetrics.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace quantum_sim::analysis {
    namespace {
        double entropyTerm(const double eigenvalue) noexcept {
            if (eigenvalue <= 1e-15) {
                return 0.0;
            }

            return -eigenvalue * std::log2(eigenvalue);
        }
    }

    double StateMetrics::fidelity(
        const quantum::QuantumRegister &left,
        const quantum::QuantumRegister &right
    ) {
        if (
            left.qubitCount() != right.qubitCount() ||
            left.stateCount() != right.stateCount()
        ) {
            throw std::invalid_argument{
                "Fidelity requires registers with matching dimensions."
            };
        }

        double overlapReal{};
        double overlapImaginary{};

        for (
            std::size_t stateIndex = 0U;
            stateIndex < left.stateCount();
            ++stateIndex
        ) {
            const auto &leftAmplitude =
                    left.amplitude(stateIndex);

            const auto &rightAmplitude =
                    right.amplitude(stateIndex);

            // conj(left) * right, expanded to avoid temporary complex objects.
            overlapReal +=
                    leftAmplitude.real() *
                    rightAmplitude.real() +
                    leftAmplitude.imaginary() *
                    rightAmplitude.imaginary();

            overlapImaginary +=
                    leftAmplitude.real() *
                    rightAmplitude.imaginary() -
                    leftAmplitude.imaginary() *
                    rightAmplitude.real();
        }

        return std::clamp(
            overlapReal * overlapReal +
            overlapImaginary * overlapImaginary,
            0.0,
            1.0
        );
    }

    QubitMetrics StateMetrics::forQubit(
        const quantum::QuantumRegister &state,
        const std::size_t qubit
    ) {
        if (qubit >= state.qubitCount()) {
            throw std::out_of_range{
                "State metric qubit is outside the register."
            };
        }

        const std::size_t bitPosition =
                state.qubitCount() - 1U - qubit;

        const std::size_t mask =
                std::size_t{1} << bitPosition;

        double probabilityZero{};
        double probabilityOne{};
        double coherenceReal{};
        double coherenceImaginary{};

        for (
            std::size_t stateIndex = 0U;
            stateIndex < state.stateCount();
            ++stateIndex
        ) {
            if ((stateIndex & mask) != 0U) {
                continue;
            }

            const std::size_t pairedIndex =
                    stateIndex | mask;

            const auto &zeroAmplitude =
                    state.amplitude(stateIndex);

            const auto &oneAmplitude =
                    state.amplitude(pairedIndex);

            probabilityZero +=
                    zeroAmplitude.magnitudeSquared();

            probabilityOne +=
                    oneAmplitude.magnitudeSquared();

            // rho(0,1) = sum a0 * conj(a1) over the traced-out basis.
            coherenceReal +=
                    zeroAmplitude.real() *
                    oneAmplitude.real() +
                    zeroAmplitude.imaginary() *
                    oneAmplitude.imaginary();

            coherenceImaginary +=
                    zeroAmplitude.imaginary() *
                    oneAmplitude.real() -
                    zeroAmplitude.real() *
                    oneAmplitude.imaginary();
        }

        const double coherenceSquared =
                coherenceReal * coherenceReal +
                coherenceImaginary * coherenceImaginary;

        const double purity =
                std::clamp(
                    probabilityZero * probabilityZero +
                    probabilityOne * probabilityOne +
                    2.0 * coherenceSquared,
                    0.0,
                    1.0
                );

        const double blochLength =
                std::clamp(
                    std::sqrt(
                        std::max(
                            0.0,
                            2.0 * purity - 1.0
                        )
                    ),
                    0.0,
                    1.0
                );

        const double largerEigenvalue =
                std::clamp(
                    (1.0 + blochLength) * 0.5,
                    0.0,
                    1.0
                );

        const double smallerEigenvalue =
                1.0 - largerEigenvalue;

        return QubitMetrics{
            qubit,
            probabilityZero,
            probabilityOne,
            std::sqrt(coherenceSquared),
            purity,
            entropyTerm(largerEigenvalue) +
                entropyTerm(smallerEigenvalue),
            blochLength
        };
    }

    std::vector<QubitMetrics> StateMetrics::forRegister(
        const quantum::QuantumRegister &state
    ) {
        std::vector<QubitMetrics> metrics;
        metrics.reserve(state.qubitCount());

        for (
            std::size_t qubit = 0U;
            qubit < state.qubitCount();
            ++qubit
        ) {
            metrics.push_back(
                forQubit(state, qubit)
            );
        }

        return metrics;
    }
}

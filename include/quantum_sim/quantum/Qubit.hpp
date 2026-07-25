#pragma once

#include "quantum_sim/math/ComplexMatrix.hpp"
#include <random>

namespace quantum_sim::quantum {
    /**
     * Computational-basis measurement result for a qubit.
     */
    enum class MeasurementResult {
        Zero,
        One
    };

    /**
     * Single normalized qubit represented as alpha|0> + beta|1>.
     */
    class Qubit final {
    public:
        /**
         * Creates a qubit from two amplitudes.
         *
         * @param alpha Amplitude for |0>.
         * @param beta Amplitude for |1>.
         * @throws std::invalid_argument if alpha and beta are not normalized.
         */
        Qubit(math::Complex alpha, math::Complex beta);

        /**
         * @return Amplitude for basis state |0>.
         */
        [[nodiscard]] const math::Complex &zeroAmplitude() const noexcept;

        /**
         * @return Amplitude for basis state |1>.
         */
        [[nodiscard]] const math::Complex &oneAmplitude() const noexcept;

        /**
         * Applies a 2x2 unitary gate and returns the transformed qubit.
         *
         * @param gate Single-qubit unitary matrix.
         * @return New qubit after applying the gate.
         * @throws std::invalid_argument if gate is not 2x2 or is not unitary.
         */
        [[nodiscard]] Qubit apply(const math::ComplexMatrix &gate) const;

        /**
         * @return Probability of measuring |0>.
         */
        [[nodiscard]] double probabilityOfZero() const noexcept;

        /**
         * @return Probability of measuring |1>.
         */
        [[nodiscard]] double probabilityOfOne() const noexcept;

        /**
         * Samples a computational-basis measurement.
         *
         * @param randomEngine Random engine used for probability sampling.
         * @return The sampled measurement result.
         */
        [[nodiscard]] MeasurementResult measure(std::mt19937 &randomEngine);

    private:
        math::Complex alpha_;
        math::Complex beta_;
    };
}

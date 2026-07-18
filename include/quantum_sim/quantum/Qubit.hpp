#pragma once

#include "quantum_sim/math/ComplexMatrix.hpp"
#include <random>

namespace quantum_sim::quantum {
    /**
     * @brief Represents the result of measuring a quantum bit (qubit).
     *
     * MeasurementResult is an enumeration that defines the two possible outcomes
     * of a qubit measurement in the computational basis: Zero and One. These
     * correspond to the classical states |0⟩ and |1⟩, respectively, resulting
     * from the collapse of the superposition state during the measurement process.
     */
    enum class MeasurementResult {
        Zero,
        One
    };

    /**
     * @brief Represents a quantum bit (qubit) capable of being in a superposition of states.
     *
     * The Qubit class encapsulates the quantum state of a single qubit, represented as a superposition
     * of the computational basis states |0⟩ and |1⟩, with complex amplitudes α (zeroAmplitude) and β (oneAmplitude).
     * It provides methods for retrieving amplitudes, computing probabilities, applying operations, and performing measurements.
     */
    class Qubit final {
        /**
         *
         */
    public:
        Qubit(math::Complex alpha, math::Complex beta);

        /**
         * @brief Retrieves the amplitude of the |0⟩ state (alpha) of the qubit.
         *
         * The amplitude represents the complex probability amplitude associated
         * with the qubit's |0⟩ state in its quantum superposition.
         *
         * @return A reference to the complex amplitude of the |0⟩ state.
         */
        [[nodiscard]] const math::Complex &zeroAmplitude() const noexcept;

        /**
         * @brief Retrieves the amplitude associated with the |1⟩ state of the qubit.
         *
         * The amplitude is a complex number that represents the contribution of the |1⟩ state
         * in the qubit's overall quantum state. This value, along with the amplitude of
         * the |0⟩ state, determines the state vector of the qubit.
         *
         * @return A constant reference to a Complex object representing the |1⟩ amplitude.
         */
        [[nodiscard]] const math::Complex &oneAmplitude() const noexcept;

        /**
         * @brief Applies a specified transformation or operation.
         *
         * @param operation The operation to be applied.
         * @param target The target on which the operation is applied.
         * @return The result of applying the operation.
         */
        [[nodiscard]] Qubit apply(const math::ComplexMatrix &gate) const;

        /**
         * @brief Computes the probability of the qubit being in the |0⟩ state.
         *
         * This method calculates the probability as the square of the magnitude
         * of the amplitude associated with the |0⟩ state (alpha). The value returned
         * is a normalized probability in the range [0.0, 1.0].
         *
         * @return The probability of the qubit being in the |0⟩ state, as a double.
         *         The returned value is in the range [0.0, 1.0].
         */
        [[nodiscard]] double probabilityOfZero() const noexcept;

        /**
         * Computes the probability of the Qubit being in the |1⟩ state.
         *
         * The probability is calculated as the square of the magnitude of the
         * amplitude corresponding to the |1⟩ state.
         *
         * @return The probability of the Qubit being in the |1⟩ state, as a double.
         *         The returned value is in the range [0.0, 1.0].
         */
        [[nodiscard]] double probabilityOfOne() const noexcept;

        /**
         * Measures the state of the qubit, collapsing its quantum state into one of the classical
         * states (Zero or One) based on the probabilities of the respective outcomes.
         * The measurement operation is non-deterministic and uses a random number generator.
         *
         * @param randomEngine A reference to the random number generator used for probabilistic sampling.
         * @return The measurement result (MeasurementResult::Zero or MeasurementResult::One)
         *         based on the qubit's quantum state probabilities prior to measurement.
         */
        [[nodiscard]] MeasurementResult measure(std::mt19937 &randomEngine);

        /**
         *
         */
    private:
        math::Complex alpha_;
        /**
         * @brief Represents the complex amplitude of the |1⟩ state in a qubit's quantum state.
         *
         * A qubit's state is defined as a superposition of the basis states |0⟩ and |1⟩, expressed
         * mathematically as |ψ⟩ = α|0⟩ + β|1⟩, where β is a complex number that determines the
         * amplitude (magnitude and phase) of the |1⟩ state.
         *
         * This variable is initialized and managed internally within the Qubit class
         * to ensure proper quantum state representation and manipulation.
         */
        math::Complex beta_;
    };
}

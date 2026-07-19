#pragma once

#include "quantum_sim/math/ComplexVector.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"
#include "Qubit.hpp"

#include <cstddef>
#include <random>
#include <vector>


namespace quantum_sim::quantum {
    /**
     * Represents a quantum register composed of qubits, enabling the simulation and manipulation
     * of quantum states through operations such as quantum gates.
     */
    class QuantumRegister final {
        /**
         * Constructs a quantum register with the specified number of qubits and their corresponding amplitudes.
         *
         * @param qubitCount The number of qubits in the quantum register. Must be at least 1.
         *                   The maximum value is limited by the size of the system's representation.
         * @param amplitudes A vector of complex values representing the quantum state amplitudes.
         *                   The size of the vector must be equal to 2 raised to the power of the qubit count,
         *                   and the amplitudes must be normalized.
         * @throw std::invalid_argument Thrown if:
         *                              - qubitCount is 0.
         *                              - qubitCount exceeds the system's representational limits.
         *                              - The size of amplitudes does not match the expected state count.
         *                              - The amplitudes are not normalized.
         */
    public:
        QuantumRegister(std::size_t qubitCount, math::ComplexVector amplitudes);

        /**
         * Retrieves the number of qubits in the quantum register.
         *
         * @return The total number of qubits in the quantum register.
         */
        [[nodiscard]] std::size_t qubitCount() const noexcept;

        /**
         * Returns the total number of possible quantum states for the register,
         * which is determined by 2 raised to the power of the number of qubits.
         *
         * @return The total number of quantum states in the register.
         */
        [[nodiscard]] std::size_t stateCount() const noexcept;

        /**
         * Represents the amplitude of a quantum state, which is a complex value composed of real and imaginary parts.
         *
         * @param real The real part of the amplitude.
         * @param imaginary The imaginary part of the amplitude.
         * @return A complex value representing the amplitude of the quantum state.
         */
        [[nodiscard]] const math::Complex &amplitude(std::size_t stateIndex) const;

        /**
         * Applies a single-qubit quantum gate to the specified target qubit within the quantum register.
         *
         * @param gate A 2x2 unitary matrix representing the single-qubit quantum gate to be applied.
         *             The matrix must be unitary and of size 2x2.
         * @param targetQubit The index of the qubit within the register to which the gate will be applied.
         *                    Must be within the range [0, qubitCount - 1].
         * @return A new QuantumRegister object with the gate applied to the target qubit. The original register remains unchanged.
         * @throw std::invalid_argument Thrown if:
         *                              - The gate is not a 2x2 matrix.
         *                              - The gate is not unitary.
         * @throw std::out_of_range Thrown if the target qubit index is outside the valid range of the register.
         */
        [[nodiscard]] QuantumRegister applySingleQubitGate(const math::ComplexMatrix &gate,
                                                           std::size_t targetQubit) const;

        /**
         * Applies a quantum gate to the quantum register, resulting in a new quantum register
         * with the state modified according to the given gate's transformation. The gate must
         * be unitary and its dimensions must align with the state count of the quantum register.
         *
         * @param gate A unitary matrix representing the quantum gate to apply. Its dimensions
         * must match the state count of the quantum register.
         * @return A new QuantumRegister object representing the updated quantum state after
         * applying the quantum gate.
         * @throws std::invalid_argument If the gate dimensions do not match the state count
         * of the quantum register, or if the gate is not unitary.
         */
        [[nodiscard]] QuantumRegister applyGate(const math::ComplexMatrix &gate) const;

        /**
         * Calculates the probability of the quantum system being in a specific state
         * identified by the given state index. The probability is derived from the
         * squared magnitude of the amplitude corresponding to the state.
         *
         * @param stateIndex The index of the state for which the probability is to be calculated.
         * @return The probability of the quantum system being in the specified state.
         */
        [[nodiscard]] double probability(std::size_t stateIndex) const;

        /**
         * Measures the current quantum state, collapsing the superposition into a single basis state
         * with a probability proportional to the square of its amplitude, and updates the state to the
         * measured basis state.
         *
         * @param randomEngine A random number generator used to sample the measurement result based on
         *                     the probabilities of the quantum states.
         * @return The index of the basis state that was measured.
         */
        [[nodiscard]] std::size_t measure(std::mt19937 &randomEngine);

        /**
         * Calculates the probability that a specific qubit within the quantum register
         * collapses to the |1⟩ state when measured.
         *
         * @param qubitIndex The index of the qubit within the register for which the
         *                   probability of being in the |1⟩ state is computed. Must be within
         *                   the bounds of the register.
         * @return The probability that the specified qubit is in the |1⟩ state.
         * @throws std::out_of_range If the provided qubitIndex is outside the bounds of the
         *                           quantum register.
         */
        [[nodiscard]] double probabilityOfQubitOne(std::size_t qubitIndex) const;

        /**
         * Computes the probability of a specified qubit being in the |0⟩ state within the quantum register.
         *
         * @param qubitIndex The index of the qubit within the quantum register.
         * @return The probability of the qubit being in the |0⟩ state, represented as a double.
         */
        [[nodiscard]] double probabilityOfQubitZero(std::size_t qubitIndex) const;

        /**
         * Performs a measurement on the specified qubit, collapsing its state to either |0⟩ or |1⟩
         * and updating the quantum state accordingly.
         *
         * The measurement uses the probabilities of the qubit being in the |0⟩ or |1⟩ states, and it
         * samples from a uniform random distribution to determine the result.
         * After the measurement, the quantum state amplitudes are updated to reflect the measured collapse.
         *
         * @param qubitIndex The index of the qubit to be measured.
         * @param randomEngine A random engine used to perform the probabilistic measurement.
         * @return The result of the measurement, either MeasurementResult::Zero or MeasurementResult::One.
         */
        [[nodiscard]] MeasurementResult measureQubit(std::size_t qubitIndex, std::mt19937 &randomEngine);

        [[nodiscard]] static QuantumRegister basisState(std::size_t qubitCount, std::size_t stateIndex);

    private:
        /**
         * @brief Stores the number of qubits in the quantum register.
         *
         * This variable represents the total number of qubits that the quantum register
         * contains. It determines the dimensionality of the quantum state space and
         * directly impacts the number of quantum states that can be represented.
         *
         * @note The value must be at least 1. The maximum allowable value is constrained
         *       by system limits, as the state count grows exponentially with the number
         *       of qubits (2^qubitCount).
         *
         * @invariant The qubit count must remain consistent with the size of the quantum
         *            state representation across the lifetime of the quantum register.
         */
        std::size_t qubitCount_;
        /**
         * @brief Stores the quantum state amplitudes of the register.
         *
         * This variable represents the complex-valued amplitudes of
         * the quantum states in the quantum register. Each amplitude
         * corresponds to the probability amplitude of a specific basis
         * state in the quantum state superposition. The amplitudes
         * are stored in a vector of complex numbers, where the index
         * of each entry maps to the binary representation of the basis
         * state.
         *
         * @note The squared magnitudes of the amplitudes should sum to 1
         *       to satisfy the quantum mechanical constraint of
         *       normalization.
         */
        math::ComplexVector amplitudes_;

        /**
         * Determines whether the specified qubit in the given quantum state index has the state |1⟩.
         *
         * @param stateIndex The index representing the quantum state.
         * @param qubitIndex The index of the qubit to check, relative to the quantum register.
         * @return True if the qubit at the specified index has the state |1⟩, otherwise false.
         */
        [[nodiscard]] bool stateHasQubitOne(std::size_t stateIndex, std::size_t qubitIndex) const noexcept;
    };
}

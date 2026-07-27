#pragma once

#include "Qubit.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/math/ComplexVector.hpp"

#include <cstddef>
#include <random>
#include <string>
#include <vector>

namespace quantum_sim::quantum {
    /**
     * Display-ready data for one computational basis state.
     */
    struct StateInfo {
        std::string label;
        math::Complex amplitude;
        double probability;
    };

    /**
     * Cartesian Bloch-vector coordinates.
     */
    struct BlochVector {
        double x, y, z;
    };

    /**
     * Spherical Bloch coordinates in radians.
     */
    struct BlochAngles {
        double theta, phi;
    };

    /**
     * Full quantum register state stored as 2^n normalized amplitudes.
     */
    class QuantumRegister final {
    public:
        /**
         * Creates a register from a normalized amplitude vector.
         *
         * @param qubitCount Number of qubits. Must be at least 1.
         * @param amplitudes Exactly 2^qubitCount normalized amplitudes.
         * @throws std::invalid_argument if the qubit count, vector size, or normalization is invalid.
         */
        QuantumRegister(std::size_t qubitCount, math::ComplexVector amplitudes);

        /**
         * @return Number of qubits represented by this register.
         */
        [[nodiscard]] std::size_t qubitCount() const noexcept;

        /**
         * @return Number of computational basis states, equal to 2^qubitCount().
         */
        [[nodiscard]] std::size_t stateCount() const noexcept;

        /**
         * Reads a basis-state amplitude.
         *
         * @param stateIndex Basis-state index.
         * @return Amplitude for the indexed basis state.
         * @throws std::out_of_range if stateIndex is outside the register.
         */
        [[nodiscard]] const math::Complex &amplitude(std::size_t stateIndex) const;

        /**
         * Applies a 2x2 gate to one qubit by expanding it across the register.
         *
         * @param gate Single-qubit unitary gate.
         * @param targetQubit Qubit index using q0 as the most-significant bit.
         * @return New register after the transformation.
         * @throws std::invalid_argument if gate is not a valid single-qubit unitary.
         * @throws std::out_of_range if targetQubit is outside the register.
         */
        [[nodiscard]] QuantumRegister applySingleQubitGate(const math::ComplexMatrix &gate,
                                                           std::size_t targetQubit) const;

        /**
         * Applies a compact 4x4 gate to any two qubits without expanding it.
         *
         * Local matrix basis ordering is |00⟩, |01⟩, |10⟩, |11⟩, where the
         * first bit belongs to firstQubit and the second bit belongs to
         * secondQubit. The operation visits each amplitude exactly once, so
         * memory and execution scale with the state vector rather than with a
         * stateCount() by stateCount() matrix.
         *
         * @param gate Two-qubit unitary matrix.
         * @param firstQubit Qubit represented by the first local basis bit.
         * @param secondQubit Qubit represented by the second local basis bit.
         * @return New register after the transformation.
         * @throws std::invalid_argument if gate is not a 4x4 unitary or the
         * qubit indices are equal.
         * @throws std::out_of_range if either qubit is outside the register.
         */
        [[nodiscard]] QuantumRegister applyTwoQubitGate(
            const math::ComplexMatrix &gate,
            std::size_t firstQubit,
            std::size_t secondQubit
        ) const;

        /**
         * Applies a full-register unitary matrix.
         *
         * @param gate Matrix sized stateCount() by stateCount().
         * @return New register after the transformation.
         * @throws std::invalid_argument if gate dimensions or unitarity are invalid.
         */
        [[nodiscard]] QuantumRegister applyGate(const math::ComplexMatrix &gate) const;

        /**
         * @param stateIndex Basis-state index.
         * @return Measurement probability for the indexed basis state.
         * @throws std::out_of_range if stateIndex is outside the register.
         */
        [[nodiscard]] double probability(std::size_t stateIndex) const;

        /**
         * Measures the whole register and collapses it to one basis state.
         *
         * @param randomEngine Random engine used for probability sampling.
         * @return Measured basis-state index.
         */
        [[nodiscard]] std::size_t measure(std::mt19937 &randomEngine);

        /**
         * Computes the marginal probability that one qubit measures as |1⟩.
         *
         * @param qubitIndex Qubit index using q0 as the most-significant bit.
         * @return Probability of outcome |1⟩ for that qubit.
         * @throws std::out_of_range if qubitIndex is outside the register.
         */
        [[nodiscard]] double probabilityOfQubitOne(std::size_t qubitIndex) const;

        /**
         * Computes the marginal probability that one qubit measures as |0⟩.
         *
         * @param qubitIndex Qubit index using q0 as the most-significant bit.
         * @return Probability of outcome |0⟩ for that qubit.
         * @throws std::out_of_range if qubitIndex is outside the register.
         */
        [[nodiscard]] double probabilityOfQubitZero(std::size_t qubitIndex) const;

        /**
         * Measures a single qubit and renormalizes the surviving amplitudes.
         *
         * @param qubitIndex Qubit to measure.
         * @param randomEngine Random engine used for probability sampling.
         * @return Sampled qubit result.
         * @throws std::out_of_range if qubitIndex is outside the register.
         */
        [[nodiscard]] MeasurementResult measureQubit(std::size_t qubitIndex, std::mt19937 &randomEngine);

        /**
         * Creates a register initialized to one computational basis state.
         *
         * @param qubitCount Number of qubits.
         * @param stateIndex Basis state whose amplitude should be 1.
         * @return Register containing only the requested basis state.
         * @throws std::invalid_argument if qubitCount is invalid.
         * @throws std::out_of_range if stateIndex is outside the register size.
         */
        [[nodiscard]] static QuantumRegister basisState(std::size_t qubitCount, std::size_t stateIndex);

        /**
         * Formats a basis-state ket such as |0101⟩.
         *
         * @param stateIndex Basis-state index.
         * @return Label for the indexed state.
         * @throws std::out_of_range if stateIndex is outside the register.
         */
        [[nodiscard]] std::string basisStateLabel(std::size_t stateIndex) const;

        /**
         * Collects label, amplitude, and probability for one basis state.
         *
         * @param stateIndex Basis-state index.
         * @return StateInfo for UI and console views.
         * @throws std::out_of_range if stateIndex is outside the register.
         */
        [[nodiscard]] StateInfo stateInfo(std::size_t stateIndex) const;

        /**
         * @return StateInfo for every basis state in index order.
         */
        [[nodiscard]] std::vector<StateInfo> states() const;

        /**
         * Computes the Bloch vector for a single-qubit register.
         *
         * @return Bloch-vector coordinates.
         * @throws std::invalid_argument if this register does not contain exactly one qubit.
         */
        [[nodiscard]] BlochVector blockVector() const;

        /**
         * Computes theta and phi for a single-qubit register.
         *
         * @return Bloch-sphere angles in radians.
         * @throws std::invalid_argument if this register does not contain exactly one qubit.
         */
        [[nodiscard]] BlochAngles blochAngles() const;

    private:
        std::size_t qubitCount_;
        math::ComplexVector amplitudes_;

        /**
         * Checks one bit inside a basis-state index.
         *
         * @param stateIndex Basis-state index.
         * @param qubitIndex Qubit index using q0 as the most-significant bit.
         * @return True when the selected qubit bit is 1.
         */
        [[nodiscard]] bool stateHasQubitOne(std::size_t stateIndex, std::size_t qubitIndex) const noexcept;
    };
}

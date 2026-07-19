#pragma once

#include "quantum_sim/math/ComplexVector.hpp"

#include<cstddef>
#include <vector>

#include "quantum_sim/math/ComplexMatrix.hpp"

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
    };
}

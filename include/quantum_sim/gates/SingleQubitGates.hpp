#pragma once
#include "../math/ComplexMatrix.hpp"

namespace quantum_sim::gates {
    /**
     * Constructs the X (Pauli-X) gate for a single qubit in the form of a 2x2 complex matrix.
     * The X gate flips the state of a qubit. It maps |0⟩ to |1⟩ and |1⟩ to |0⟩.
     *
     * @return A 2x2 ComplexMatrix representing the Pauli-X gate.
     */
    [[nodiscard]] math::ComplexMatrix xGate();

    /**
     * Constructs the Y gate (Pauli-Y gate) matrix for single-qubit quantum operations.
     * The Y gate is represented as a 2x2 complex matrix.
     *
     * @return A ComplexMatrix representing the Y gate, with the following structure:
     *         [[0, i],
     *          [-i, 0]]
     */
    [[nodiscard]] math::ComplexMatrix yGate();

    /**
     * Constructs the Z (Pauli-Z) gate for a single qubit in the form of a 2x2 complex matrix.
     * The Z gate applies a phase flip to the state of a qubit. It maps |0⟩ to |0⟩ and |1⟩ to -|1⟩.
     *
     * @return A 2x2 ComplexMatrix representing the Pauli-Z gate.
     */
    [[nodiscard]] math::ComplexMatrix zGate();

    /**
     * Constructs the S gate matrix, which is a single-qubit quantum gate used in quantum computing.
     * The S gate applies a phase shift and is represented by a 2x2 matrix:
     *
     * [[1, 0],
     *  [0, i]],
     * where i is the imaginary unit.
     *
     * @return A ComplexMatrix representing the S gate matrix.
     */
    [[nodiscard]] math::ComplexMatrix sGate();

    /**
     * Constructs the T gate matrix used in quantum computing.
     * The T gate is a single-qubit quantum gate that corresponds
     * to a diagonal matrix with elements {1, exp(i * π / 4)}.
     *
     * @return A 2x2 complex matrix representing the T gate.
     */
    [[nodiscard]] math::ComplexMatrix tGate();

    /**
     * Constructs the Hadamard gate for a single qubit, representing
     * a quantum operation that puts the qubit into a superposition state.
     *
     * @return A representation of the Hadamard gate for a single qubit.
     */
    [[nodiscard]] math::ComplexMatrix hadamardGate();
}

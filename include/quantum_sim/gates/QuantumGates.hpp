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
     * Constructs the RX rotation gate for a single qubit as a 2x2 complex matrix.
     * The RX gate performs a counterclockwise rotation around the X-axis of the Bloch sphere
     * by an angle specified in radians.
     *
     * @param angleRadians The rotation angle in radians.
     * @return A 2x2 ComplexMatrix representing the RX rotation gate.
     */
    [[nodiscard]] math::ComplexMatrix rxGate(double angleRadians);

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
     * Constructs the RY (rotation around the Y axis) gate for a single qubit
     * as a 2x2 complex matrix. The RY gate applies a rotation by the specified angle
     * about the Y axis in the Bloch sphere representation of a qubit's state.
     *
     * @param angleRadians The rotation angle in radians.
     * @return A 2x2 ComplexMatrix representing the RY gate.
     */
    [[nodiscard]] math::ComplexMatrix ryGate(double angleRadians);

    /**
     * Constructs the Z (Pauli-Z) gate for a single qubit in the form of a 2x2 complex matrix.
     * The Z gate applies a phase flip to the state of a qubit. It maps |0⟩ to |0⟩ and |1⟩ to -|1⟩.
     *
     * @return A 2x2 ComplexMatrix representing the Pauli-Z gate.
     */
    [[nodiscard]] math::ComplexMatrix zGate();

    /**
     * Constructs the Rz (rotation around the Z-axis) gate for a single qubit in the form of a 2x2 complex matrix.
     * The Rz gate applies a phase shift dependent on the given angle in radians.
     *
     * @param angleRadians The rotation angle in radians. The phase shift is calculated as a function of this angle.
     * @return A 2x2 ComplexMatrix representing the Rz gate.
     */
    [[nodiscard]] math::ComplexMatrix rzGate(double angleRadians);

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

    /**
     * Constructs the CX (Controlled-NOT) gate as a 4x4 complex matrix.
     * The CX gate is a two-qubit quantum gate that flips the state of the target qubit
     * if and only if the control qubit is in the |1⟩ state.
     *
     * @return A 4x4 ComplexMatrix representing the CX gate.
     */
    [[nodiscard]] math::ComplexMatrix cxGate();

    [[nodiscard]] math::ComplexMatrix cyGate();

    [[nodiscard]] math::ComplexMatrix czGate();

    /**
     * Constructs the SWAP gate for two qubits in the form of a 4x4 complex matrix.
     * The SWAP gate exchanges the states of two qubits. It maps the basis states
     * |01⟩ to |10⟩ and |10⟩ to |01⟩, leaving |00⟩ and |11⟩ unchanged.
     *
     * @return A 4x4 ComplexMatrix representing the SWAP gate.
     */
    [[nodiscard]] math::ComplexMatrix swapGate();

    [[nodiscard]] math::ComplexMatrix swapGate(std::size_t qubitCount, std::size_t firstQubit, std::size_t secondQubit);

    [[nodiscard]] math::ComplexMatrix cxGate(std::size_t qubitCount, std::size_t controlQubit,
                                             std::size_t targetQubit);

    [[nodiscard]] math::ComplexMatrix cyGate(std::size_t qubitCount, std::size_t controlQubit,
                                             std::size_t targetQubit);

    [[nodiscard]] math::ComplexMatrix czGate(std::size_t qubitCount, std::size_t controlQubit,
                                             std::size_t targetQubit);

    [[nodiscard]] math::ComplexMatrix iSwapGate();

    [[nodiscard]] math::ComplexMatrix iSwapGate(std::size_t qubitCount, std::size_t controlQubit,
                                                        std::size_t targetQubit);
}

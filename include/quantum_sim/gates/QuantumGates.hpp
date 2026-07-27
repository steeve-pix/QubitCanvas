#pragma once
#include "quantum_sim/math/ComplexMatrix.hpp"

#include <cstddef>

namespace quantum_sim::gates {
    /**
     * @return Pauli-X single-qubit gate.
     */
    [[nodiscard]] math::ComplexMatrix xGate();

    /**
     * Creates an Rx rotation gate.
     *
     * @param angleRadians Rotation angle in radians.
     * @return 2x2 unitary rotation around the X axis.
     */
    [[nodiscard]] math::ComplexMatrix rxGate(double angleRadians);

    /**
     * @return Pauli-Y single-qubit gate.
     */
    [[nodiscard]] math::ComplexMatrix yGate();

    /**
     * Creates an Ry rotation gate.
     *
     * @param angleRadians Rotation angle in radians.
     * @return 2x2 unitary rotation around the Y axis.
     */
    [[nodiscard]] math::ComplexMatrix ryGate(double angleRadians);

    /**
     * @return Pauli-Z single-qubit gate.
     */
    [[nodiscard]] math::ComplexMatrix zGate();

    /**
     * Creates an Rz rotation gate.
     *
     * @param angleRadians Rotation angle in radians.
     * @return 2x2 unitary rotation around the Z axis.
     */
    [[nodiscard]] math::ComplexMatrix rzGate(double angleRadians);

    /**
     * @return S phase gate, diag(1, i).
     */
    [[nodiscard]] math::ComplexMatrix sGate();

    /**
     * @return Inverse S phase gate, diag(1, -i).
     */
    [[nodiscard]] math::ComplexMatrix sDaggerGate();

    /**
     * @return T phase gate, diag(1, exp(i*pi/4)).
     */
    [[nodiscard]] math::ComplexMatrix tGate();

    /**
     * @return Inverse T phase gate, diag(1, exp(-i*pi/4)).
     */
    [[nodiscard]] math::ComplexMatrix tDaggerGate();

    /**
     * @return Hadamard gate, which maps basis states into equal superposition.
     */
    [[nodiscard]] math::ComplexMatrix hadamardGate();

    /**
     * @return Two-qubit controlled-X matrix for adjacent two-qubit states.
     */
    [[nodiscard]] math::ComplexMatrix cxGate();

    /**
     * @return Two-qubit controlled-Y matrix for adjacent two-qubit states.
     */
    [[nodiscard]] math::ComplexMatrix cyGate();

    /**
     * @return Two-qubit controlled-Z matrix for adjacent two-qubit states.
     */
    [[nodiscard]] math::ComplexMatrix czGate();

    /**
     * @return Two-qubit SWAP matrix for adjacent two-qubit states.
     */
    [[nodiscard]] math::ComplexMatrix swapGate();

    /**
     * Creates a full-register SWAP matrix for any two qubits.
     *
     * @param qubitCount Register size.
     * @param firstQubit First qubit to exchange.
     * @param secondQubit Second qubit to exchange.
     * @return stateCount by stateCount unitary matrix.
     * @throws std::invalid_argument if the qubit count is invalid or both qubits are the same.
     * @throws std::out_of_range if either qubit index is outside the register.
     */
    [[nodiscard]] math::ComplexMatrix swapGate(std::size_t qubitCount, std::size_t firstQubit, std::size_t secondQubit);

    /**
     * Creates a full-register controlled-X matrix.
     *
     * @param qubitCount Register size.
     * @param controlQubit Control qubit index.
     * @param targetQubit Target qubit index.
     * @return stateCount by stateCount unitary matrix.
     * @throws std::invalid_argument if the qubit count is invalid or control equals target.
     * @throws std::out_of_range if either qubit index is outside the register.
     */
    [[nodiscard]] math::ComplexMatrix cxGate(std::size_t qubitCount, std::size_t controlQubit,
                                             std::size_t targetQubit);

    /**
     * Creates a full-register controlled-Y matrix.
     *
     * @param qubitCount Register size.
     * @param controlQubit Control qubit index.
     * @param targetQubit Target qubit index.
     * @return stateCount by stateCount unitary matrix.
     * @throws std::invalid_argument if the qubit count is invalid or control equals target.
     * @throws std::out_of_range if either qubit index is outside the register.
     */
    [[nodiscard]] math::ComplexMatrix cyGate(std::size_t qubitCount, std::size_t controlQubit,
                                             std::size_t targetQubit);

    /**
     * Creates a full-register controlled-Z matrix.
     *
     * @param qubitCount Register size.
     * @param controlQubit Control qubit index.
     * @param targetQubit Target qubit index.
     * @return stateCount by stateCount unitary matrix.
     * @throws std::invalid_argument if the qubit count is invalid or control equals target.
     * @throws std::out_of_range if either qubit index is outside the register.
     */
    [[nodiscard]] math::ComplexMatrix czGate(std::size_t qubitCount, std::size_t controlQubit,
                                             std::size_t targetQubit);

    /**
     * @return Two-qubit iSWAP matrix for adjacent two-qubit states.
     */
    [[nodiscard]] math::ComplexMatrix iSwapGate();

    /**
     * Creates a full-register iSWAP matrix for any two qubits.
     *
     * @param qubitCount Register size.
     * @param controlQubit First qubit to exchange.
     * @param targetQubit Second qubit to exchange.
     * @return stateCount by stateCount unitary matrix.
     * @throws std::invalid_argument if the qubit count is invalid or both qubits are the same.
     * @throws std::out_of_range if either qubit index is outside the register.
     */
    [[nodiscard]] math::ComplexMatrix iSwapGate(std::size_t qubitCount, std::size_t controlQubit,
                                                std::size_t targetQubit);
}

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
     * @return Principal square root of Pauli-X.
     */
    [[nodiscard]] math::ComplexMatrix sxGate();

    /**
     * @return Inverse principal square root of Pauli-X.
     */
    [[nodiscard]] math::ComplexMatrix sxDaggerGate();

    /**
     * Creates a general one-qubit phase gate.
     *
     * @param angleRadians Phase applied to the |1> basis state.
     * @return diag(1, exp(i * angleRadians)).
     */
    [[nodiscard]] math::ComplexMatrix phaseGate(double angleRadians);

    /**
     * Creates the universal three-angle one-qubit gate.
     *
     * @param thetaRadians Polar rotation angle.
     * @param phiRadians Relative output phase.
     * @param lambdaRadians Relative input phase.
     * @return U(theta, phi, lambda) in the standard computational basis.
     */
    [[nodiscard]] math::ComplexMatrix uGate(
        double thetaRadians,
        double phiRadians,
        double lambdaRadians
    );

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
     * @return Compact controlled-Hadamard matrix.
     */
    [[nodiscard]] math::ComplexMatrix chGate();

    /**
     * @return Compact controlled-S matrix.
     */
    [[nodiscard]] math::ComplexMatrix csGate();

    /**
     * @return Compact controlled inverse-S matrix.
     */
    [[nodiscard]] math::ComplexMatrix csDaggerGate();

    /**
     * @return Compact controlled-T matrix.
     */
    [[nodiscard]] math::ComplexMatrix ctGate();

    /**
     * @return Compact controlled inverse-T matrix.
     */
    [[nodiscard]] math::ComplexMatrix ctDaggerGate();

    /**
     * Creates a controlled phase gate.
     *
     * @param angleRadians Phase applied only to |11>.
     * @return Compact 4x4 controlled-phase matrix.
     */
    [[nodiscard]] math::ComplexMatrix controlledPhaseGate(
        double angleRadians
    );

    /**
     * Creates a controlled X-axis rotation.
     *
     * @param angleRadians Target rotation angle.
     * @return Compact 4x4 controlled-Rx matrix.
     */
    [[nodiscard]] math::ComplexMatrix crxGate(double angleRadians);

    /**
     * Creates a controlled Y-axis rotation.
     *
     * @param angleRadians Target rotation angle.
     * @return Compact 4x4 controlled-Ry matrix.
     */
    [[nodiscard]] math::ComplexMatrix cryGate(double angleRadians);

    /**
     * Creates a controlled Z-axis rotation.
     *
     * @param angleRadians Target rotation angle.
     * @return Compact 4x4 controlled-Rz matrix.
     */
    [[nodiscard]] math::ComplexMatrix crzGate(double angleRadians);

    /**
     * Creates the two-qubit XX interaction exp(-i theta X tensor X / 2).
     *
     * @param angleRadians Interaction angle.
     * @return Compact 4x4 RXX matrix.
     */
    [[nodiscard]] math::ComplexMatrix rxxGate(double angleRadians);

    /**
     * Creates the two-qubit YY interaction exp(-i theta Y tensor Y / 2).
     *
     * @param angleRadians Interaction angle.
     * @return Compact 4x4 RYY matrix.
     */
    [[nodiscard]] math::ComplexMatrix ryyGate(double angleRadians);

    /**
     * Creates the two-qubit ZZ interaction exp(-i theta Z tensor Z / 2).
     *
     * @param angleRadians Interaction angle.
     * @return Compact 4x4 RZZ matrix.
     */
    [[nodiscard]] math::ComplexMatrix rzzGate(double angleRadians);

    /**
     * @return Double-CNOT matrix, CX(0->1) followed by CX(1->0).
     */
    [[nodiscard]] math::ComplexMatrix dcxGate();

    /**
     * @return Echoed cross-resonance two-qubit gate matrix.
     */
    [[nodiscard]] math::ComplexMatrix ecrGate();

    /**
     * @return Principal square root of the SWAP operation.
     */
    [[nodiscard]] math::ComplexMatrix squareRootSwapGate();

    /**
     * Creates the two-parameter fermionic simulation gate.
     *
     * @param thetaRadians Exchange rotation between |01> and |10>.
     * @param phiRadians Conditional phase applied to |11>.
     * @return Compact 4x4 fSim matrix.
     */
    [[nodiscard]] math::ComplexMatrix fSimGate(
        double thetaRadians,
        double phiRadians
    );

    /**
     * @return Compact 8x8 controlled-controlled-X (Toffoli) matrix.
     */
    [[nodiscard]] math::ComplexMatrix ccxGate();

    /**
     * @return Compact 8x8 controlled-SWAP (Fredkin) matrix.
     */
    [[nodiscard]] math::ComplexMatrix cSwapGate();

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

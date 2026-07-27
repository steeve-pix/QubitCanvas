#pragma once

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include <cstddef>
#include <cstdint>

namespace quantum_sim::algorithms {
    /**
     * Builds a Bell-state demonstration in the leading two qubits.
     *
     * Additional register qubits remain in their initial state.
     *
     * @param qubitCount Total register size. Must be at least 2.
     * @return Circuit containing H(q0) and CX(q0 -> q1).
     * @throws std::invalid_argument if qubitCount is less than 2.
     */
    [[nodiscard]] circuit::QuantumCircuit bellStateCircuit(
        std::size_t qubitCount = 2U
    );

    /**
     * Builds a register-wide equal superposition circuit.
     *
     * @param qubitCount Number of qubits to place into superposition.
     * @return Circuit containing one Hadamard gate per qubit.
     */
    [[nodiscard]] circuit::QuantumCircuit equalSuperpositionCircuit(std::size_t qubitCount);

    /**
     * Builds a QFT-style phase-history showcase circuit for the GUI.
     *
     * @param qubitCount Number of qubits to use in the scripted circuit.
     * @return Circuit with Hadamards, decomposed phase interactions, swaps, and a final phase pass.
     * @throws std::invalid_argument if qubitCount is zero.
     */
    [[nodiscard]] circuit::QuantumCircuit qftCircuit(std::size_t qubitCount);

    /**
     * Builds the exact inverse of the QFT showcase circuit.
     *
     * @param qubitCount Number of qubits to use in the scripted circuit.
     * @return Circuit containing the reversed swaps, phase interactions, and rotations.
     * @throws std::invalid_argument if qubitCount is zero or cannot be represented safely.
     */
    [[nodiscard]] circuit::QuantumCircuit inverseQftCircuit(std::size_t qubitCount);

    /**
     * Builds a GHZ state across the complete selected register.
     *
     * @param qubitCount Number of qubits in the GHZ chain.
     * @return Circuit containing H(q0) followed by nearest-neighbor CX gates.
     * @throws std::invalid_argument if qubitCount is zero.
     */
    [[nodiscard]] circuit::QuantumCircuit ghzStateCircuit(
        std::size_t qubitCount = 3U
    );

    /**
     * Builds a register-wide Grover search.
     * The oracle marks |11...1> across the complete selected register.
     *
     * @param qubitCount Total register size. Must be at least 2.
     * @return Circuit with near-optimal register-wide oracle/diffusion iterations.
     * @throws std::invalid_argument if qubitCount is less than 2.
     */
    [[nodiscard]] circuit::QuantumCircuit groverSearchCircuit(
        std::size_t qubitCount = 2U
    );

    /**
     * Builds a balanced Deutsch-Jozsa demonstration.
     *
     * q0 through q(n - 2) are input qubits and q(n - 1) is the oracle ancilla.
     * The balanced function is the parity of every input bit.
     *
     * @param qubitCount Total register size. Must be at least 2.
     * @return Circuit using qubitCount - 1 inputs and one ancilla.
     * @throws std::invalid_argument if qubitCount is less than 2.
     */
    [[nodiscard]] circuit::QuantumCircuit deutschJozsaCircuit(
        std::size_t qubitCount = 3U
    );

    /**
     * Builds a Bernstein-Vazirani circuit for a caller-provided hidden bit string.
     *
     * Input qubits occupy q0 through q(inputQubitCount - 1), with the oracle
     * ancilla stored in the final qubit. The most-significant hidden bit maps to q0.
     *
     * @param inputQubitCount Number of input qubits, excluding the oracle ancilla.
     * @param hiddenValue Hidden bit string encoded as an unsigned integer.
     * @return Circuit that recovers hiddenValue on the input register.
     * @throws std::invalid_argument if the input count is zero, too large, or hiddenValue does not fit.
     */
    [[nodiscard]] circuit::QuantumCircuit bernsteinVaziraniCircuit(
        std::size_t inputQubitCount,
        std::size_t hiddenValue
    );

    /**
     * Builds a Toffoli demonstration using H, T, inverse-T, and CX gates.
     *
     * The circuit prepares q0 and q1 as |1⟩, then toggles q2 through a standard
     * decomposition so execution from |000⟩ finishes in |111⟩.
     *
     * Additional register qubits remain in their initial state.
     *
     * @param qubitCount Total register size. Must be at least 3.
     * @return Decomposed Toffoli demonstration on q0, q1, and q2.
     * @throws std::invalid_argument if qubitCount is less than 3.
     */
    [[nodiscard]] circuit::QuantumCircuit toffoliDemoCircuit(
        std::size_t qubitCount = 3U
    );

    /**
     * Builds a two-qubit phase-kickback demonstration.
     *
     * The target is prepared in |-⟩ before a controlled-X oracle. A final
     * Hadamard pass exposes the kicked-back phase as the basis state |11⟩.
     *
     * Additional register qubits remain in their initial state.
     *
     * @param qubitCount Total register size. Must be at least 2.
     * @return Phase-kickback circuit on q0 and q1.
     * @throws std::invalid_argument if qubitCount is less than 2.
     */
    [[nodiscard]] circuit::QuantumCircuit phaseKickbackCircuit(
        std::size_t qubitCount = 2U
    );

    /**
     * Builds a coherent three-qubit teleportation demonstration.
     *
     * q0 is prepared with Ry(pi/3) and Rz(pi/5), q1/q2 form the Bell pair,
     * and controlled corrections replace measurement feed-forward so every
     * debugger step remains unitary. The prepared state finishes on q2.
     *
     * Additional register qubits remain in their initial state.
     *
     * @param qubitCount Total register size. Must be at least 3.
     * @return Coherent teleportation circuit on q0, q1, and q2.
     * @throws std::invalid_argument if qubitCount is less than 3.
     */
    [[nodiscard]] circuit::QuantumCircuit teleportationCircuit(
        std::size_t qubitCount = 3U
    );

    /**
     * Builds a deterministic mixed-gate circuit for visualization stress testing.
     *
     * @param qubitCount Number of qubits to scramble.
     * @return Circuit containing superposition, local phase gates, entanglement, and an optional swap.
     * @throws std::invalid_argument if qubitCount is zero.
     */
    [[nodiscard]] circuit::QuantumCircuit scrambleCircuit(std::size_t qubitCount);

    /**
     * Builds a four-qubit Simon demonstration for the hidden period 11.
     *
     * q0/q1 form the input register and q2/q3 store a two-to-one parity
     * oracle. Additional qubits remain untouched.
     *
     * @param qubitCount Total register size. Must be at least 4.
     * @return Simon circuit whose input measurements satisfy y dot 11 = 0.
     * @throws std::invalid_argument if qubitCount is less than 4.
     */
    [[nodiscard]] circuit::QuantumCircuit simonCircuit(
        std::size_t qubitCount = 4U
    );

    /**
     * Builds a compact Shor order-finding demonstration for a = 4 mod 15.
     *
     * Three counting qubits estimate the work-register period r = 2. This is
     * the quantum period-finding component of Shor rather than a general
     * classical factoring front end.
     *
     * @param qubitCount Total register size. Must be at least 4.
     * @return Compiled order-finding circuit on q0 through q3.
     * @throws std::invalid_argument if qubitCount is less than 4.
     */
    [[nodiscard]] circuit::QuantumCircuit shorPeriodFindingCircuit(
        std::size_t qubitCount = 4U
    );

    /**
     * Builds two-bit quantum phase estimation for the known phase 1/4.
     *
     * q0/q1 are counting qubits and q2 is the prepared eigenstate.
     * Additional qubits remain untouched.
     *
     * @param qubitCount Total register size. Must be at least 3.
     * @return QPE circuit whose counting register resolves binary 0.01.
     * @throws std::invalid_argument if qubitCount is less than 3.
     */
    [[nodiscard]] circuit::QuantumCircuit quantumPhaseEstimationCircuit(
        std::size_t qubitCount = 3U
    );

    /**
     * Builds one fixed two-qubit VQE ansatz layer.
     *
     * The circuit prepares a Hartree-Fock seed and applies parameterized
     * rotations plus entanglement. Classical parameter optimization is
     * intentionally outside the unitary circuit trace.
     *
     * @param qubitCount Total register size. Must be at least 2.
     * @return Fixed-parameter VQE ansatz on q0/q1.
     * @throws std::invalid_argument if qubitCount is less than 2.
     */
    [[nodiscard]] circuit::QuantumCircuit vqeAnsatzCircuit(
        std::size_t qubitCount = 2U
    );

    /**
     * Builds a p=1 QAOA Max-Cut ansatz over the selected register.
     *
     * @param qubitCount Number of graph vertices/qubits. Must be at least 2.
     * @return Cost and mixer layers for a line graph, closed into a ring when possible.
     * @throws std::invalid_argument if qubitCount is less than 2.
     */
    [[nodiscard]] circuit::QuantumCircuit qaoaMaxCutCircuit(
        std::size_t qubitCount = 3U
    );

    /**
     * Builds a coherent four-qubit HHL structure for a fixed 2x2 toy system.
     *
     * The trace includes phase estimation, an eigenvalue-conditioned ancilla
     * rotation, and phase-register uncomputation. It demonstrates the quantum
     * kernel without claiming a general sparse-matrix solver.
     *
     * @param qubitCount Total register size. Must be at least 4.
     * @return Fixed HHL demonstration on q0 through q3.
     * @throws std::invalid_argument if qubitCount is less than 4.
     */
    [[nodiscard]] circuit::QuantumCircuit hhlDemoCircuit(
        std::size_t qubitCount = 4U
    );

    /**
     * Builds a three-qubit SWAP-test circuit for two non-identical states.
     *
     * q0 is the ancilla while q1/q2 hold |+> and |1>. The controlled SWAP is
     * decomposed into compact one- and two-qubit instructions.
     *
     * @param qubitCount Total register size. Must be at least 3.
     * @return SWAP test with ancilla-zero probability 3/4.
     * @throws std::invalid_argument if qubitCount is less than 3.
     */
    [[nodiscard]] circuit::QuantumCircuit swapTestCircuit(
        std::size_t qubitCount = 3U
    );

    /**
     * Builds two steps of a coined quantum walk on a four-position cycle.
     *
     * q0 is the coin and q1/q2 encode the position. Additional qubits remain
     * untouched.
     *
     * @param qubitCount Total register size. Must be at least 3.
     * @return Coherent conditional-increment/decrement walk circuit.
     * @throws std::invalid_argument if qubitCount is less than 3.
     */
    [[nodiscard]] circuit::QuantumCircuit quantumWalkCircuit(
        std::size_t qubitCount = 3U
    );

    /**
     * Builds a two-signal BB84 basis demonstration.
     *
     * q0 uses matching diagonal preparation/readout and recovers bit 1.
     * q1 uses a mismatched readout basis and therefore samples randomly.
     *
     * @param qubitCount Total register size. Must be at least 2.
     * @return Unitary BB84 preparation and basis-selection demonstration.
     * @throws std::invalid_argument if qubitCount is less than 2.
     */
    [[nodiscard]] circuit::QuantumCircuit bb84DemoCircuit(
        std::size_t qubitCount = 2U
    );

    /**
     * Builds superdense coding for the classical message 11.
     *
     * q0/q1 create a Bell pair, encode both bits on q0, then decode to |11>.
     * Additional qubits remain untouched.
     *
     * @param qubitCount Total register size. Must be at least 2.
     * @return Superdense coding circuit for message 11.
     * @throws std::invalid_argument if qubitCount is less than 2.
     */
    [[nodiscard]] circuit::QuantumCircuit superdenseCodingCircuit(
        std::size_t qubitCount = 2U
    );

    /**
     * Prepares the register-wide W state with one shared excitation.
     *
     * @param qubitCount Number of qubits. Must be at least 2.
     * @return Compact state-preparation circuit for an equal superposition of
     *         all basis states whose Hamming weight is one.
     * @throws std::invalid_argument if qubitCount is less than 2.
     */
    [[nodiscard]] circuit::QuantumCircuit wStateCircuit(
        std::size_t qubitCount
    );

    /**
     * Prepares a symmetric Dicke state with a fixed excitation count.
     *
     * @param qubitCount Number of qubits. Must be at least 2.
     * @param excitationCount Number of one bits in every populated basis state.
     * @return Compact state-preparation circuit with equal non-zero amplitudes.
     * @throws std::invalid_argument if the excitation count is zero or exceeds
     *         qubitCount.
     */
    [[nodiscard]] circuit::QuantumCircuit dickeStateCircuit(
        std::size_t qubitCount,
        std::size_t excitationCount = 2U
    );

    /**
     * Builds a one-dimensional graph/cluster state across the register.
     *
     * @param qubitCount Number of graph vertices. Must be at least 2.
     * @return Hadamard preparation followed by CZ edges between neighbors.
     * @throws std::invalid_argument if qubitCount is less than 2.
     */
    [[nodiscard]] circuit::QuantumCircuit graphStateCircuit(
        std::size_t qubitCount
    );

    /**
     * Builds a reproducible pseudo-random hardware-efficient circuit.
     *
     * Each layer applies random Ry/Rz angles followed by a staggered ring of
     * controlled-X gates, producing intentionally uneven probabilities.
     *
     * @param qubitCount Register size. Must be at least 2.
     * @param seed Seed controlling all generated angles.
     * @param layerCount Number of rotation and entanglement layers.
     * @return Deterministic circuit for the supplied seed.
     * @throws std::invalid_argument if qubitCount is less than 2 or layerCount is zero.
     */
    [[nodiscard]] circuit::QuantumCircuit randomCircuit(
        std::size_t qubitCount,
        std::uint64_t seed,
        std::size_t layerCount = 4U
    );

    /**
     * Prepares a deterministic non-uniform probability distribution.
     *
     * @param qubitCount Register size. Must be at least 1.
     * @return Compact state preparation whose basis-state probabilities differ.
     */
    [[nodiscard]] circuit::QuantumCircuit weightedStatePreparationCircuit(
        std::size_t qubitCount
    );

    /**
     * Demonstrates three-qubit repetition-code encoding and correction.
     *
     * @param qubitCount Total register size. Must be at least 3.
     * @return Circuit encoding q0, injecting a bit flip, and coherently decoding it.
     * @throws std::invalid_argument if qubitCount is less than 3.
     */
    [[nodiscard]] circuit::QuantumCircuit bitFlipCodeCircuit(
        std::size_t qubitCount = 3U
    );

    /**
     * Prepares the logical zero state of the seven-qubit Steane code.
     *
     * @param qubitCount Total register size. Must be at least 7.
     * @return Stabilizer encoding circuit on q0 through q6.
     * @throws std::invalid_argument if qubitCount is less than 7.
     */
    [[nodiscard]] circuit::QuantumCircuit steaneCodeCircuit(
        std::size_t qubitCount = 7U
    );

    /**
     * Encodes one prepared qubit with the nine-qubit Shor code.
     *
     * @param qubitCount Total register size. Must be at least 9.
     * @return Phase- and bit-flip repetition encoding on q0 through q8.
     * @throws std::invalid_argument if qubitCount is less than 9.
     */
    [[nodiscard]] circuit::QuantumCircuit shorCodeCircuit(
        std::size_t qubitCount = 9U
    );

    /**
     * Demonstrates coherent correction with the three-qubit phase-flip code.
     *
     * q0 carries a non-trivial input state, q0 through q2 are encoded in the
     * Hadamard basis, and a Z error is injected on q1 before coherent decoding.
     *
     * @param qubitCount Total register size. Must be at least 3.
     * @return Phase-flip encoding, error injection, and correction circuit.
     * @throws std::invalid_argument if qubitCount is less than 3.
     */
    [[nodiscard]] circuit::QuantumCircuit phaseFlipCodeCircuit(
        std::size_t qubitCount = 3U
    );

    /**
     * Prepares the logical-zero state of the five-qubit perfect code.
     *
     * The exact sixteen-term codeword is prepared with one compact reflection;
     * extra register qubits remain in |0>.
     *
     * @param qubitCount Total register size. Must be at least 5.
     * @return Exact [[5,1,3]] logical-zero state preparation.
     * @throws std::invalid_argument if qubitCount is less than 5.
     */
    [[nodiscard]] circuit::QuantumCircuit fiveQubitCodeCircuit(
        std::size_t qubitCount = 5U
    );

    /**
     * Builds a compact quantum-counting demonstration.
     *
     * q0 through q2 form the counting register and q3/q4 form a two-qubit
     * Grover search space with one marked state.
     *
     * @param qubitCount Total register size. Must be at least 5.
     * @return Controlled Grover powers followed by a three-qubit inverse QFT.
     * @throws std::invalid_argument if qubitCount is less than 5.
     */
    [[nodiscard]] circuit::QuantumCircuit quantumCountingCircuit(
        std::size_t qubitCount = 5U
    );

    /**
     * Builds three-bit quantum amplitude estimation for a fixed probability.
     *
     * q0 through q2 estimate the prepared probability on q3. The demonstration
     * uses only compact one- and two-qubit instructions at every register size.
     *
     * @param qubitCount Total register size. Must be at least 4.
     * @return Controlled amplification powers and inverse-QFT readout.
     * @throws std::invalid_argument if qubitCount is less than 4.
     */
    [[nodiscard]] circuit::QuantumCircuit amplitudeEstimationCircuit(
        std::size_t qubitCount = 4U
    );

    /**
     * Adds the two-bit value in q0/q1 into q2/q3 with a ripple carry.
     *
     * The preset prepares 1 + 2 and leaves the first operand intact, producing
     * the deterministic register state A=1, B=3.
     *
     * @param qubitCount Total register size. Must be at least 4.
     * @return Compact reversible ripple-carry addition circuit.
     * @throws std::invalid_argument if qubitCount is less than 4.
     */
    [[nodiscard]] circuit::QuantumCircuit rippleCarryAdderCircuit(
        std::size_t qubitCount = 4U
    );

    /**
     * Adds two two-bit values in the Fourier basis.
     *
     * The preset prepares 1 + 2, applies a QFT to q2/q3, accumulates controlled
     * phases from q0/q1, and uncomputes the Fourier transform.
     *
     * @param qubitCount Total register size. Must be at least 4.
     * @return Draper-style addition circuit producing A=1, B=3.
     * @throws std::invalid_argument if qubitCount is less than 4.
     */
    [[nodiscard]] circuit::QuantumCircuit draperAdderCircuit(
        std::size_t qubitCount = 4U
    );

    /**
     * Builds an instantaneous-quantum-polynomial sampling circuit.
     *
     * Hadamard layers surround deterministic commuting Rz and RZZ phase terms,
     * producing a reproducible uneven probability distribution.
     *
     * @param qubitCount Register size. Must be at least 2.
     * @return Register-wide IQP sampling circuit.
     * @throws std::invalid_argument if qubitCount is less than 2.
     */
    [[nodiscard]] circuit::QuantumCircuit iqpCircuit(
        std::size_t qubitCount
    );

    /**
     * Builds a compact surface-code stabilizer-measurement demonstration.
     *
     * q0 through q8 form a 3x3 data patch and q9 is a syndrome ancilla. The
     * circuit prepares graph correlations and coherently extracts one X check.
     *
     * @param qubitCount Total register size. Must be at least 10.
     * @return Ten-qubit stabilizer demonstration using local CZ interactions.
     * @throws std::invalid_argument if qubitCount is less than 10.
     */
    [[nodiscard]] circuit::QuantumCircuit surfaceCodeStabilizerCircuit(
        std::size_t qubitCount = 10U
    );

    /**
     * Builds a one-qubit Rx rotation demo.
     *
     * @param angleRadians Rotation angle in radians.
     * @return Circuit with one Rx instruction.
     */
    [[nodiscard]] circuit::QuantumCircuit rxRotationCircuit(double angleRadians);

    /**
     * Builds a one-qubit Ry rotation demo.
     *
     * @param angleRadians Rotation angle in radians.
     * @return Circuit with one Ry instruction.
     */
    [[nodiscard]] circuit::QuantumCircuit ryRotationCircuit(double angleRadians);

    /**
     * Builds a one-qubit Rz rotation demo.
     *
     * @param angleRadians Rotation angle in radians.
     * @return Circuit with one Rz instruction.
     */
    [[nodiscard]] circuit::QuantumCircuit rzRotationCircuit(double angleRadians);
}

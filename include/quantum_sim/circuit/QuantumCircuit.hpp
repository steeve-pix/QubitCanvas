#pragma once

#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

#include <cstddef>
#include <optional>
#include <random>
#include <string>
#include <variant>
#include <vector>

namespace quantum_sim::circuit {
    /**
     * One executed instruction and the register state immediately after it.
     */
    struct TraceStep {
        std::string description;
        quantum::QuantumRegister state;
    };

    /**
     * Instruction category used by renderers and console visualizers.
     */
    enum class CircuitInstructionKind {
        SingleQubit,
        TwoQubit,
        FullRegister,
        Reflection
    };

    /**
     * Public metadata for one instruction without exposing its full matrix.
     */
    struct CircuitInstructionInfo {
        std::string name;
        CircuitInstructionKind kind;
        std::optional<std::size_t> targetQubit;
        std::optional<std::size_t> controlQubit;
        std::optional<std::size_t> secondaryTargetQubit;
        std::optional<double> angleRadians;
    };

    /**
     * Ordered list of quantum instructions executed against a fixed-size register.
     */
    class QuantumCircuit final {
    public:
        /**
         * Creates an empty circuit for a fixed number of qubits.
         *
         * @param qubitCount Number of qubits. Must be at least 1.
         * @throws std::invalid_argument if qubitCount is zero.
         */
        explicit QuantumCircuit(std::size_t qubitCount);

        /**
         * @return Number of qubits this circuit acts on.
         */
        [[nodiscard]] std::size_t qubitCount() const noexcept;

        /**
         * Appends a single-qubit instruction.
         *
         * @param name Display name for the gate.
         * @param gate 2x2 unitary matrix.
         * @param targetQubit Qubit index to transform.
         * @param angleRadians Optional rotation angle retained for UI inspection.
         * @throws std::invalid_argument if gate is not a valid single-qubit unitary.
         * @throws std::out_of_range if targetQubit is outside the circuit.
         */
        void addSingleQubitGate(
            std::string name,
            math::ComplexMatrix gate,
            std::size_t targetQubit,
            std::optional<double> angleRadians = std::nullopt
        );

        /**
         * Appends a compact two-qubit instruction.
         *
         * The 4x4 matrix uses local |00⟩, |01⟩, |10⟩, |11⟩ ordering, with
         * firstQubit represented by the first bit. Keeping the local matrix
         * compact prevents controlled gates from consuming O(4^n) memory.
         *
         * @param name Display name for the gate.
         * @param gate 4x4 unitary matrix.
         * @param firstQubit Control or first gate operand.
         * @param secondQubit Target or second gate operand.
         * @param angleRadians Optional parameter angle retained for UI inspection.
         * @throws std::invalid_argument if the qubits match or gate is invalid.
         * @throws std::out_of_range if either qubit is outside the circuit.
         */
        void addTwoQubitGate(
            std::string name,
            math::ComplexMatrix gate,
            std::size_t firstQubit,
            std::size_t secondQubit,
            std::optional<double> angleRadians = std::nullopt
        );

        /**
         * Appends a compact full-state Householder reflection.
         *
         * The reflection is evaluated in O(2^n) time and memory rather than
         * materializing an O(4^n) full-register matrix.
         *
         * @param name Display name for the operation.
         * @param normalizedAxis Unit state-vector axis defining I - 2|u><u|.
         * @param displayQubit Qubit row used to display the operation.
         * @throws std::invalid_argument if the axis size or normalization is invalid.
         * @throws std::out_of_range if displayQubit is outside the circuit.
         */
        void addReflection(
            std::string name,
            math::ComplexVector normalizedAxis,
            std::size_t displayQubit = 0U
        );

        /**
         * Appends an instruction represented by a full-register matrix.
         *
         * @param name Display name for the gate.
         * @param gate stateCount by stateCount unitary matrix.
         * @throws std::invalid_argument if matrix dimensions or unitarity are invalid.
         */
        void addFullRegisterGate(std::string name, math::ComplexMatrix gate);

        /**
         * @return Number of instructions currently stored.
         */
        [[nodiscard]] std::size_t instructionCount() const noexcept;

        /**
         * Executes all instructions from an initial state.
         *
         * @param initialState Register whose qubit count must match this circuit.
         * @return Final register state.
         * @throws std::invalid_argument if initialState has the wrong qubit count.
         */
        [[nodiscard]] quantum::QuantumRegister execute(const quantum::QuantumRegister &initialState) const;

        /**
         * Executes the circuit repeatedly and measures each final state.
         *
         * @param initialState Register whose qubit count must match this circuit.
         * @param shotCount Number of measurement shots.
         * @param randomEngine Random engine used for sampling.
         * @return Measurement counts indexed by basis-state index.
         * @throws std::invalid_argument if initialState has the wrong qubit count.
         */
        [[nodiscard]] std::vector<std::size_t> runShots(const quantum::QuantumRegister &initialState,
                                                        std::size_t shotCount, std::mt19937 &randomEngine) const;

        /**
         * Executes the circuit and records state after every instruction.
         *
         * @param initialState Register whose qubit count must match this circuit.
         * @return Trace steps in instruction order.
         * @throws std::invalid_argument if initialState has the wrong qubit count.
         */
        [[nodiscard]] std::vector<TraceStep> executeWithTrace(const quantum::QuantumRegister &initialState) const;

        /**
         * Executes a suffix of the circuit and records every resulting state.
         *
         * @param stateBeforeFirstInstruction Register state immediately before
         *        firstInstructionIndex executes.
         * @param firstInstructionIndex First instruction to execute.
         * @return Trace steps from firstInstructionIndex through the circuit end.
         * @throws std::invalid_argument if the register size does not match.
         * @throws std::out_of_range if firstInstructionIndex is past the circuit end.
         */
        [[nodiscard]] std::vector<TraceStep> executeWithTraceFrom(
            const quantum::QuantumRegister &stateBeforeFirstInstruction,
            std::size_t firstInstructionIndex
        ) const;

        /**
         * @return Display metadata for all instructions in order.
         */
        [[nodiscard]] std::vector<CircuitInstructionInfo> instructionInfo() const;

        /**
         * Appends a two-qubit gate stored as a full-register instruction.
         *
         * @param name Display name for the gate.
         * @param gate stateCount by stateCount unitary matrix.
         * @param controlQubit Control or first qubit.
         * @param targetQubit Target or second qubit.
         * @throws std::invalid_argument if the qubits match or gate is invalid.
         * @throws std::out_of_range if either qubit index is outside the circuit.
         */
        void addControlledGate(std::string name, math::ComplexMatrix gate, std::size_t controlQubit,
                               std::size_t targetQubit);

        /**
         * Removes the final instruction when the circuit is not empty.
         *
         * @return True when an instruction was removed.
         */
        [[nodiscard]] bool removeLastInstruction() noexcept;

        /**
         * Removes an instruction by index.
         *
         * @param index Instruction index.
         * @return True when an instruction was removed.
         */
        [[nodiscard]] bool removeInstruction(std::size_t index);

        /**
         * Moves one instruction to another position without changing its data.
         *
         * @param fromIndex Current instruction index.
         * @param toIndex Destination index in the completed instruction list.
         * @return True when a valid instruction changed position.
         */
        [[nodiscard]] bool moveInstruction(
            std::size_t fromIndex,
            std::size_t toIndex
        );

        /**
         * Inserts a single-qubit instruction before the requested index.
         *
         * @param instructionIndex Insert position; values past the end append.
         * @param name Display name for the gate.
         * @param gate 2x2 unitary matrix.
         * @param targetQubit Qubit index to transform.
         * @param angleRadians Optional rotation angle retained for UI inspection.
         * @throws std::invalid_argument if gate is not a valid single-qubit unitary.
         * @throws std::out_of_range if targetQubit is outside the circuit.
         */
        void insertSingleQubitGate(
            std::size_t instructionIndex,
            std::string name,
            math::ComplexMatrix gate,
            std::size_t targetQubit,
            std::optional<double> angleRadians = std::nullopt
        );

        /**
         * Inserts a compact two-qubit instruction before the requested index.
         *
         * @param instructionIndex Insert position; values past the end append.
         * @param name Display name for the gate.
         * @param gate 4x4 unitary matrix in first/second local basis order.
         * @param firstQubit Control or first gate operand.
         * @param secondQubit Target or second gate operand.
         * @param angleRadians Optional parameter angle retained for UI inspection.
         * @throws std::invalid_argument if the qubits match or gate is invalid.
         * @throws std::out_of_range if either qubit is outside the circuit.
         */
        void insertTwoQubitGate(
            std::size_t instructionIndex,
            std::string name,
            math::ComplexMatrix gate,
            std::size_t firstQubit,
            std::size_t secondQubit,
            std::optional<double> angleRadians = std::nullopt
        );

        /**
         * Inserts a two-qubit full-register instruction before the requested index.
         *
         * @param instructionIndex Insert position; values past the end append.
         * @param name Display name for the gate.
         * @param gate stateCount by stateCount unitary matrix.
         * @param controlQubit Control or first qubit.
         * @param targetQubit Target or second qubit.
         * @throws std::invalid_argument if the qubits match or gate is invalid.
         * @throws std::out_of_range if either qubit index is outside the circuit.
         */
        void insertControlledGate(std::size_t instructionIndex, std::string name, math::ComplexMatrix gate,
                                  std::size_t controlQubit, std::size_t targetQubit);

    private:
        struct SingleQubitInstruction {
            std::string name;
            math::ComplexMatrix gate;
            std::size_t targetQubit;
            std::optional<double> angleRadians;
        };

        struct TwoQubitInstruction {
            std::string name;
            math::ComplexMatrix gate;
            std::size_t firstQubit;
            std::size_t secondQubit;
            std::optional<double> angleRadians;
        };

        struct ReflectionInstruction {
            std::string name;
            math::ComplexVector normalizedAxis;
            std::size_t displayQubit;
        };

        struct FullRegisterInstruction {
            std::string name;
            math::ComplexMatrix gate;
            std::optional<std::size_t> controlQubit;
            std::optional<std::size_t> targetQubit;
        };

        using Instruction = std::variant<
            SingleQubitInstruction,
            TwoQubitInstruction,
            FullRegisterInstruction,
            ReflectionInstruction
        >;

        std::size_t qubitCount_;
        std::vector<Instruction> instructions_;
    };
}

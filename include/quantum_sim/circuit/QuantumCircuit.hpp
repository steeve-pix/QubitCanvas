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
        FullRegister
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

        struct FullRegisterInstruction {
            std::string name;
            math::ComplexMatrix gate;
            std::optional<std::size_t> controlQubit;
            std::optional<std::size_t> targetQubit;
        };

        using Instruction = std::variant<SingleQubitInstruction, FullRegisterInstruction>;

        std::size_t qubitCount_;
        std::vector<Instruction> instructions_;
    };
}

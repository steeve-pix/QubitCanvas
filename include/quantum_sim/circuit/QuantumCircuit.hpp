#pragma once

#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"

#include <cstddef>
#include <vector>
#include <variant>
#include <random>
#include <string>
#include <optional>

namespace quantum_sim::circuit {
    /**
     * Represents a single step in the execution trace of a quantum circuit or process.
     *
     * This class encapsulates information about an individual operation
     * or transformation performed during the execution of a quantum computation.
     */
    struct TraceStep {
        /**
         * A string that provides a human-readable description of a specific step
         * in the execution trace of a quantum circuit.
         *
         * This description typically explains the operation performed or the
         * transformation applied to the quantum state at this step.
         */
        std::string description;
        /**
         * Represents the current configuration or status of a system, process,
         * or component at a specific point in time.
         *
         * This variable typically holds information about the present conditions,
         * enabling logic to make decisions or track changes over time.
         */
        quantum::QuantumRegister state;
    };

    enum class CircuitInstructionKind {
        SingleQubit,
        FullRegister
    };

    struct CircuitInstructionInfo {
        /**
         * Represents the name or identifier associated with an entity or object.
         *
         * This variable is used to store a textual label or designation
         * that uniquely identifies or describes the corresponding instance.
         */
        std::string name;
        /**
         * Represents a classification or category identifier used to differentiate or group items.
         *
         * This variable is typically used to specify or determine the type, nature,
         * or purpose of an entity in a given context.
         */
        CircuitInstructionKind kind;
        /**
         * Specifies the target qubit for a quantum operation within a quantum circuit.
         *
         * This variable is used to identify the qubit on which a single-qubit gate
         * or operation is applied. If no target qubit is specified, it indicates
         * that the operation does not involve a specific qubit or is not a single-qubit
         * operation.
         *
         * The value is optional to account for operations that may not require a target
         * qubit, such as full-register operations or measurements involving multiple qubits.
         */
        std::optional<std::size_t> targetQubit;
        /**
         * Represents the optional index of the control qubit in a quantum circuit operation.
         *
         * This variable denotes the qubit that acts as the control for a controlled quantum gate,
         * such as a CX or controlled phase gate. If the value is present, it specifies the index
         * of the control qubit within the circuit. Otherwise, the operation does not have a control qubit.
         */
        std::optional<std::size_t> controlQubit;
        /**
         * Represents an optional secondary target qubit in a quantum operation.
         *
         * This variable is used in quantum circuits to specify a secondary
         * target qubit that is involved in multi-qubit operations, such as
         * controlled gates (e.g., CX). When set, it identifies the index
         * of the additional qubit required by the operation. If nullopt, no
         * secondary target qubit is involved.
         */
        std::optional<std::size_t> secondaryTargetQubit;
    };

    /**
     * A class representing a quantum circuit that operates on a specified number of qubits.
     *
     * This class allows the user to construct a quantum circuit, add quantum gates, and
     * simulate quantum computations, including measurements.
     */
    class QuantumCircuit final {
        /**
         * Constructs a QuantumCircuit object with the specified number of qubits.
         *
         * @param qubitCount The number of qubits in the quantum circuit. Must be greater than zero.
         * @throws std::invalid_argument If the specified qubit count is zero.
         */
    public:
        explicit QuantumCircuit(std::size_t qubitCount);

        /**
         * Retrieves the number of qubits in the quantum circuit.
         *
         * @return The total number of qubits in the circuit as a std::size_t value.
         */
        [[nodiscard]] std::size_t qubitCount() const noexcept;

        /**
         * Adds a single-qubit quantum gate to the circuit.
         *
         * The provided gate must be a 2x2 unitary matrix. The target qubit index must
         * be in the valid range of the circuit's qubits.
         *
         * @param gate A 2x2 unitary matrix representing the single-qubit quantum gate.
         * @param targetQubit The index of the target qubit where the gate will be applied.
         * @throws std::invalid_argument If the gate is not a 2x2 matrix or if it is not unitary.
         * @throws std::out_of_range If the target qubit index is outside the valid bounds of the circuit.
         */
        void addSingleQubitGate(std::string name, math::ComplexMatrix gate, std::size_t targetQubit);

        /**
         * Adds a full-register quantum gate to the circuit.
         * A full-register gate operates on all qubits of the circuit simultaneously.
         * The gate must have dimensions matching the size of the circuit state space
         * (2^qubitCount) and must be a unitary matrix.
         *
         * @param gate The complex-valued matrix representing the full-register gate.
         *             The matrix dimensions must be 2^qubitCount x 2^qubitCount, and
         *             it must satisfy the property of being unitary.
         * @throws std::invalid_argument if the gate dimensions do not match the circuit
         *         state count or if the gate is not unitary.
         */
        void addFullRegisterGate(std::string name, math::ComplexMatrix gate);

        /**
         * Retrieves the total number of instructions added to the quantum circuit.
         *
         * @return The number of instructions currently present in the quantum circuit.
         */
        [[nodiscard]] std::size_t instructionCount() const noexcept;

        /**
         * Executes the primary operation or process defined by the method's implementation.
         *
         * @return A boolean value indicating the success or failure of the execution.
         */
        [[nodiscard]] quantum::QuantumRegister execute(const quantum::QuantumRegister &initialState) const;

        /**
         * Simulates multiple measurement shots on the quantum circuit starting from an initial quantum state.
         * Each shot involves executing the circuit, collapsing the quantum state with a measurement,
         * and counting the occurrences of measured states over all shots.
         *
         * @param initialState The initial quantum state as a QuantumRegister.
         * @param shotCount The number of measurement shots to simulate.
         * @param randomEngine The random number generator used for sampling during measurement.
         * @return A vector where each element represents the number of occurrences of the corresponding state
         *         after simulating the specified number of shots.
         */
        [[nodiscard]] std::vector<std::size_t> runShots(const quantum::QuantumRegister &initialState,
                                                        std::size_t shotCount, std::mt19937 &randomEngine) const;

        /**
         * Executes the quantum circuit on a given initial quantum register and returns
         * a detailed execution trace of the process.
         *
         * This method processes each instruction in the circuit, applying it to the
         * quantum register, and records the sequential steps in the form of a trace.
         * Each trace step includes a description of the operation performed and the
         * corresponding quantum state after the operation.
         *
         * @param initialState The initial quantum register to be used as the starting
         *                     state for the execution of the circuit. The number of
         *                     qubits in this register must match the circuit's
         *                     qubit count, or an exception will be thrown.
         * @return A vector of `TraceStep` objects, where each object represents an
         *         individual operation performed during circuit execution along with
         *         the resulting quantum state.
         * @throws std::invalid_argument If the number of qubits in the initial state
         *                                does not match the qubit count of the circuit.
         */
        [[nodiscard]] std::vector<TraceStep> executeWithTrace(const quantum::QuantumRegister &initialState) const;

        /**
         * Retrieves detailed information about the instructions in the quantum circuit.
         *
         * This method processes all instructions contained within the quantum circuit and
         * compiles a vector of CircuitInstructionInfo objects, each representing a single
         * instruction with its associated metadata. The type of instruction (e.g., single-qubit
         * or full-register operation) and relevant parameters such as target and control qubits
         * are included in the result.
         *
         * @return A vector of CircuitInstructionInfo objects, each containing metadata about
         *         an individual instruction in the quantum circuit.
         */
        [[nodiscard]] std::vector<CircuitInstructionInfo> instructionInfo() const;

        /**
         * Adds a controlled quantum gate to the circuit.
         *
         * This method appends a controlled quantum gate operation to the circuit's list of instructions.
         * A controlled gate applies the specified unitary transformation to the target qubit
         * only when the control qubit is in the specified state.
         *
         * @param name The name of the controlled gate.
         * @param gate A unitary matrix defining the transformation of the controlled gate.
         * @param controlQubit The index of the control qubit in the circuit.
         * @param targetQubit The index of the target qubit for the gate.
         *
         * @throws std::out_of_range If the control or target qubit index exceeds the number of qubits in the circuit.
         * @throws std::invalid_argument If the control qubit is the same as the target qubit.
         * @throws std::invalid_argument If the dimensions of the matrix do not match the size of the circuit's state.
         * @throws std::invalid_argument If the matrix is not unitary.
         */
        void addControlledGate(std::string name, math::ComplexMatrix gate, std::size_t controlQubit,
                               std::size_t targetQubit);

        [[nodiscard]] bool removeLastInstruction() noexcept;

        [[nodiscard]] bool removeInstruction(std::size_t index);

        /**
         * A structure representing an instruction for a single-qubit quantum gate.
         *
         * This structure encapsulates a quantum gate, represented as a complex matrix,
         * and the target qubit on which the gate is applied during the execution of
         * a quantum circuit. It is used to define single-qubit operations within a
         * quantum algorithm.
         */
    private:
        struct SingleQubitInstruction {
            /**
             * A variable representing the name or identifier, typically used to store a label or title.
             *
             * This variable is commonly utilized as a general-purpose placeholder for textual content
             * and may represent entity names, user inputs, or other string-based identifiers in a program.
             */
            std::string name;
            /**
             * A complex matrix representing a single-qubit quantum gate.
             *
             * This matrix defines the transformation applied to the quantum state
             * of a single qubit during the execution of a quantum circuit.
             */
            math::ComplexMatrix gate;
            /**
             * The index of the target qubit on which a single-qubit quantum gate is applied.
             *
             * This variable specifies the qubit in the quantum register to which the gate
             * operation defined by the instruction applies. The index corresponds to the
             * position of the qubit within the quantum circuit.
             */
            std::size_t targetQubit;
        };

        /**
         * A structure representing an instruction that applies a full-register quantum gate
         * to a quantum circuit.
         *
         * A full-register quantum gate operates on all qubits within the circuit simultaneously.
         * The gate is represented as a complex-valued matrix with dimensions matching the state
         * space size of the circuit, which is 2^qubitCount x 2^qubitCount, where qubitCount
         * is the total number of qubits in the circuit. The matrix must satisfy the property
         * of being unitary.
         */
        struct FullRegisterInstruction {
            /**
             * A string representing the name of the full register instruction.
             *
             * This variable is used to identify or label the instruction, allowing for
             * easier reference and clarity in quantum circuit operations.
             */
            std::string name;
            /**
             * A complex-valued matrix representing the quantum gate to be applied.
             *
             * This matrix defines the operation of the quantum gate on the quantum register.
             * It encapsulates the mathematical transformation that the gate performs when
             * executed as part of a quantum circuit.
             */
            math::ComplexMatrix gate;
            /**
             * Represents the control qubit for a quantum gate in a quantum circuit.
             *
             * This variable optionally specifies the index of the qubit that acts as the control
             * in a controlled quantum operation. If a value is present, the gate is conditional
             * on the state of the specified qubit. If no value is present, the operation is
             * performed without a control qubit.
             */
            std::optional<std::size_t> controlQubit;
            /**
             * Represents the target qubit for a quantum gate operation in a quantum circuit.
             *
             * This optional variable identifies the specific qubit on which a single-qubit gate
             * is applied. If the value is not set, it indicates that the operation does not
             * target a specific qubit or applies to all qubits in the register.
             */
            std::optional<std::size_t> targetQubit;
        };

        using Instruction = std::variant<SingleQubitInstruction, FullRegisterInstruction>;

        /**
         * Stores the number of qubits in the quantum circuit.
         *
         * This variable defines the size of the quantum system and is used to validate
         * operations such as gate applications and circuit manipulations. The value must be a
         * positive integer greater than zero and remains constant for the lifetime of the
         * QuantumCircuit instance.
         */
        std::size_t qubitCount_;
        /**
         * A container for storing the sequence of quantum instructions in the circuit.
         *
         * This vector holds a collection of quantum instructions, where each instruction
         * can either represent a single-qubit gate operation or a full-register gate operation.
         * The stored instructions define the transformations applied to the quantum state
         * of the circuit.
         *
         * The `Instruction` type is a variant that can encapsulate one of the following:
         * - `SingleQubitInstruction`: Represents a gate operation applied to a single qubit.
         * - `FullRegisterInstruction`: Represents a gate operation applied to all qubits in the circuit.
         *
         * Instructions are added to this container when gates are applied using the appropriate
         * methods in the QuantumCircuit class. The sequence in which instructions are stored
         * corresponds to the order in which the gates were applied.
         */
        std::vector<Instruction> instructions_;
    };
}

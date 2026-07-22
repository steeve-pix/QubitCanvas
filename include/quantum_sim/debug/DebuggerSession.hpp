#pragma once
#include <cstddef>
#include <vector>

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"


namespace quantum_sim::debug {
    class DebuggerSession {
    public:
        /**
         * Defines a debugging session for quantum circuit execution.
         *
         * This class represents a session for debugging the execution of a quantum circuit.
         * It maintains state information, including the initial state of the quantum register,
         * the sequence of trace steps, and metadata for the circuit's instructions. Users can
         * step through the execution trace, inspect states and operations, and restart the
         * session. The class provides mechanisms for navigating, analyzing, and managing
         * the quantum circuit's execution.
         */
        DebuggerSession(const circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState);

        /**
         * Retrieves the total number of steps in the execution trace.
         *
         * This method returns the total count of all trace steps that have been recorded
         * in the debugging session, representing the full sequence of operations or transformations.
         *
         * @return The total number of steps in the execution trace as a `std::size_t`.
         */
        [[nodiscard]] std::size_t stepCount() const noexcept;

        /**
         * Retrieves the index of the current step in the execution trace.
         *
         * The method returns the position of the current trace step in the sequence
         * of operations, which represents the index of the operation or transformation
         * being executed during the debugging session.
         *
         * @return The current step index in the execution trace as a `std::size_t`.
         */
        [[nodiscard]] std::size_t currentStepIndex() const noexcept;

        /**
         * Retrieves the current step in the execution trace of the quantum circuit.
         *
         * The method returns a reference to the current `TraceStep` object, which
         * represents the operation or transformation applied at the current trace
         * index during the quantum circuit's execution. The returned step provides
         * detailed information about the ongoing computation.
         *
         * @return A constant reference to the current `TraceStep` in the execution trace.
         */
        [[nodiscard]] const circuit::TraceStep &currentStep() const noexcept;

        /**
         * Determines whether the debugger session can move to the next step
         * in the quantum circuit execution trace.
         *
         * The method checks if the current step index incremented by one
         * is less than the total number of steps in the trace, indicating
         * that moving to the next step is possible.
         *
         * @return true if moving to the next step is allowed, false otherwise.
         */
        [[nodiscard]] bool canMoveNext() const noexcept;

        /**
         * Determines whether the debugger session can move to the previous step
         * in the quantum circuit execution trace.
         *
         * The method checks if the current step index is greater than zero,
         * indicating that moving to the previous step is possible.
         *
         * @return true if moving to the previous step is allowed, false otherwise.
         */
        [[nodiscard]] bool canMovePrevious() const noexcept;

        /**
         * Advances the debugger session to the next step in the quantum circuit execution trace.
         *
         * The method checks whether advancing to the next step is possible by invoking `canMoveNext()`.
         * If the transition is allowed, it increments the `currentStep_` index and returns true.
         * Otherwise, it returns false without making any changes.
         *
         * @return true if the session successfully moves to the next step, false otherwise.
         */
        bool moveNext() noexcept;

        /**
         * Moves the debugger session to the previous step in the quantum circuit execution trace.
         *
         * The method verifies whether moving to the previous step is possible by invoking `canMovePrevious()`.
         * If the transition is allowed, it decrements the `currentStep_` index and returns true.
         * Otherwise, it returns false without making any changes.
         *
         * @return true if the session successfully moves to the previous step, false otherwise.
         */
        bool movePrevious() noexcept;

        /**
         * Restarts the current process or system, resetting it to its initial state.
         *
         * This method terminates any ongoing operations, clears relevant state data, and
         * reinitializes the process to its starting configuration. It ensures that the system
         * is effectively restarted and ready for further operations.
         */
        void restart() noexcept;

        /**
         * Retrieves the initial state of the quantum register at the start of the debugging session.
         *
         * @return A constant reference to a QuantumRegister representing the initial state.
         */
        [[nodiscard]] const quantum::QuantumRegister &initialState() const noexcept;

        /**
         * Retrieves the quantum register state before the current simulation step.
         *
         * If the current step is the initial step (step 0), this method returns the
         * initial state of the quantum register. Otherwise, it returns the state
         * of the quantum register from the last completed simulation step.
         *
         * @return A constant reference to the quantum::QuantumRegister representing
         *         the state before the current step. If the current step is 0, the
         *         initial state is returned.
         */
        [[nodiscard]] const quantum::QuantumRegister &stateBeforeCurrentStep() const noexcept;

        [[nodiscard]] const circuit::CircuitInstructionInfo &currentInstruction() const;

        /**
         * Represents the initial state of the quantum register for the debugging session.
         *
         * This variable holds the quantum register's configuration at the beginning of
         * the session and serves as a reference point for operations and analyses
         * performed during the debugging process.
         */
    private:
        quantum::QuantumRegister initialState_;
        /**
         * Stores the sequence of trace steps for a quantum circuit's execution.
         *
         * This variable serves as a container for the ordered set of operations or instructions
         * that constitute the quantum circuit. Each entry in the vector is a `TraceStep` object,
         * which encapsulates details about a single operation, such as the type of gate, involved qubits,
         * and any associated control parameters.
         *
         * The `trace_` vector is primarily used for debugging and analysis purposes, enabling
         * users to visualize or step through the sequence of operations performed on the quantum register.
         * It is populated at the initialization of the debugger session and may be referenced throughout
         * the debugging process to retrieve metadata for the circuit's individual steps.
         */
        std::vector<circuit::TraceStep> trace_;
        /**
         * Stores information about the sequence of instructions in a quantum circuit.
         *
         * This variable holds a collection of metadata or descriptions
         * for each step or operation in the quantum circuit. Each item
         * in the vector corresponds to a single instruction, storing
         * details such as operation type, target qubits, control qubits,
         * and any additional parameters or context required for
         * executing or debugging the quantum circuit.
         *
         * It is initialized by extracting the instruction details
         * from the quantum circuit and is primarily used for debugging
         * or analysis tasks involving the executed instructions.
         */
        std::vector<circuit::CircuitInstructionInfo> instructions_;
        /**
         * Tracks the current step index within the quantum circuit execution trace.
         *
         * This variable represents the position of the currently active step during a
         * debugging session. It is initialized to zero and is used to index into the
         * `trace_` vector, which holds the sequence of trace steps in the quantum circuit.
         *
         * The value of `currentStep_` is updated through methods such as `moveNext()`
         * and `movePrevious()`, ensuring that the debugger correctly reflects the
         * current state of the quantum circuit execution. Valid values range from 0
         * (the initial step) to `trace_.size() - 1`.
         */
        std::size_t currentStep_{};
    };
}

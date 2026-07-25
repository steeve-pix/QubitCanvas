#pragma once
#include <cstddef>
#include <vector>

#include "DebuggerSnapshot.hpp"
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

namespace quantum_sim::debug {
    /**
     * Stores an execution trace and exposes step-by-step navigation through it.
     */
    class DebuggerSession {
    public:
        /**
         * Builds an execution trace for the provided circuit and initial state.
         *
         * @param circuit Circuit to execute.
         * @param initialState Register used as the trace start.
         */
        DebuggerSession(const circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState);

        /**
         * @return Number of trace steps, one per circuit instruction.
         */
        [[nodiscard]] std::size_t stepCount() const noexcept;

        /**
         * @return Current trace step index.
         */
        [[nodiscard]] std::size_t currentStepIndex() const noexcept;

        /**
         * @return Current trace step.
         * @throws std::logic_error if the trace is empty.
         */
        [[nodiscard]] const circuit::TraceStep &currentStep() const;

        /**
         * @return True when moveNext() can advance.
         */
        [[nodiscard]] bool canMoveNext() const noexcept;

        /**
         * @return True when movePrevious() can rewind.
         */
        [[nodiscard]] bool canMovePrevious() const noexcept;

        /**
         * Moves to the next trace step when possible.
         *
         * @return True when the current step changed.
         */
        bool moveNext() noexcept;

        /**
         * Moves to the previous trace step when possible.
         *
         * @return True when the current step changed.
         */
        bool movePrevious() noexcept;

        /**
         * Returns navigation to the first trace step.
         */
        void restart() noexcept;

        /**
         * @return Register state before any instruction executes.
         */
        [[nodiscard]] const quantum::QuantumRegister &initialState() const noexcept;

        /**
         * @return State immediately before the current instruction.
         */
        [[nodiscard]] const quantum::QuantumRegister &stateBeforeCurrentStep() const noexcept;

        /**
         * @return Metadata for the current instruction.
         * @throws std::out_of_range if the current step is invalid.
         */
        [[nodiscard]] const circuit::CircuitInstructionInfo &currentInstruction() const;

        /**
         * @return Snapshot containing current instruction, before state, and after state.
         */
        [[nodiscard]] DebuggerSnapshot snapshot() const;

        /**
         * Reads any trace step by index.
         *
         * @param index Trace step index.
         * @return Requested trace step.
         * @throws std::out_of_range if index is outside the trace.
         */
        [[nodiscard]] const circuit::TraceStep &stepAt(std::size_t index) const;

        /**
         * Jumps directly to a trace step.
         *
         * @param index Trace step index.
         * @throws std::out_of_range if index is outside the trace.
         */
        void moveToStep(std::size_t index);

        /**
         * Replaces the stored trace after the circuit or initial state changes.
         *
         * @param circuit Circuit to execute.
         * @param initialState Register used as the trace start.
         */
        void rebuild(const circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState);

        /**
         * @return True when the trace contains at least one step.
         */
        [[nodiscard]] bool hasSteps() const noexcept;

    private:
        quantum::QuantumRegister initialState_;
        std::vector<circuit::TraceStep> trace_;
        std::vector<circuit::CircuitInstructionInfo> instructions_;
        std::size_t currentStep_{};
    };
}

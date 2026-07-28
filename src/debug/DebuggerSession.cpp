#include "quantum_sim/debug/DebuggerSession.hpp"
#include <iterator>
#include <stdexcept>

namespace quantum_sim::debug {
    DebuggerSession::DebuggerSession(const circuit::QuantumCircuit &circuit,
                                     const quantum::QuantumRegister &initialState)
        : initialState_{initialState} {
        // Reuse rebuild so construction and later circuit edits follow one path.
        rebuild(circuit, initialState);
    }

    std::size_t DebuggerSession::stepCount() const noexcept {
        return trace_.size();
    }

    std::size_t DebuggerSession::currentStepNumber() const noexcept {
        return currentStepNumber_;
    }

    bool DebuggerSession::isAtInitialState() const noexcept {
        return currentStepNumber_ == 0U;
    }

    const circuit::TraceStep &DebuggerSession::currentStep() const {
        if (isAtInitialState() || trace_.empty()) {
            throw std::logic_error{
                "DebuggerSession::currentStep called at initial step zero"
            };
        }

        return trace_[currentStepNumber_ - 1U];
    }

    bool DebuggerSession::canMoveNext() const noexcept {
        return currentStepNumber_ < trace_.size();
    }

    bool DebuggerSession::canMovePrevious() const noexcept {
        return currentStepNumber_ > 0U;
    }

    bool DebuggerSession::moveNext() noexcept {
        if (!canMoveNext()) {
            return false;
        }

        ++currentStepNumber_;
        return true;
    }

    bool DebuggerSession::movePrevious() noexcept {
        if (!canMovePrevious()) {
            return false;
        }

        --currentStepNumber_;
        return true;
    }

    void DebuggerSession::restart() noexcept {
        currentStepNumber_ = 0U;
    }

    const quantum::QuantumRegister &DebuggerSession::initialState() const noexcept {
        return initialState_;
    }

    const quantum::QuantumRegister &DebuggerSession::stateBeforeCurrentStep() const noexcept {
        if (currentStepNumber_ <= 1U) {
            // Step zero and the first instruction both begin at the initial register.
            return initialState_;
        }

        // Later steps compare against the state after the previous instruction.
        return trace_[currentStepNumber_ - 2U].state;
    }

    const circuit::CircuitInstructionInfo &DebuggerSession::currentInstruction() const {
        if (isAtInitialState()) {
            throw std::logic_error{
                "DebuggerSession::currentInstruction called at initial step zero"
            };
        }

        return instructions_.at(currentStepNumber_ - 1U);
    }

    DebuggerSnapshot DebuggerSession::snapshot() const {
        if (isAtInitialState()) {
            // Step zero is available for both empty and populated circuits.
            return DebuggerSnapshot{
                .currentStepNumber = 0U,
                .stepCount = stepCount(),
                .instruction = std::nullopt,
                .beforeState = initialState_,
                .afterState = initialState_,
                .canMoveNext = canMoveNext(),
                .canMovePrevious = false
            };
        }

        return DebuggerSnapshot{
            currentStepNumber(),
            stepCount(),
            std::cref(currentInstruction()),
            std::cref(stateBeforeCurrentStep()),
            std::cref(currentStep().state),
            canMoveNext(),
            canMovePrevious()
        };
    }

    const circuit::TraceStep &DebuggerSession::stepAt(std::size_t index) const {
        if (index >= trace_.size()) {
            throw std::out_of_range(
                "DebuggerSession::stepAt index out of range"
            );
        }

        return trace_[index];
    }

    void DebuggerSession::moveToStep(std::size_t index) {
        if (index >= trace_.size()) {
            throw std::out_of_range{"DebuggerSession::moveToStep index out of range"};
        }

        currentStepNumber_ = index + 1U;
    }

    void DebuggerSession::moveToStepNumber(const std::size_t stepNumber) {
        if (stepNumber > trace_.size()) {
            throw std::out_of_range{
                "DebuggerSession::moveToStepNumber step number out of range"
            };
        }

        currentStepNumber_ = stepNumber;
    }

    void DebuggerSession::rebuild(const circuit::QuantumCircuit &circuit,
                                  const quantum::QuantumRegister &initialState,
                                  const util::StopToken stopToken) {
        initialState_ = initialState;

        // Trace and metadata are rebuilt together so indices stay aligned.
        trace_ =
                circuit.executeWithTrace(initialState_, stopToken);

        if (stopToken.stop_requested()) {
            throw circuit::TraceBuildCancelled{};
        }

        instructions_ = circuit.instructionInfo();

        currentStepNumber_ = 0U;
    }

    void DebuggerSession::rebuildFrom(
        const circuit::QuantumCircuit &circuit,
        const quantum::QuantumRegister &initialState,
        const std::size_t firstChangedInstruction,
        const util::StopToken stopToken
    ) {
        if (
            initialState.qubitCount() != initialState_.qubitCount() ||
            firstChangedInstruction > trace_.size() ||
            firstChangedInstruction > circuit.instructionCount()
        ) {
            rebuild(circuit, initialState, stopToken);
            return;
        }

        initialState_ =
                initialState;

        const quantum::QuantumRegister &stateBeforeChangedInstruction =
                firstChangedInstruction == 0U
                    ? initialState_
                    : trace_[firstChangedInstruction - 1U].state;

        std::vector<circuit::TraceStep> rebuiltSuffix =
                circuit.executeWithTraceFrom(
                    stateBeforeChangedInstruction,
                    firstChangedInstruction,
                    stopToken
                );

        if (stopToken.stop_requested()) {
            throw circuit::TraceBuildCancelled{};
        }

        trace_.erase(
            trace_.begin() +
            static_cast<std::ptrdiff_t>(firstChangedInstruction),
            trace_.end()
        );

        trace_.insert(
            trace_.end(),
            std::make_move_iterator(rebuiltSuffix.begin()),
            std::make_move_iterator(rebuiltSuffix.end())
        );

        instructions_ =
                circuit.instructionInfo();

        currentStepNumber_ = 0U;
    }

    bool DebuggerSession::hasSteps() const noexcept {
        return !trace_.empty();
    }
}

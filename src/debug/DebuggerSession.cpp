#include "quantum_sim/debug/DebuggerSession.hpp"
#include <stdexcept>

namespace quantum_sim::debug {
    DebuggerSession::DebuggerSession(const circuit::QuantumCircuit &circuit,
                                     const quantum::QuantumRegister &initialState)
        : initialState_{initialState} {
        rebuild(circuit, initialState);
    }

    std::size_t DebuggerSession::stepCount() const noexcept {
        return trace_.size();
    }

    std::size_t DebuggerSession::currentStepIndex() const noexcept {
        return currentStep_;
    }

    const circuit::TraceStep &DebuggerSession::currentStep() const {
        if (trace_.empty()) {
            throw std::logic_error{"DebuggerSession::currentStep called with an empty trace"};
        }

        return trace_[currentStep_];
    }

    bool DebuggerSession::canMoveNext() const noexcept {
        return currentStep_ + 1 < trace_.size();
    }

    bool DebuggerSession::canMovePrevious() const noexcept {
        return currentStep_ > 0;
    }

    bool DebuggerSession::moveNext() noexcept {
        if (!canMoveNext()) {
            return false;
        }

        ++currentStep_;
        return true;
    }

    bool DebuggerSession::movePrevious() noexcept {
        if (!canMovePrevious()) {
            return false;
        }

        --currentStep_;
        return true;
    }

    void DebuggerSession::restart() noexcept {
        currentStep_ = 0;
    }

    const quantum::QuantumRegister &DebuggerSession::initialState() const noexcept {
        return initialState_;
    }

    const quantum::QuantumRegister &DebuggerSession::stateBeforeCurrentStep() const noexcept {
        if (currentStep_ == 0) {
            return initialState_;
        }

        return trace_[currentStep_ - 1].state;
    }

    const circuit::CircuitInstructionInfo &DebuggerSession::currentInstruction() const {
        return instructions_.at(currentStep_);
    }

    DebuggerSnapshot DebuggerSession::snapshot() const {
        if (trace_.empty()) {
            return DebuggerSnapshot{
                .currentStepIndex = 0,
                .stepCount = 0,
                .instruction = std::nullopt,
                .beforeState = initialState_,
                .afterState = initialState_,
                .canMoveNext = false,
                .canMovePrevious = false
            };
        }
        return DebuggerSnapshot{
            currentStepIndex(),
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

        currentStep_ = index;
    }

    void DebuggerSession::rebuild(const circuit::QuantumCircuit &circuit,
                                  const quantum::QuantumRegister &initialState) {
        initialState_ = initialState;
        trace_ =
                circuit.executeWithTrace(initialState_);

        instructions_ = circuit.instructionInfo();

        currentStep_ = 0;
    }

    bool DebuggerSession::hasSteps() const noexcept {
        return !trace_.empty();
    }
}

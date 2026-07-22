#include "quantum_sim/debug/DebuggerSession.hpp"

namespace quantum_sim::debug {
    DebuggerSession::DebuggerSession(const circuit::QuantumCircuit &circuit,
                                     const quantum::QuantumRegister &initialState)
        : initialState_(initialState), trace_(circuit.executeWithTrace(initialState)) {
    }

    std::size_t DebuggerSession::stepCount() const noexcept {
        return trace_.size();
    }

    std::size_t DebuggerSession::currentStepIndex() const noexcept {
        return currentStep_;
    }

    const circuit::TraceStep &DebuggerSession::currentStep() const noexcept {
        return trace_.at(currentStep_);
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
}

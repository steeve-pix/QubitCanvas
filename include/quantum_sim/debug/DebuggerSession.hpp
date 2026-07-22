#pragma once
#include <cstddef>
#include <vector>

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"


namespace quantum_sim::debug {
    class DebuggerSession {
    public:
        DebuggerSession(const circuit::QuantumCircuit &circuit, const quantum::QuantumRegister &initialState);

        [[nodiscard]] std::size_t stepCount() const noexcept;

        [[nodiscard]] std::size_t currentStepIndex() const noexcept;

        [[nodiscard]] const circuit::TraceStep &currentStep() const noexcept;

        [[nodiscard]] bool canMoveNext() const noexcept;

        [[nodiscard]] bool canMovePrevious() const noexcept;

        bool moveNext() noexcept;

        bool movePrevious() noexcept;

        void restart() noexcept;

        [[nodiscard]] const quantum::QuantumRegister &initialState() const noexcept;

        [[nodiscard]] const quantum::QuantumRegister &stateBeforeCurrentStep() const noexcept;

    private:
        quantum::QuantumRegister initialState_;
        std::vector<circuit::TraceStep> trace_;
        std::size_t currentStep_{};
    };
}

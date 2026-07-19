#pragma once

#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"

#include <cstddef>
#include <vector>

namespace quantum_sim::circuit {
    class QuantumCircuit final {
    public:
        explicit QuantumCircuit(std::size_t qubitCount);

        [[nodiscard]] std::size_t qubitCount() const noexcept;

        void addSingleQubitGate(math::ComplexMatrix gate, std::size_t targetQubit);

        [[nodiscard]] std::size_t instructionCount() const noexcept;

        [[nodiscard]] quantum::QuantumRegister execute(const quantum::QuantumRegister &initialState);

    private:
        struct SingleQubitInstruction {
            math::ComplexMatrix gate;
            std::size_t targetQubit;
        };

        std::size_t qubitCount_;
        std::vector<SingleQubitInstruction> instructions_;
    };
}

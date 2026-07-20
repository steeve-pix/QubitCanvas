#pragma once

#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"

#include <cstddef>
#include <vector>
#include <variant>
#include <random>
#include <string>

namespace quantum_sim::circuit {
    class QuantumCircuit final {
    public:
        explicit QuantumCircuit(std::size_t qubitCount);

        [[nodiscard]] std::size_t qubitCount() const noexcept;

        void addSingleQubitGate(math::ComplexMatrix gate, std::size_t targetQubit);

        void addFullRegisterGate(math::ComplexMatrix gate);

        [[nodiscard]] std::size_t instructionCount() const noexcept;

        [[nodiscard]] quantum::QuantumRegister execute(const quantum::QuantumRegister &initialState) const;

        [[nodiscard]] std::vector<std::size_t> runShots(const quantum::QuantumRegister &initialState,
                                                        std::size_t shotCount, std::mt19937 &randomEngine) const;

    private:
        struct SingleQubitInstruction {
            math::ComplexMatrix gate;
            std::size_t targetQubit;
        };

        struct FullRegisterInstruction {
            math::ComplexMatrix gate;
        };

        using Instruction = std::variant<SingleQubitInstruction, FullRegisterInstruction>;

        std::size_t qubitCount_;
        std::vector<Instruction> instructions_;
    };
}

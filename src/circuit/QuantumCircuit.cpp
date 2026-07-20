#include "quantum_sim/circuit/QuantumCircuit.hpp"

#include <stdexcept>
#include <utility>
#include <type_traits>
#include <variant>
#include <string>

namespace quantum_sim::circuit {
    QuantumCircuit::QuantumCircuit(std::size_t qubitCount) : qubitCount_(qubitCount) {
        if (qubitCount == 0) {
            throw std::invalid_argument{"Can't be 0"};
        }
    }

    std::size_t QuantumCircuit::qubitCount() const noexcept {
        return qubitCount_;
    }

    void QuantumCircuit::addSingleQubitGate(math::ComplexMatrix gate, std::size_t targetQubit) {
        if (gate.rows() != 2 || gate.columns() != 2) {
            throw std::invalid_argument{"A single-qubit gate must be a 2 by 2 matrix."};
        }
        if (!gate.isUnitary()) {
            throw std::invalid_argument{"A quantum gate must be unitary."};
        }

        if (targetQubit >= qubitCount_) {
            throw std::out_of_range{"Target qubit index is outside the circuit."};
        }

        instructions_.push_back(SingleQubitInstruction{std::move(gate), targetQubit});
    }

    void QuantumCircuit::addFullRegisterGate(math::ComplexMatrix gate) {
        const std::size_t expectedSize =
                std::size_t{1} << qubitCount_;
        if (gate.rows() != expectedSize || gate.columns() != expectedSize) {
            throw std::invalid_argument{"Full-register gate dimensions must match the circuit state count."};
        }

        if (!gate.isUnitary()) {
            throw std::invalid_argument{"A quantum gate must be unitary."};
        }

        instructions_.push_back(FullRegisterInstruction{std::move(gate)});
    }

    std::size_t QuantumCircuit::instructionCount() const noexcept {
        return instructions_.size();
    }

    quantum::QuantumRegister QuantumCircuit::execute(const quantum::QuantumRegister &initialState) const {
        if (initialState.qubitCount() != qubitCount_) {
            throw std::invalid_argument{"Register qubit count must match the circuit qubit count."};
        }

        quantum::QuantumRegister currentState = initialState;

        for (const Instruction &instruction: instructions_) {
            std::visit([&currentState]<typename T0>(const T0 &actualInstruction) {
                           using InstructionType =
                                   std::decay_t<T0>;

                           if constexpr (std::is_same_v<InstructionType, SingleQubitInstruction>) {
                               currentState =
                                       currentState.applySingleQubitGate(
                                           actualInstruction.gate, actualInstruction.targetQubit);
                           } else {
                               currentState = currentState.applyGate(actualInstruction.gate);
                           }
                       },
                       instruction);
        }

        return currentState;
    }

    std::vector<std::size_t> QuantumCircuit::runShots(const quantum::QuantumRegister &initialState,
                                                      std::size_t shotCount, std::mt19937 &randomEngine) const {
        std::vector<std::size_t> counts(initialState.stateCount(), 0);

        for (std::size_t shot = 0; shot < shotCount; ++shot) {
            quantum::QuantumRegister result = execute(initialState);
            const std::size_t measuredState = result.measure(randomEngine);

            ++counts[measuredState];
        }

        return counts;
    }

    std::vector<TraceStep> QuantumCircuit::executeWithTrace(const quantum::QuantumRegister &initialState) const {
        if (initialState.qubitCount() != qubitCount_) {
            throw std::invalid_argument{"Register qubit count must match the circuit qubit count."};
        }

        quantum::QuantumRegister currentState = initialState;
        std::vector<TraceStep> trace;
        trace.reserve(instructions_.size());
        for (const Instruction &instruction: instructions_) {
            std::visit([&currentState,&trace](const auto &actualInstruction) {
                using InstructionType = std::decay_t<decltype(actualInstruction)>;

                std::string description;

                if constexpr (std::is_same_v<InstructionType, SingleQubitInstruction>) {
                    currentState = currentState.applySingleQubitGate(actualInstruction.gate,
                                                                     actualInstruction.targetQubit);
                    description = "Single-qubit gate on qubit " + std::to_string(actualInstruction.targetQubit);
                } else {
                    currentState = currentState.applyGate(actualInstruction.gate);
                    description = "Full-register gate";
                }
                trace.push_back(TraceStep{std::move(description), currentState});
            }, instruction);
        }

        return trace;
    }
}

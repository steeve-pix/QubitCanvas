#include "quantum_sim/circuit/QuantumCircuit.hpp"

#include <stdexcept>
#include <utility>

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

    std::size_t QuantumCircuit::instructionCount() const noexcept {
        return instructions_.size();
    }

    quantum::QuantumRegister QuantumCircuit::execute(const quantum::QuantumRegister &initialState) {
        if (initialState.qubitCount() != qubitCount_) {
            throw std::invalid_argument{"Register qubit count must match the circuit qubit count."};
        }

        quantum::QuantumRegister currentState = initialState;

        for (const SingleQubitInstruction &instruction: instructions_) {
            currentState =
                currentState.applySingleQubitGate(instruction.gate, instruction.targetQubit);
        }

        return currentState;
    }
}

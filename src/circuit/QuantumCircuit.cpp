#include "quantum_sim/circuit/QuantumCircuit.hpp"

#include <stdexcept>
#include <utility>
#include <type_traits>
#include <variant>
#include <string>
#include <optional>
#include <algorithm>
#include <cstddef>

namespace quantum_sim::circuit {
    QuantumCircuit::QuantumCircuit(std::size_t qubitCount) : qubitCount_(qubitCount) {
        if (qubitCount == 0) {
            throw std::invalid_argument{"Can't be 0"};
        }
    }

    std::size_t QuantumCircuit::qubitCount() const noexcept {
        return qubitCount_;
    }

    void QuantumCircuit::addSingleQubitGate(
        std::string name,
        math::ComplexMatrix gate,
        const std::size_t targetQubit,
        const std::optional<double> angleRadians
    ) {
        if (gate.rows() != 2 || gate.columns() != 2) {
            throw std::invalid_argument{"A single-qubit gate must be a 2 by 2 matrix."};
        }
        if (!gate.isUnitary()) {
            throw std::invalid_argument{"A quantum gate must be unitary."};
        }

        if (targetQubit >= qubitCount_) {
            throw std::out_of_range{"Target qubit index is outside the circuit."};
        }

        instructions_.push_back(
            SingleQubitInstruction{
                std::move(name),
                std::move(gate),
                targetQubit,
                angleRadians
            }
        );
    }

    void QuantumCircuit::addTwoQubitGate(
        std::string name,
        math::ComplexMatrix gate,
        const std::size_t firstQubit,
        const std::size_t secondQubit,
        const std::optional<double> angleRadians
    ) {
        if (gate.rows() != 4U || gate.columns() != 4U) {
            throw std::invalid_argument{"A two-qubit gate must be a 4 by 4 matrix."};
        }

        if (!gate.isUnitary()) {
            throw std::invalid_argument{"A quantum gate must be unitary."};
        }

        if (firstQubit >= qubitCount_ || secondQubit >= qubitCount_) {
            throw std::out_of_range{"Two-qubit gate index is outside the circuit."};
        }

        if (firstQubit == secondQubit) {
            throw std::invalid_argument{"A two-qubit gate requires two different qubits."};
        }

        instructions_.push_back(
            TwoQubitInstruction{
                std::move(name),
                std::move(gate),
                firstQubit,
                secondQubit,
                angleRadians
            }
        );
    }

    void QuantumCircuit::addReflection(
        std::string name,
        math::ComplexVector normalizedAxis,
        const std::size_t displayQubit
    ) {
        const std::size_t expectedSize =
                std::size_t{1} << qubitCount_;

        if (
            normalizedAxis.size() != expectedSize ||
            !normalizedAxis.isNormalized()
        ) {
            throw std::invalid_argument{
                "Reflection axis must be a normalized full-register vector."
            };
        }

        if (displayQubit >= qubitCount_) {
            throw std::out_of_range{
                "Reflection display qubit is outside the circuit."
            };
        }

        instructions_.push_back(
            ReflectionInstruction{
                std::move(name),
                std::move(normalizedAxis),
                displayQubit
            }
        );
    }

    void QuantumCircuit::addFullRegisterGate(std::string name, math::ComplexMatrix gate) {
        const std::size_t expectedSize =
                std::size_t{1} << qubitCount_;
        if (gate.rows() != expectedSize || gate.columns() != expectedSize) {
            throw std::invalid_argument{"Full-register gate dimensions must match the circuit state count."};
        }

        if (!gate.isUnitary()) {
            throw std::invalid_argument{"A quantum gate must be unitary."};
        }

        instructions_.push_back(FullRegisterInstruction{
            std::move(name),
            std::move(gate),
            std::nullopt,
            std::nullopt
        });
    }

    std::size_t QuantumCircuit::instructionCount() const noexcept {
        return instructions_.size();
    }

    quantum::QuantumRegister QuantumCircuit::execute(const quantum::QuantumRegister &initialState) const {
        if (initialState.qubitCount() != qubitCount_) {
            throw std::invalid_argument{"Register qubit count must match the circuit qubit count."};
        }

        quantum::QuantumRegister currentState = initialState;

        // Instructions store either a single-qubit gate or a full-register gate;
        // visit selects the correct execution path without exposing the variant.
        for (const Instruction &instruction: instructions_) {
            std::visit([&currentState]<typename T0>(const T0 &actualInstruction) {
                           using InstructionType =
                                   std::decay_t<T0>;

                           if constexpr (std::is_same_v<InstructionType, SingleQubitInstruction>) {
                               currentState =
                                       currentState.applySingleQubitGate(
                                           actualInstruction.gate, actualInstruction.targetQubit);
                           } else if constexpr (std::is_same_v<InstructionType, TwoQubitInstruction>) {
                               currentState =
                                       currentState.applyTwoQubitGate(
                                           actualInstruction.gate,
                                           actualInstruction.firstQubit,
                                           actualInstruction.secondQubit
                                       );
                           } else if constexpr (std::is_same_v<InstructionType, ReflectionInstruction>) {
                               currentState =
                                       currentState.applyReflection(
                                           actualInstruction.normalizedAxis
                                       );
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

        // Each shot starts from the same initial state, executes the circuit,
        // then measures and increments the observed basis-state bucket.
        for (std::size_t shot = 0; shot < shotCount; ++shot) {
            quantum::QuantumRegister result = execute(initialState);
            const std::size_t measuredState = result.measure(randomEngine);

            ++counts[measuredState];
        }

        return counts;
    }

    std::vector<TraceStep> QuantumCircuit::executeWithTrace(
        const quantum::QuantumRegister &initialState
    ) const {
        return executeWithTraceFrom(initialState, 0U);
    }

    std::vector<TraceStep> QuantumCircuit::executeWithTraceFrom(
        const quantum::QuantumRegister &stateBeforeFirstInstruction,
        const std::size_t firstInstructionIndex
    ) const {
        if (stateBeforeFirstInstruction.qubitCount() != qubitCount_) {
            throw std::invalid_argument{"Register qubit count must match the circuit qubit count."};
        }

        if (firstInstructionIndex > instructions_.size()) {
            throw std::out_of_range{
                "First trace instruction index is outside the circuit."
            };
        }

        quantum::QuantumRegister currentState =
                stateBeforeFirstInstruction;

        std::vector<TraceStep> trace;
        trace.reserve(
            instructions_.size() - firstInstructionIndex
        );

        // The trace records the state after each instruction so the debugger can
        // show both before/after state without re-executing the circuit per frame.
        for (
            auto instructionIterator =
                    instructions_.begin() +
                    static_cast<std::ptrdiff_t>(firstInstructionIndex);
            instructionIterator != instructions_.end();
            ++instructionIterator
        ) {
            const Instruction &instruction =
                    *instructionIterator;

            std::visit([&currentState,&trace]<typename T0>(const T0 &actualInstruction) {
                using InstructionType = std::decay_t<T0>;

                std::string description;

                if constexpr (std::is_same_v<InstructionType, SingleQubitInstruction>) {
                    currentState = currentState.applySingleQubitGate(actualInstruction.gate,
                                                                     actualInstruction.targetQubit);
                    description = actualInstruction.name + " on qubit " + std::to_string(actualInstruction.targetQubit);

                    if (actualInstruction.angleRadians.has_value()) {
                        description +=
                                " at " +
                                std::to_string(actualInstruction.angleRadians.value()) +
                                " radians";
                    }
                } else if constexpr (std::is_same_v<InstructionType, TwoQubitInstruction>) {
                    currentState = currentState.applyTwoQubitGate(
                        actualInstruction.gate,
                        actualInstruction.firstQubit,
                        actualInstruction.secondQubit
                    );
                    description =
                            actualInstruction.name +
                            " on qubits " +
                            std::to_string(actualInstruction.firstQubit) +
                            " and " +
                            std::to_string(actualInstruction.secondQubit);

                    if (actualInstruction.angleRadians.has_value()) {
                        description +=
                                " at " +
                                std::to_string(actualInstruction.angleRadians.value()) +
                                " radians";
                    }
                } else if constexpr (std::is_same_v<InstructionType, ReflectionInstruction>) {
                    currentState =
                            currentState.applyReflection(
                                actualInstruction.normalizedAxis
                            );

                    description =
                            actualInstruction.name;
                } else {
                    currentState = currentState.applyGate(actualInstruction.gate);
                    description = actualInstruction.name;
                }
                trace.push_back(TraceStep{std::move(description), currentState});
            }, instruction);
        }

        return trace;
    }

    std::vector<CircuitInstructionInfo> QuantumCircuit::instructionInfo() const {
        std::vector<CircuitInstructionInfo> result;
        result.reserve(instructions_.size());

        // Convert private executable instructions into lightweight metadata for
        // renderers, debugger panels, and console diagrams.
        for (const Instruction &instruction: instructions_) {
            std::visit([&result]<typename T0>(const T0 &actualInstruction) {
                using InstructionType = std::decay_t<decltype(actualInstruction)>;

                if constexpr (std::is_same_v<InstructionType, SingleQubitInstruction>) {
                    result.push_back(CircuitInstructionInfo{
                        actualInstruction.name,
                        CircuitInstructionKind::SingleQubit,
                        actualInstruction.targetQubit,
                        std::nullopt,
                        std::nullopt,
                        actualInstruction.angleRadians
                    });
                } else if constexpr (std::is_same_v<InstructionType, TwoQubitInstruction>) {
                    result.push_back(CircuitInstructionInfo{
                        actualInstruction.name,
                        CircuitInstructionKind::TwoQubit,
                        std::nullopt,
                        actualInstruction.firstQubit,
                        actualInstruction.secondQubit,
                        actualInstruction.angleRadians
                    });
                } else if constexpr (std::is_same_v<InstructionType, ReflectionInstruction>) {
                    result.push_back(CircuitInstructionInfo{
                        actualInstruction.name,
                        CircuitInstructionKind::Reflection,
                        actualInstruction.displayQubit,
                        std::nullopt,
                        std::nullopt,
                        std::nullopt
                    });
                } else {
                    result.push_back(CircuitInstructionInfo{
                        actualInstruction.name,
                        CircuitInstructionKind::FullRegister,
                        std::nullopt,
                        actualInstruction.controlQubit,
                        actualInstruction.targetQubit,
                        std::nullopt
                    });
                }
            }, instruction);
        }

        return result;
    }

    void QuantumCircuit::addControlledGate(std::string name, math::ComplexMatrix gate, std::size_t controlQubit,
                                           std::size_t targetQubit) {
        if (controlQubit >= qubitCount_ || targetQubit >= qubitCount_) {
            throw std::out_of_range{"Controlled-gate qubit index is outside the circuit."};
        }
        if (controlQubit == targetQubit) {
            throw std::invalid_argument{"Control and target qubits must be different."};
        }

        const std::size_t expectedSize =
                std::size_t{1} << qubitCount_;

        if (gate.rows() != expectedSize || gate.columns() != expectedSize) {
            throw std::invalid_argument{"Controlled-gate dimensions must match the circuit state count."};
        }

        if (!gate.isUnitary()) {
            throw std::invalid_argument{"A quantum gate must be unitary."};
        }

        instructions_.push_back(
            FullRegisterInstruction{
                std::move(name),
                std::move(gate),
                controlQubit,
                targetQubit
            });
    }

    bool QuantumCircuit::removeLastInstruction() noexcept {
        if (instructions_.empty()) {
            return false;
        }

        instructions_.pop_back();
        return true;
    }

    bool QuantumCircuit::removeInstruction(std::size_t index) {
        if (index >= instructions_.size()) {
            return false;
        }

        instructions_.erase(instructions_.begin() + static_cast<ptrdiff_t>(index));

        return true;
    }

    bool QuantumCircuit::moveInstruction(
        const std::size_t fromIndex,
        const std::size_t toIndex
    ) {
        if (
            fromIndex >= instructions_.size() ||
            toIndex >= instructions_.size() ||
            fromIndex == toIndex
        ) {
            return false;
        }

        Instruction movedInstruction =
                std::move(instructions_[fromIndex]);

        instructions_.erase(
            instructions_.begin() +
            static_cast<std::ptrdiff_t>(fromIndex)
        );

        instructions_.insert(
            instructions_.begin() +
            static_cast<std::ptrdiff_t>(toIndex),
            std::move(movedInstruction)
        );

        return true;
    }

    void QuantumCircuit::insertSingleQubitGate(
        const std::size_t instructionIndex,
        std::string name,
        math::ComplexMatrix gate,
        const std::size_t targetQubit,
        const std::optional<double> angleRadians
    ) {
        if (gate.rows() != 2 || gate.columns() != 2) {
            throw std::invalid_argument{"A single-qubit gate must be a 2 by 2 matrix."};
        }
        if (!gate.isUnitary()) {
            throw std::invalid_argument{"A quantum gate must be unitary."};
        }

        if (targetQubit >= qubitCount_) {
            throw std::out_of_range{"Target qubit index is outside the circuit."};
        }

        const std::size_t clampedIndex =
                std::min(
                    instructionIndex,
                    instructions_.size()
                );

        // Insertion positions beyond the end behave like append, which keeps
        // drag/placement code forgiving.
        instructions_.insert(
            instructions_.begin()
            + static_cast<ptrdiff_t>(clampedIndex),
            SingleQubitInstruction{
                std::move(name),
                std::move(gate),
                targetQubit,
                angleRadians
            }
        );
    }

    void QuantumCircuit::insertTwoQubitGate(
        const std::size_t instructionIndex,
        std::string name,
        math::ComplexMatrix gate,
        const std::size_t firstQubit,
        const std::size_t secondQubit,
        const std::optional<double> angleRadians
    ) {
        if (gate.rows() != 4U || gate.columns() != 4U) {
            throw std::invalid_argument{"A two-qubit gate must be a 4 by 4 matrix."};
        }

        if (!gate.isUnitary()) {
            throw std::invalid_argument{"A quantum gate must be unitary."};
        }

        if (firstQubit >= qubitCount_ || secondQubit >= qubitCount_) {
            throw std::out_of_range{"Two-qubit gate index is outside the circuit."};
        }

        if (firstQubit == secondQubit) {
            throw std::invalid_argument{"A two-qubit gate requires two different qubits."};
        }

        const std::size_t clampedIndex =
                std::min(instructionIndex, instructions_.size());

        instructions_.insert(
            instructions_.begin() + static_cast<ptrdiff_t>(clampedIndex),
            TwoQubitInstruction{
                std::move(name),
                std::move(gate),
                firstQubit,
                secondQubit,
                angleRadians
            }
        );
    }

    void QuantumCircuit::insertControlledGate(std::size_t instructionIndex, std::string name, math::ComplexMatrix gate,
                                              std::size_t controlQubit, std::size_t targetQubit) {
        if (controlQubit >= qubitCount_ || targetQubit >= qubitCount_) {
            throw std::out_of_range{"Controlled-gate qubit index is outside the circuit."};
        }
        if (controlQubit == targetQubit) {
            throw std::invalid_argument{"Control and target qubits must be different."};
        }

        const std::size_t expectedSize =
                std::size_t{1} << qubitCount_;

        if (gate.rows() != expectedSize || gate.columns() != expectedSize) {
            throw std::invalid_argument{"Controlled-gate dimensions must match the circuit state count."};
        }

        if (!gate.isUnitary()) {
            throw std::invalid_argument{"A quantum gate must be unitary."};
        }

        const std::size_t clampedIndex =
                std::min(instructionIndex, instructions_.size());

        // Controlled and swap-style gates are stored as full-register matrices
        // plus qubit metadata for renderers.
        instructions_.insert(
            instructions_.begin()
            + static_cast<ptrdiff_t>(clampedIndex),
            FullRegisterInstruction{
                std::move(name),
                std::move(gate),
                controlQubit,
                targetQubit
            });
    }
}

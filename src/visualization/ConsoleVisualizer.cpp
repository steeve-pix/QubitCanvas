#include "quantum_sim/visualization/ConsoleVisualizer.hpp"

#include <algorithm>
#include <format>
#include <iomanip>
#include <ostream>
#include <string>
#include <stdexcept>
#include <cmath>

namespace quantum_sim::visualization {
    void printProbabilityBars(const quantum::QuantumRegister &state, std::ostream &output, std::size_t barWidth) {
        for (const quantum::StateInfo &info: state.states()) {
            const double clampedProbability =
                    std::clamp(info.probability, 0.0, 1.0);

            const std::size_t filledWidth =
                    std::min(barWidth, static_cast<std::size_t>(std::llround(
                                 clampedProbability * static_cast<double>(barWidth))));

            const std::string filled(filledWidth, '#');

            const std::string empty(barWidth - filledWidth, ' ');

            output
                    << info.label
                    << " ["
                    << filled
                    << empty
                    << "] "
                    << std::fixed
                    << std::setprecision(2)
                    << clampedProbability * 100.0
                    << "%\n";
        }
    }

    void printShotBars(const quantum::QuantumRegister &state, const std::vector<std::size_t> &counts,
                       std::ostream &output, std::size_t barWidth) {
        if (counts.size() != state.stateCount()) {
            throw std::invalid_argument{"Shot count must match the register state count."};
        }

        std::size_t totalShots = 0;
        for (const std::size_t count: counts) {
            totalShots += count;
        }
        for (std::size_t stateIndex = 0;
             stateIndex < state.stateCount();
             ++stateIndex) {
            const double frequency =
                    totalShots == 0
                        ? 0.0
                        : static_cast<double>(counts[stateIndex])
                          / static_cast<double>(totalShots);

            const std::size_t filledWidth =
                    static_cast<std::size_t>(
                        frequency * static_cast<double>(barWidth)
                    );

            const std::string filled(filledWidth, '#');
            const std::string empty(
                barWidth - filledWidth,
                ' '
            );

            output
                    << state.basisStateLabel(stateIndex)
                    << " ["
                    << filled
                    << empty
                    << "] "
                    << counts[stateIndex]
                    << " ("
                    << std::fixed
                    << std::setprecision(2)
                    << frequency * 100.0
                    << "%)\n";
        }
    }

    void printExecutionTrace(const quantum::QuantumRegister &initialState, const std::vector<circuit::TraceStep> &trace,
                             std::ostream &output, std::size_t barWidth) {
        output << "Initial state:\n";
        printProbabilityBars(initialState, output, barWidth);

        for (const circuit::TraceStep &step: trace) {
            output << "\nAfter " << step.description << ":\n";

            printProbabilityBars(step.state, output, barWidth);
        }
    }

    void printCircuitDiagram(const circuit::QuantumCircuit &circuit, std::ostream &output,
                             std::optional<std::size_t> currentInstruction) {
        const std::vector<circuit::CircuitInstructionInfo> instructions =
                circuit.instructionInfo();

        for (std::size_t qubit = 0; qubit < circuit.qubitCount(); ++qubit) {
            output
                    << "q"
                    << qubit
                    << ": |0> ";

            for (std::size_t instructionIndex = 0; instructionIndex < instructions.size(); ++instructionIndex) {
                const circuit::CircuitInstructionInfo &instruction =
                        instructions[instructionIndex];

                const bool isCurrentInstruction =
                        currentInstruction == instructionIndex;

                if (instruction.kind == circuit::CircuitInstructionKind::SingleQubit) {
                    if (instruction.targetQubit.value() == qubit) {
                        output << (isCurrentInstruction ? "-[" : "--") << instruction.name << (
                            isCurrentInstruction ? "]-" : "--");
                    } else {
                        output << "-----";
                    }
                } else {
                    if (instruction.controlQubit.has_value() && instruction.secondaryTargetQubit.has_value()) {
                        if (qubit == instruction.controlQubit.value()) {
                            output << (isCurrentInstruction ? "-[C]-" : "--C--");
                        } else if (qubit == instruction.secondaryTargetQubit.value()
                        ) {
                            output << (isCurrentInstruction ? "-[X]-" : "--X--");
                        } else {
                            output << "-----";
                        }
                    } else {
                        output
                                << "--"
                                << instruction.name
                                << "--";
                    }
                }
            }

            output << '\n';
        }

        for (const circuit::CircuitInstructionInfo &instruction: instructions) {
            if (instruction.controlQubit.has_value() && instruction.secondaryTargetQubit.has_value()) {
                output
                        << "            control q"
                        << instruction.controlQubit.value()
                        << " --> target q"
                        << instruction.secondaryTargetQubit.value()
                        << '\n';
            }
        }
    }

    void printAmplitudes(const quantum_sim::quantum::QuantumRegister &state, std::ostream &output) {
        for (const quantum::StateInfo &info: state.states()) {
            output
                    << info.label
                    << ": "
                    << info.amplitude.real()
                    << (info.amplitude.imaginary() < 0 ? " - " : " + ")
                    << std::abs(info.amplitude.imaginary())
                    << "i\n";
        }
    }
}

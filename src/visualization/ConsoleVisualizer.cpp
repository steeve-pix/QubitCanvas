#include "quantum_sim/visualization/ConsoleVisualizer.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <string>
#include <stdexcept>
#include <cmath>
#include <numbers>
#include <sstream>
#include <vector>

namespace {
    std::string formatPhaseAsPi(double phase) {
        constexpr double epsilon = 1e-10;
        constexpr double pi = std::numbers::pi;

        // Prefer familiar exact labels for common quantum phases.
        struct KnownPhase {
            double value;
            const char *text;
        };

        constexpr KnownPhase knownPhases[] = {
            {0.0, "0.00"},
            {pi / 4.0, "pi/4"},
            {pi / 2.0, "pi/2"},
            {3.0 * pi / 4.0, "3pi/4"},
            {pi, "pi"},
            {-pi / 4.0, "-pi/4"},
            {-pi / 2.0, "-pi/2"},
            {-3.0 * pi / 4.0, "-3pi/4"},
            {-pi, "-pi"}
        };

        for (const auto &knownPhase: knownPhases) {
            if (std::abs(phase - knownPhase.value) < epsilon) {
                return knownPhase.text;
            }
        }

        // Fall back to a decimal value for arbitrary rotations.
        std::ostringstream output;
        output << std::fixed << std::setprecision(2) << phase;
        return output.str();
    }
}

namespace quantum_sim::visualization {
    void printProbabilityBars(const quantum::QuantumRegister &state, std::ostream &output, std::size_t barWidth) {
        for (const quantum::StateInfo &info: state.states()) {
            // Clamp protects the text renderer from tiny floating-point drift.
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

        // Convert raw counts to frequencies before scaling the bar width.
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

        // Each qubit gets one row; each instruction writes one fixed-width cell.
        for (std::size_t qubit = 0; qubit < circuit.qubitCount(); ++qubit) {
            output
                    << "q"
                    << qubit
                    << ": |0\xE2\x9F\xA9 ";

            for (std::size_t instructionIndex = 0; instructionIndex < instructions.size(); ++instructionIndex) {
                const circuit::CircuitInstructionInfo &instruction =
                        instructions[instructionIndex];

                const bool isCurrentInstruction =
                        currentInstruction == instructionIndex;

                if (instruction.kind == circuit::CircuitInstructionKind::SingleQubit) {
                    // Single-qubit gates draw only on their target row.
                    if (instruction.targetQubit.value() == qubit) {
                        output << (isCurrentInstruction ? "-[" : "--") << instruction.name << (
                            isCurrentInstruction ? "]-" : "--");
                    } else {
                        output << "-----";
                    }
                } else {
                    if (
                        instruction.controlQubit.has_value() &&
                        instruction.secondaryTargetQubit.has_value() &&
                        instruction.tertiaryTargetQubit.has_value()
                    ) {
                        const bool controlledSwap =
                                instruction.name == "CSWAP";

                        if (qubit == instruction.controlQubit.value()) {
                            output << (isCurrentInstruction ? "-[C]-" : "--C--");
                        } else if (
                            qubit == instruction.secondaryTargetQubit.value()
                        ) {
                            output <<
                                    (
                                        controlledSwap
                                            ? (
                                                isCurrentInstruction
                                                    ? "-[S]-"
                                                    : "--S--"
                                            )
                                            : (
                                                isCurrentInstruction
                                                    ? "-[C]-"
                                                    : "--C--"
                                            )
                                    );
                        } else if (
                            qubit == instruction.tertiaryTargetQubit.value()
                        ) {
                            output <<
                                    (
                                        controlledSwap
                                            ? (
                                                isCurrentInstruction
                                                    ? "-[S]-"
                                                    : "--S--"
                                            )
                                            : (
                                                isCurrentInstruction
                                                    ? "-[X]-"
                                                    : "--X--"
                                            )
                                    );
                        } else {
                            output << "-----";
                        }
                    } else if (
                        instruction.controlQubit.has_value() &&
                        instruction.secondaryTargetQubit.has_value()
                    ) {
                        // Two-qubit controlled gates draw one marker per operand.
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
            if (
                instruction.controlQubit.has_value() &&
                instruction.secondaryTargetQubit.has_value() &&
                instruction.tertiaryTargetQubit.has_value()
            ) {
                output
                        << "            operands q"
                        << instruction.controlQubit.value()
                        << ", q"
                        << instruction.secondaryTargetQubit.value()
                        << ", q"
                        << instruction.tertiaryTargetQubit.value()
                        << '\n';
            } else if (
                instruction.controlQubit.has_value() &&
                instruction.secondaryTargetQubit.has_value()
            ) {
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

    void printStateComparison(const quantum::QuantumRegister &beforeState, const quantum::QuantumRegister &afterState,
                              std::ostream &output) {
        if (beforeState.stateCount() != afterState.stateCount()) {
            throw std::invalid_argument{"Compared quantum states must have the same size."};
        }

        const auto beforeStates = beforeState.states();
        const auto afterStates = afterState.states();

        // Compare matching basis states so probability and phase changes line up.
        for (std::size_t stateIndex = 0; stateIndex < beforeStates.size(); ++stateIndex) {
            const double beforeProbability =
                    beforeStates[stateIndex].probability;

            const double afterProbability =
                    afterStates[stateIndex].probability;

            const double change = afterProbability - beforeProbability;
            constexpr double epsilon = 1e-10;

            const auto &beforeAmplitude =
                    beforeStates[stateIndex].amplitude;
            const auto &afterAmplitude =
                    afterStates[stateIndex].amplitude;

            const bool amplitudeChanged =
                    std::abs(afterAmplitude.real() - beforeAmplitude.real()) > epsilon || std::abs(
                        afterAmplitude.imaginary() - beforeAmplitude.imaginary()) > epsilon;

            const double beforePhase =
                    std::atan2(beforeAmplitude.imaginary(), beforeAmplitude.real());

            const double afterPhase =
                    std::atan2(afterAmplitude.imaginary(), afterAmplitude.real());

            output
                    << beforeStates[stateIndex].label
                    << ": "
                    << std::fixed
                    << std::setprecision(2)
                    << beforeProbability * 100.0
                    << "% -> "
                    << afterProbability * 100.0
                    << "%";

            if (change > 0.0) {
                output << " (+" << change * 100.0 << "%)";
            } else if (change < 0.0) {
                output << " (" << change * 100.0 << "%)";
            } else if (amplitudeChanged) {
                output << " (amplitude changed, probability unchanged)";
            } else {
                output << " (unchanged)";
            }
            output << '\n';
            if (amplitudeChanged) {
                // Show amplitude and phase only when probability does not tell
                // the whole story, especially for phase-only gates.
                const auto printAmplitude = [&output]<typename T0>(const T0 &amplitude) {
                    output
                            << amplitude.real()
                            << (amplitude.imaginary() < 0.0 ? " - " : " + ")
                            << std::abs(amplitude.imaginary())
                            << 'i';
                };

                output << "  Amplitude: ";

                printAmplitude(beforeAmplitude);

                output << " -> ";

                printAmplitude(afterAmplitude);

                const std::string beforePhaseText =
                        formatPhaseAsPi(beforePhase);

                const std::string afterPhaseText =
                        formatPhaseAsPi(afterPhase);

                output
                        << "\n  Phase: "
                        << beforePhaseText
                        << " rad -> "
                        << afterPhaseText
                        << " rad"
                        << '\n';
            }
            output << '\n';
        }
    }

    void printBlochVector(const quantum::QuantumRegister &state, std::ostream &output) {
        const quantum::BlochVector vector = state.blockVector();
        const quantum::BlochAngles angles = state.blochAngles();

        output
                << "Bloch vector:\n"
                << std::fixed << std::setprecision(2)
                << "("
                << vector.x
                << ", "
                << vector.y
                << ", "
                << vector.z
                << ")\n";
        output
                << "(theta, phi) -> ("
                << formatPhaseAsPi(angles.theta)
                << ", ";

        constexpr double epsilon = 1e-10;

        const bool isAtPole =
                std::abs(vector.x) < epsilon && std::abs(vector.y) < epsilon;

        // Phi is undefined at the poles because the vector has no X/Y direction.
        if (isAtPole) {
            output << "NaN) rad\n";
        } else {
            output
                    << formatPhaseAsPi(angles.phi)
                    << ") rad\n";
        }
    }

    void printAsciiBlochSphere(const quantum::QuantumRegister &state, std::ostream &output) {
        const quantum::BlochVector vector =
                state.blockVector();

        constexpr double centerRow = 5.0;
        constexpr double centerColumn = 14.0;

        constexpr double verticalRadius = 3.0;
        constexpr double horizontalRadius = 9.0;

        const double clampedX =
                std::clamp(vector.x, -1.0, 1.0);

        const double clampedZ =
                std::clamp(vector.z, -1.0, 1.0);

        // Project X/Z coordinates onto a fixed ASCII ellipse.
        const std::size_t markerRow =
                static_cast<std::size_t>(std::lround(centerRow - clampedZ * verticalRadius));

        const std::size_t markerColumn =
                static_cast<std::size_t>(std::lround(centerColumn + clampedX * horizontalRadius));
        std::vector<std::string> canvas{
            "          +Z |0\xE2\x9F\xA9",
            "             |",
            "        .-----------.",
            "      .'             '.",
            "     /                 \\",
            "-X  |         +         |  +X",
            "     \\                 /",
            "      '.             .'",
            "        '-----------'",
            "             |",
            "          -Z |1\xE2\x9F\xA9"
        };

        constexpr std::size_t canvasWidth = 29;
        for (std::string &row: canvas) {
            row.resize(canvasWidth, ' ');
        }

        canvas.at(markerRow).at(markerColumn) = 'Q';

        constexpr double epsilon = 1e-10;

        output << (vector.y > epsilon
                       ? "Y direction: +Y, out of the screen\n"
                       : (vector.y < -epsilon
                              ? "Y direction: -Y, into the screen\n"
                              : "Y direction: 0, in the X-Z plane\n"));

        for (const std::string &row: canvas) {
            output << row << '\n';
        }
    }
}

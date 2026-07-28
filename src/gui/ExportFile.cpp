#include "quantum_sim/gui/ExportFile.hpp"

#include "quantum_sim/gui/GateNotation.hpp"
#include "quantum_sim/gui/QuantumNotation.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <string>

namespace quantum_sim::gui {
    namespace {
        std::string xmlEscape(const std::string &value) {
            std::string escaped;
            escaped.reserve(value.size());

            for (const char character : value) {
                switch (character) {
                    case '&': escaped += "&amp;"; break;
                    case '<': escaped += "&lt;"; break;
                    case '>': escaped += "&gt;"; break;
                    case '"': escaped += "&quot;"; break;
                    case '\'': escaped += "&apos;"; break;
                    default: escaped += character; break;
                }
            }

            return escaped;
        }

        void requireWritable(const std::ofstream &output) {
            if (!output) {
                throw std::runtime_error{
                    "Unable to open the export destination."
                };
            }
        }
    }

    void ExportFile::saveCircuitSvg(
        const std::filesystem::path &path,
        const circuit::QuantumCircuit &circuit
    ) {
        std::ofstream output{
            path,
            std::ios::trunc
        };

        requireWritable(output);

        constexpr double left = 92.0;
        constexpr double top = 74.0;
        constexpr double wireSpacing = 64.0;
        constexpr double gateSpacing = 88.0;

        const double width =
                std::max(
                    760.0,
                    left * 2.0 +
                    gateSpacing *
                    static_cast<double>(
                        circuit.instructionCount() + 1U
                    )
                );

        const double height =
                top * 2.0 +
                wireSpacing *
                static_cast<double>(
                    std::max<std::size_t>(
                        1U,
                        circuit.qubitCount() - 1U
                    )
                );

        output
            << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\""
            << width
            << "\" height=\""
            << height
            << "\" viewBox=\"0 0 "
            << width
            << ' '
            << height
            << "\">\n"
            << "<rect width=\"100%\" height=\"100%\" fill=\"#080d15\"/>\n"
            << "<g font-family=\"JetBrains Mono, monospace\" font-size=\"13\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\">\n";

        for (
            std::size_t qubit = 0U;
            qubit < circuit.qubitCount();
            ++qubit
        ) {
            const double y =
                    top +
                    static_cast<double>(qubit) *
                    wireSpacing;

            output
                << "<text x=\"24\" y=\""
                << y + 5.0
                << "\" fill=\"#a8b8cc\">q"
                << qubit
                << "</text>\n"
                << "<line x1=\""
                << left
                << "\" y1=\""
                << y
                << "\" x2=\""
                << width - 40.0
                << "\" y2=\""
                << y
                << "\" stroke=\"#7890a8\" stroke-width=\"1.5\"/>\n";
        }

        const auto instructions =
                circuit.instructionInfo();

        for (
            std::size_t instructionIndex = 0U;
            instructionIndex < instructions.size();
            ++instructionIndex
        ) {
            const auto &instruction =
                    instructions[instructionIndex];

            const double x =
                    left +
                    gateSpacing *
                    static_cast<double>(
                        instructionIndex + 1U
                    );

            output
                << "<text x=\""
                << x
                << "\" y=\"28\" text-anchor=\"middle\" fill=\"#70869e\">"
                << instructionIndex + 1U
                << "</text>\n";

            std::vector<std::size_t> operands;

            if (instruction.controlQubit.has_value()) {
                operands.push_back(
                    instruction.controlQubit.value()
                );
            }

            if (instruction.targetQubit.has_value()) {
                operands.push_back(
                    instruction.targetQubit.value()
                );
            }

            if (instruction.secondaryTargetQubit.has_value()) {
                operands.push_back(
                    instruction.secondaryTargetQubit.value()
                );
            }

            if (instruction.tertiaryTargetQubit.has_value()) {
                operands.push_back(
                    instruction.tertiaryTargetQubit.value()
                );
            }

            if (operands.empty()) {
                operands.push_back(0U);
            }

            const auto [minimumOperand, maximumOperand] =
                    std::minmax_element(
                        operands.begin(),
                        operands.end()
                    );

            if (operands.size() > 1U) {
                output
                    << "<line x1=\""
                    << x
                    << "\" y1=\""
                    << top +
                       static_cast<double>(*minimumOperand) *
                       wireSpacing
                    << "\" x2=\""
                    << x
                    << "\" y2=\""
                    << top +
                       static_cast<double>(*maximumOperand) *
                       wireSpacing
                    << "\" stroke=\"#b66bff\" stroke-width=\"2.2\"/>\n";
            }

            for (
                std::size_t operandIndex = 0U;
                operandIndex < operands.size();
                ++operandIndex
            ) {
                const double y =
                        top +
                        static_cast<double>(operands[operandIndex]) *
                        wireSpacing;

                const bool control =
                        operands.size() > 1U &&
                        operandIndex + 1U < operands.size();

                if (control) {
                    output
                        << "<circle cx=\""
                        << x
                        << "\" cy=\""
                        << y
                        << "\" r=\"5\" fill=\"#b66bff\"/>\n";
                    continue;
                }

                const std::string label =
                        xmlEscape(
                            std::string{
                                gate_notation::circuitLabel(
                                    instruction.name
                                )
                            }
                        );

                output
                    << "<rect x=\""
                    << x - 23.0
                    << "\" y=\""
                    << y - 20.0
                    << "\" width=\"46\" height=\"40\" rx=\"5\" "
                       "fill=\"#13283a\" stroke=\"#36c4f2\" stroke-width=\"1.8\"/>\n"
                    << "<text x=\""
                    << x
                    << "\" y=\""
                    << y + 5.0
                    << "\" text-anchor=\"middle\" fill=\"#edf8ff\">"
                    << label
                    << "</text>\n";
            }

            if (instruction.angleRadians.has_value()) {
                output
                    << "<text x=\""
                    << x
                    << "\" y=\"52\" text-anchor=\"middle\" fill=\"#8fa8c0\">"
                    << xmlEscape(
                        notation::formatRadians(
                            instruction.angleRadians.value(),
                            4,
                            false
                        )
                    )
                    << "</text>\n";
            }
        }

        output
            << "</g>\n"
            << "</svg>\n";

        if (!output) {
            throw std::runtime_error{
                "Writing the circuit SVG failed."
            };
        }
    }

    void ExportFile::saveStateCsv(
        const std::filesystem::path &path,
        const quantum::QuantumRegister &state
    ) {
        std::ofstream output{
            path,
            std::ios::trunc
        };

        requireWritable(output);

        output
            << std::setprecision(
                std::numeric_limits<double>::max_digits10
            )
            << "basis_index,ket,real,imaginary,magnitude,probability,phase_radians\n";

        for (
            std::size_t stateIndex = 0U;
            stateIndex < state.stateCount();
            ++stateIndex
        ) {
            const auto &amplitude =
                    state.amplitude(stateIndex);

            output
                << stateIndex
                << ",\""
                << state.basisStateLabel(stateIndex)
                << "\","
                << amplitude.real()
                << ','
                << amplitude.imaginary()
                << ','
                << amplitude.magnitude()
                << ','
                << state.probability(stateIndex)
                << ','
                << std::atan2(
                    amplitude.imaginary(),
                    amplitude.real()
                )
                << '\n';
        }

        if (!output) {
            throw std::runtime_error{
                "Writing the state CSV failed."
            };
        }
    }

    void ExportFile::saveDensityCsv(
        const std::filesystem::path &path,
        const density_volume::DensityLayer &layer
    ) {
        std::ofstream output{
            path,
            std::ios::trunc
        };

        requireWritable(output);

        output
            << std::setprecision(
                std::numeric_limits<double>::max_digits10
            )
            << "layer,row,column,real,imaginary,magnitude,intensity,phase_radians,bucketed\n";

        for (const auto &cell : layer.cells) {
            output
                << layer.index
                << ','
                << cell.row
                << ','
                << cell.column
                << ','
                << cell.real
                << ','
                << cell.imaginary
                << ','
                << cell.magnitude
                << ','
                << cell.intensity
                << ','
                << cell.phaseRadians
                << ','
                << (layer.bucketed ? 1 : 0)
                << '\n';
        }

        if (!output) {
            throw std::runtime_error{
                "Writing the density CSV failed."
            };
        }
    }
}

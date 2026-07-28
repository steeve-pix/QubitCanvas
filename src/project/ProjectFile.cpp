#include "quantum_sim/project/ProjectFile.hpp"

#include "quantum_sim/math/Complex.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/math/ComplexVector.hpp"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace quantum_sim::project {
    namespace {
        constexpr int projectVersion = 1;
        constexpr std::size_t maximumProjectQubits = 10U;
        constexpr std::size_t maximumInstructionCount = 100000U;
        constexpr std::size_t maximumStoredComplexValues =
                std::size_t{1} << 20U;

        void requireToken(
            std::istream &input,
            const char *expected
        ) {
            std::string token;

            if (!(input >> token) || token != expected) {
                throw std::runtime_error{
                    std::string{"Expected project token "} +
                    expected +
                    "."
                };
            }
        }

        template<typename Value>
        Value readValue(
            std::istream &input,
            const char *description
        ) {
            Value value{};

            if (!(input >> value)) {
                throw std::runtime_error{
                    std::string{"Invalid project "} +
                    description +
                    "."
                };
            }

            return value;
        }

        math::Complex readComplex(
            std::istream &input
        ) {
            const double real =
                    readValue<double>(
                        input,
                        "complex real component"
                    );

            const double imaginary =
                    readValue<double>(
                        input,
                        "complex imaginary component"
                    );

            if (
                !std::isfinite(real) ||
                !std::isfinite(imaginary)
            ) {
                throw std::runtime_error{
                    "Project contains a non-finite complex value."
                };
            }

            return math::Complex{real, imaginary};
        }

        void writeComplex(
            std::ostream &output,
            const math::Complex &value
        ) {
            output
                << value.real()
                << ' '
                << value.imaginary()
                << '\n';
        }

        void writeMatrix(
            std::ostream &output,
            const std::optional<math::ComplexMatrix> &matrix
        ) {
            if (!matrix.has_value()) {
                output << "MATRIX 0 0 0\n";
                return;
            }

            output
                << "MATRIX 1 "
                << matrix->rows()
                << ' '
                << matrix->columns()
                << '\n';

            for (std::size_t row = 0U; row < matrix->rows(); ++row) {
                for (
                    std::size_t column = 0U;
                    column < matrix->columns();
                    ++column
                ) {
                    writeComplex(
                        output,
                        matrix->at(row, column)
                    );
                }
            }
        }

        std::optional<math::ComplexMatrix> readMatrix(
            std::istream &input
        ) {
            requireToken(input, "MATRIX");

            const int present =
                    readValue<int>(input, "matrix presence flag");

            const std::size_t rows =
                    readValue<std::size_t>(input, "matrix row count");

            const std::size_t columns =
                    readValue<std::size_t>(
                        input,
                        "matrix column count"
                    );

            if (present == 0) {
                if (rows != 0U || columns != 0U) {
                    throw std::runtime_error{
                        "Absent project matrix has non-zero dimensions."
                    };
                }

                return std::nullopt;
            }

            if (
                present != 1 ||
                rows == 0U ||
                columns == 0U ||
                rows > maximumStoredComplexValues ||
                columns >
                    maximumStoredComplexValues / rows
            ) {
                throw std::runtime_error{
                    "Project matrix dimensions are invalid or too large."
                };
            }

            std::vector<math::Complex> values;
            values.reserve(rows * columns);

            for (
                std::size_t index = 0U;
                index < rows * columns;
                ++index
            ) {
                values.push_back(readComplex(input));
            }

            return math::ComplexMatrix{
                rows,
                columns,
                std::move(values)
            };
        }

        void writeVector(
            std::ostream &output,
            const std::optional<math::ComplexVector> &vector
        ) {
            if (!vector.has_value()) {
                output << "VECTOR 0 0\n";
                return;
            }

            output
                << "VECTOR 1 "
                << vector->size()
                << '\n';

            for (std::size_t index = 0U; index < vector->size(); ++index) {
                writeComplex(output, vector->at(index));
            }
        }

        std::optional<math::ComplexVector> readVector(
            std::istream &input
        ) {
            requireToken(input, "VECTOR");

            const int present =
                    readValue<int>(input, "vector presence flag");

            const std::size_t size =
                    readValue<std::size_t>(input, "vector size");

            if (present == 0) {
                if (size != 0U) {
                    throw std::runtime_error{
                        "Absent project vector has non-zero size."
                    };
                }

                return std::nullopt;
            }

            if (
                present != 1 ||
                size == 0U ||
                size > maximumStoredComplexValues
            ) {
                throw std::runtime_error{
                    "Project vector size is invalid or too large."
                };
            }

            std::vector<math::Complex> values;
            values.reserve(size);

            for (std::size_t index = 0U; index < size; ++index) {
                values.push_back(readComplex(input));
            }

            return math::ComplexVector{std::move(values)};
        }
    }

    void ProjectFile::save(
        const std::filesystem::path &path,
        const circuit::QuantumCircuit &circuit,
        const quantum::QuantumRegister &initialState
    ) {
        if (
            initialState.qubitCount() !=
            circuit.qubitCount()
        ) {
            throw std::invalid_argument{
                "Project circuit and initial register sizes differ."
            };
        }

        std::ofstream output{
            path,
            std::ios::binary |
            std::ios::trunc
        };

        if (!output) {
            throw std::runtime_error{
                "Unable to open the project file for writing."
            };
        }

        output
            << std::setprecision(
                std::numeric_limits<double>::max_digits10
            )
            << "QUBITCANVAS_PROJECT "
            << projectVersion
            << '\n'
            << "QUBITS "
            << circuit.qubitCount()
            << '\n'
            << "INITIAL "
            << initialState.stateCount()
            << '\n';

        for (
            std::size_t stateIndex = 0U;
            stateIndex < initialState.stateCount();
            ++stateIndex
        ) {
            writeComplex(
                output,
                initialState.amplitude(stateIndex)
            );
        }

        const std::vector<circuit::CircuitInstructionSnapshot> instructions =
                circuit.instructionSnapshots();

        output
            << "INSTRUCTIONS "
            << instructions.size()
            << '\n';

        for (
            const circuit::CircuitInstructionSnapshot &instruction :
                instructions
        ) {
            output
                << "BEGIN\n"
                << "KIND "
                << static_cast<int>(instruction.kind)
                << '\n'
                << "NAME "
                << std::quoted(instruction.name)
                << '\n'
                << "ANGLE "
                << (instruction.angleRadians.has_value() ? 1 : 0)
                << ' '
                << instruction.angleRadians.value_or(0.0)
                << '\n'
                << "OPERANDS "
                << instruction.operands.size();

            for (const std::size_t operand : instruction.operands) {
                output << ' ' << operand;
            }

            output << '\n';
            writeMatrix(output, instruction.matrix);
            writeVector(output, instruction.reflectionAxis);
            output << "END\n";
        }

        if (!output) {
            throw std::runtime_error{
                "Writing the project file failed."
            };
        }
    }

    ProjectDocument ProjectFile::load(
        const std::filesystem::path &path
    ) {
        std::ifstream input{
            path,
            std::ios::binary
        };

        if (!input) {
            throw std::runtime_error{
                "Unable to open the project file."
            };
        }

        requireToken(input, "QUBITCANVAS_PROJECT");

        const int version =
                readValue<int>(input, "version");

        if (version != projectVersion) {
            throw std::runtime_error{
                "Unsupported QubitCanvas project version."
            };
        }

        requireToken(input, "QUBITS");

        const std::size_t qubitCount =
                readValue<std::size_t>(
                    input,
                    "qubit count"
                );

        if (
            qubitCount == 0U ||
            qubitCount > maximumProjectQubits
        ) {
            throw std::runtime_error{
                "Project qubit count is outside the supported range."
            };
        }

        const std::size_t expectedStateCount =
                std::size_t{1} << qubitCount;

        requireToken(input, "INITIAL");

        const std::size_t stateCount =
                readValue<std::size_t>(
                    input,
                    "initial state count"
                );

        if (stateCount != expectedStateCount) {
            throw std::runtime_error{
                "Project initial state size does not match its qubit count."
            };
        }

        std::vector<math::Complex> amplitudes;
        amplitudes.reserve(stateCount);

        for (
            std::size_t stateIndex = 0U;
            stateIndex < stateCount;
            ++stateIndex
        ) {
            amplitudes.push_back(readComplex(input));
        }

        quantum::QuantumRegister initialState{
            qubitCount,
            math::ComplexVector{
                std::move(amplitudes)
            }
        };

        requireToken(input, "INSTRUCTIONS");

        const std::size_t instructionCount =
                readValue<std::size_t>(
                    input,
                    "instruction count"
                );

        if (instructionCount > maximumInstructionCount) {
            throw std::runtime_error{
                "Project instruction count is too large."
            };
        }

        circuit::QuantumCircuit circuit{qubitCount};

        for (
            std::size_t instructionIndex = 0U;
            instructionIndex < instructionCount;
            ++instructionIndex
        ) {
            requireToken(input, "BEGIN");
            requireToken(input, "KIND");

            const int kindValue =
                    readValue<int>(input, "instruction kind");

            if (
                kindValue <
                    static_cast<int>(
                        circuit::CircuitInstructionKind::SingleQubit
                    ) ||
                kindValue >
                    static_cast<int>(
                        circuit::CircuitInstructionKind::Reflection
                    )
            ) {
                throw std::runtime_error{
                    "Project instruction kind is invalid."
                };
            }

            requireToken(input, "NAME");

            std::string name;

            if (!(input >> std::quoted(name)) || name.empty()) {
                throw std::runtime_error{
                    "Project instruction name is invalid."
                };
            }

            requireToken(input, "ANGLE");

            const int hasAngle =
                    readValue<int>(input, "angle presence flag");

            const double angle =
                    readValue<double>(input, "angle");

            if (
                (hasAngle != 0 && hasAngle != 1) ||
                !std::isfinite(angle)
            ) {
                throw std::runtime_error{
                    "Project instruction angle is invalid."
                };
            }

            requireToken(input, "OPERANDS");

            const std::size_t operandCount =
                    readValue<std::size_t>(
                        input,
                        "operand count"
                    );

            if (operandCount > 3U) {
                throw std::runtime_error{
                    "Project instruction has too many operands."
                };
            }

            std::vector<std::size_t> operands;
            operands.reserve(operandCount);

            for (
                std::size_t operandIndex = 0U;
                operandIndex < operandCount;
                ++operandIndex
            ) {
                const std::size_t operand =
                        readValue<std::size_t>(
                            input,
                            "qubit operand"
                        );

                if (operand >= qubitCount) {
                    throw std::runtime_error{
                        "Project instruction operand is outside the register."
                    };
                }

                operands.push_back(operand);
            }

            std::optional<math::ComplexMatrix> matrix =
                    readMatrix(input);

            std::optional<math::ComplexVector> reflectionAxis =
                    readVector(input);

            requireToken(input, "END");

            circuit.insertInstructionSnapshot(
                circuit.instructionCount(),
                circuit::CircuitInstructionSnapshot{
                    .kind =
                        static_cast<circuit::CircuitInstructionKind>(
                            kindValue
                        ),
                    .name = std::move(name),
                    .operands = std::move(operands),
                    .angleRadians =
                        hasAngle == 1
                            ? std::optional<double>{angle}
                            : std::nullopt,
                    .matrix = std::move(matrix),
                    .reflectionAxis = std::move(reflectionAxis)
                }
            );
        }

        return ProjectDocument{
            .circuit = std::move(circuit),
            .initialState = std::move(initialState)
        };
    }
}

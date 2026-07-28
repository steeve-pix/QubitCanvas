#include "quantum_sim/project/OpenQasmFile.hpp"

#include "quantum_sim/gates/QuantumGates.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <numbers>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace quantum_sim::project {
    namespace {
        std::string trim(std::string value) {
            const auto first =
                    std::find_if_not(
                        value.begin(),
                        value.end(),
                        [](const unsigned char character) {
                            return std::isspace(character) != 0;
                        }
                    );

            const auto last =
                    std::find_if_not(
                        value.rbegin(),
                        value.rend(),
                        [](const unsigned char character) {
                            return std::isspace(character) != 0;
                        }
                    ).base();

            if (first >= last) {
                return {};
            }

            return std::string{first, last};
        }

        std::string lowercase(std::string value) {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](const unsigned char character) {
                    return static_cast<char>(
                        std::tolower(character)
                    );
                }
            );

            return value;
        }

        double parseNumber(const std::string &text) {
            std::size_t consumed{};
            const double value =
                    std::stod(text, &consumed);

            if (
                consumed != text.size() ||
                !std::isfinite(value)
            ) {
                throw std::runtime_error{
                    "Invalid OpenQASM numeric expression '" +
                    text +
                    "'."
                };
            }

            return value;
        }

        double parseAngle(std::string expression) {
            expression.erase(
                std::remove_if(
                    expression.begin(),
                    expression.end(),
                    [](const unsigned char character) {
                        return std::isspace(character) != 0;
                    }
                ),
                expression.end()
            );

            while (
                expression.size() >= 2U &&
                expression.front() == '(' &&
                expression.back() == ')'
            ) {
                expression =
                        expression.substr(
                            1U,
                            expression.size() - 2U
                        );
            }

            const std::size_t piPosition =
                    expression.find("pi");

            if (piPosition == std::string::npos) {
                return parseNumber(expression);
            }

            if (
                expression.find(
                    "pi",
                    piPosition + 2U
                ) != std::string::npos
            ) {
                throw std::runtime_error{
                    "OpenQASM angle contains pi more than once."
                };
            }

            std::string coefficientText =
                    expression.substr(0U, piPosition);

            if (
                !coefficientText.empty() &&
                coefficientText.back() == '*'
            ) {
                coefficientText.pop_back();
            }

            double coefficient{};

            if (
                coefficientText.empty() ||
                coefficientText == "+"
            ) {
                coefficient = 1.0;
            } else if (coefficientText == "-") {
                coefficient = -1.0;
            } else {
                coefficient =
                        parseNumber(coefficientText);
            }

            const std::string suffix =
                    expression.substr(piPosition + 2U);

            if (!suffix.empty()) {
                if (suffix.front() != '/') {
                    throw std::runtime_error{
                        "Unsupported OpenQASM pi expression '" +
                        expression +
                        "'."
                    };
                }

                const double divisor =
                        parseNumber(suffix.substr(1U));

                if (std::abs(divisor) <= 1e-15) {
                    throw std::runtime_error{
                        "OpenQASM angle divides by zero."
                    };
                }

                coefficient /= divisor;
            }

            return coefficient * std::numbers::pi;
        }

        std::vector<std::string> splitParameters(
            const std::string &text
        ) {
            std::vector<std::string> parameters;
            std::size_t start{};

            while (start <= text.size()) {
                const std::size_t comma =
                        text.find(',', start);

                parameters.push_back(
                    trim(
                        text.substr(
                            start,
                            comma == std::string::npos
                                ? std::string::npos
                                : comma - start
                        )
                    )
                );

                if (comma == std::string::npos) {
                    break;
                }

                start = comma + 1U;
            }

            if (
                parameters.size() == 1U &&
                parameters.front().empty()
            ) {
                parameters.clear();
            }

            return parameters;
        }

        std::vector<std::size_t> parseOperands(
            const std::string &text,
            const std::size_t qubitCount
        ) {
            std::vector<std::size_t> operands;
            std::size_t start{};

            while (start < text.size()) {
                const std::size_t comma =
                        text.find(',', start);

                const std::string operand =
                        trim(
                            text.substr(
                                start,
                                comma == std::string::npos
                                    ? std::string::npos
                                    : comma - start
                            )
                        );

                if (
                    operand.size() < 4U ||
                    operand.rfind("q[", 0U) != 0U ||
                    operand.back() != ']'
                ) {
                    throw std::runtime_error{
                        "Expected an OpenQASM operand such as q[0]."
                    };
                }

                const std::size_t qubit =
                        static_cast<std::size_t>(
                            std::stoull(
                                operand.substr(
                                    2U,
                                    operand.size() - 3U
                                )
                            )
                        );

                if (qubit >= qubitCount) {
                    throw std::runtime_error{
                        "OpenQASM operand is outside the declared register."
                    };
                }

                operands.push_back(qubit);

                if (comma == std::string::npos) {
                    break;
                }

                start = comma + 1U;
            }

            return operands;
        }

        void requireArity(
            const std::string &gate,
            const std::vector<std::size_t> &operands,
            const std::vector<std::string> &parameters,
            const std::size_t operandCount,
            const std::size_t parameterCount
        ) {
            if (
                operands.size() != operandCount ||
                parameters.size() != parameterCount
            ) {
                throw std::runtime_error{
                    "OpenQASM gate '" +
                    gate +
                    "' has the wrong operand or parameter count."
                };
            }
        }

        math::ComplexMatrix singleGate(
            const std::string &gate,
            const std::vector<std::string> &parameters
        ) {
            if (gate == "h") return gates::hadamardGate();
            if (gate == "x") return gates::xGate();
            if (gate == "y") return gates::yGate();
            if (gate == "z") return gates::zGate();
            if (gate == "s") return gates::sGate();
            if (gate == "sdg") return gates::sDaggerGate();
            if (gate == "t") return gates::tGate();
            if (gate == "tdg") return gates::tDaggerGate();
            if (gate == "sx") return gates::sxGate();
            if (gate == "sxdg") return gates::sxDaggerGate();

            const double angle =
                    parseAngle(parameters.front());

            if (gate == "p") return gates::phaseGate(angle);
            if (gate == "rx") return gates::rxGate(angle);
            if (gate == "ry") return gates::ryGate(angle);
            if (gate == "rz") return gates::rzGate(angle);

            throw std::runtime_error{
                "Unsupported OpenQASM single-qubit gate '" +
                gate +
                "'."
            };
        }

        math::ComplexMatrix twoGate(
            const std::string &gate,
            const std::vector<std::string> &parameters
        ) {
            if (gate == "cx") return gates::cxGate();
            if (gate == "cy") return gates::cyGate();
            if (gate == "cz") return gates::czGate();
            if (gate == "ch") return gates::chGate();
            if (gate == "cs") return gates::csGate();
            if (gate == "csdg") return gates::csDaggerGate();
            if (gate == "swap") return gates::swapGate();
            if (gate == "iswap") return gates::iSwapGate();
            if (gate == "dcx") return gates::dcxGate();
            if (gate == "ecr") return gates::ecrGate();
            if (gate == "sqrtswap") return gates::squareRootSwapGate();

            const double angle =
                    parseAngle(parameters.front());

            if (gate == "cp") return gates::controlledPhaseGate(angle);
            if (gate == "crx") return gates::crxGate(angle);
            if (gate == "cry") return gates::cryGate(angle);
            if (gate == "crz") return gates::crzGate(angle);
            if (gate == "rxx") return gates::rxxGate(angle);
            if (gate == "ryy") return gates::ryyGate(angle);
            if (gate == "rzz") return gates::rzzGate(angle);

            throw std::runtime_error{
                "Unsupported OpenQASM two-qubit gate '" +
                gate +
                "'."
            };
        }

        std::string displayName(const std::string &gate) {
            if (gate == "cx") return "CX";
            if (gate == "cy") return "CY";
            if (gate == "cz") return "CZ";
            if (gate == "ch") return "CH";
            if (gate == "cs") return "CS";
            if (gate == "csdg") return "CSdg";
            if (gate == "cp") return "CP";
            if (gate == "crx") return "CRx";
            if (gate == "cry") return "CRy";
            if (gate == "crz") return "CRz";
            if (gate == "rxx") return "RXX";
            if (gate == "ryy") return "RYY";
            if (gate == "rzz") return "RZZ";
            if (gate == "dcx") return "DCX";
            if (gate == "ecr") return "ECR";
            if (gate == "sdg") return "Sdg";
            if (gate == "tdg") return "Tdg";
            if (gate == "sxdg") return "SXdg";
            if (gate == "swap") return "SWAP";
            if (gate == "iswap") return "iSWAP";
            if (gate == "sqrtswap") return "sqrtSWAP";
            if (gate == "ccx") return "CCX";
            if (gate == "cswap") return "CSWAP";

            std::string name = gate;
            name.front() =
                    static_cast<char>(
                        std::toupper(
                            static_cast<unsigned char>(
                                name.front()
                            )
                        )
                    );
            return name;
        }

        void appendGate(
            circuit::QuantumCircuit &circuit,
            const std::string &statement
        ) {
            const std::size_t openParenthesis =
                    statement.find('(');

            const std::size_t closeParenthesis =
                    openParenthesis == std::string::npos
                        ? std::string::npos
                        : statement.find(')', openParenthesis + 1U);

            if (
                openParenthesis != std::string::npos &&
                closeParenthesis == std::string::npos
            ) {
                throw std::runtime_error{
                    "OpenQASM gate has an unterminated parameter list."
                };
            }

            const std::size_t operationEnd =
                    openParenthesis == std::string::npos
                        ? statement.find_first_of(" \t\r\n")
                        : openParenthesis;

            if (operationEnd == std::string::npos) {
                throw std::runtime_error{
                    "OpenQASM gate statement has no operands."
                };
            }

            const std::string gate =
                    lowercase(
                        trim(
                            statement.substr(0U, operationEnd)
                        )
                    );

            const std::vector<std::string> parameters =
                    openParenthesis == std::string::npos
                        ? std::vector<std::string>{}
                        : splitParameters(
                            statement.substr(
                                openParenthesis + 1U,
                                closeParenthesis -
                                openParenthesis - 1U
                            )
                        );

            const std::size_t operandStart =
                    openParenthesis == std::string::npos
                        ? operationEnd
                        : closeParenthesis + 1U;

            const std::vector<std::size_t> operands =
                    parseOperands(
                        statement.substr(operandStart),
                        circuit.qubitCount()
                    );

            const bool isParameterizedSingle =
                    gate == "p" ||
                    gate == "rx" ||
                    gate == "ry" ||
                    gate == "rz";

            const bool isFixedSingle =
                    gate == "h" ||
                    gate == "x" ||
                    gate == "y" ||
                    gate == "z" ||
                    gate == "s" ||
                    gate == "sdg" ||
                    gate == "t" ||
                    gate == "tdg" ||
                    gate == "sx" ||
                    gate == "sxdg";

            if (isFixedSingle || isParameterizedSingle) {
                requireArity(
                    gate,
                    operands,
                    parameters,
                    1U,
                    isParameterizedSingle ? 1U : 0U
                );

                const std::optional<double> angle =
                        isParameterizedSingle
                            ? std::optional<double>{
                                parseAngle(parameters.front())
                            }
                            : std::nullopt;

                circuit.addSingleQubitGate(
                    displayName(gate),
                    singleGate(gate, parameters),
                    operands[0U],
                    angle
                );
                return;
            }

            const bool isParameterizedTwo =
                    gate == "cp" ||
                    gate == "crx" ||
                    gate == "cry" ||
                    gate == "crz" ||
                    gate == "rxx" ||
                    gate == "ryy" ||
                    gate == "rzz";

            const bool isFixedTwo =
                    gate == "cx" ||
                    gate == "cy" ||
                    gate == "cz" ||
                    gate == "ch" ||
                    gate == "cs" ||
                    gate == "csdg" ||
                    gate == "swap" ||
                    gate == "iswap" ||
                    gate == "dcx" ||
                    gate == "ecr" ||
                    gate == "sqrtswap";

            if (isFixedTwo || isParameterizedTwo) {
                requireArity(
                    gate,
                    operands,
                    parameters,
                    2U,
                    isParameterizedTwo ? 1U : 0U
                );

                const std::optional<double> angle =
                        isParameterizedTwo
                            ? std::optional<double>{
                                parseAngle(parameters.front())
                            }
                            : std::nullopt;

                circuit.addTwoQubitGate(
                    displayName(gate),
                    twoGate(gate, parameters),
                    operands[0U],
                    operands[1U],
                    angle
                );
                return;
            }

            if (gate == "ccx" || gate == "cswap") {
                requireArity(
                    gate,
                    operands,
                    parameters,
                    3U,
                    0U
                );

                circuit.addThreeQubitGate(
                    displayName(gate),
                    gate == "ccx"
                        ? gates::ccxGate()
                        : gates::cSwapGate(),
                    operands[0U],
                    operands[1U],
                    operands[2U]
                );
                return;
            }

            throw std::runtime_error{
                "Unsupported OpenQASM gate '" +
                gate +
                "'."
            };
        }

        std::string qasmName(const std::string &name) {
            if (name == "H") return "h";
            if (name == "X") return "x";
            if (name == "Y") return "y";
            if (name == "Z") return "z";
            if (name == "S") return "s";
            if (name == "Sdg") return "sdg";
            if (name == "T") return "t";
            if (name == "Tdg") return "tdg";
            if (name == "SX") return "sx";
            if (name == "P") return "p";
            if (name == "Rx") return "rx";
            if (name == "Ry") return "ry";
            if (name == "Rz") return "rz";
            if (name == "CX") return "cx";
            if (name == "CY") return "cy";
            if (name == "CZ") return "cz";
            if (name == "CH") return "ch";
            if (name == "CP") return "cp";
            if (name == "CRx") return "crx";
            if (name == "CRy") return "cry";
            if (name == "CRz") return "crz";
            if (name == "SWAP") return "swap";
            if (name == "CCX") return "ccx";
            if (name == "CSWAP") return "cswap";

            throw std::invalid_argument{
                "Gate '" +
                name +
                "' has no faithful stdgates OpenQASM export."
            };
        }

        std::string formatAngle(const double radians) {
            const double coefficient =
                    radians / std::numbers::pi;

            for (int denominator = 1; denominator <= 64; ++denominator) {
                const double scaled =
                        coefficient *
                        static_cast<double>(denominator);

                const long long numerator =
                        std::llround(scaled);

                if (
                    std::abs(
                        scaled -
                        static_cast<double>(numerator)
                    ) <= 1e-10
                ) {
                    if (numerator == 0) {
                        return "0";
                    }

                    std::string result;

                    if (numerator == -1) {
                        result = "-";
                    } else if (numerator != 1) {
                        result =
                                std::to_string(numerator) +
                                "*";
                    }

                    result += "pi";

                    if (denominator != 1) {
                        result +=
                                "/" +
                                std::to_string(denominator);
                    }

                    return result;
                }
            }

            std::ostringstream output;
            output
                << std::setprecision(
                    std::numeric_limits<double>::max_digits10
                )
                << radians;
            return output.str();
        }
    }

    void OpenQasmFile::save(
        const std::filesystem::path &path,
        const circuit::QuantumCircuit &circuit
    ) {
        std::ofstream output{
            path,
            std::ios::trunc
        };

        if (!output) {
            throw std::runtime_error{
                "Unable to open the OpenQASM destination."
            };
        }

        output
            << "OPENQASM 3.0;\n"
            << "include \"stdgates.inc\";\n"
            << "qubit["
            << circuit.qubitCount()
            << "] q;\n\n";

        const auto instructions =
                circuit.instructionSnapshots();

        for (
            std::size_t instructionIndex = 0U;
            instructionIndex < instructions.size();
            ++instructionIndex
        ) {
            const auto &instruction =
                    instructions[instructionIndex];

            if (
                instruction.kind ==
                    circuit::CircuitInstructionKind::FullRegister ||
                instruction.kind ==
                    circuit::CircuitInstructionKind::Reflection
            ) {
                throw std::invalid_argument{
                    "Instruction " +
                    std::to_string(instructionIndex + 1U) +
                    " is register-wide and cannot be exported faithfully."
                };
            }

            std::string gate;

            try {
                gate = qasmName(instruction.name);
            } catch (const std::invalid_argument &error) {
                throw std::invalid_argument{
                    "Instruction " +
                    std::to_string(instructionIndex + 1U) +
                    ": " +
                    error.what()
                };
            }

            output << gate;

            const bool needsAngle =
                    gate == "p" ||
                    gate == "rx" ||
                    gate == "ry" ||
                    gate == "rz" ||
                    gate == "cp" ||
                    gate == "crx" ||
                    gate == "cry" ||
                    gate == "crz";

            if (needsAngle) {
                if (!instruction.angleRadians.has_value()) {
                    throw std::invalid_argument{
                        "Instruction " +
                        std::to_string(instructionIndex + 1U) +
                        " is missing its OpenQASM angle."
                    };
                }

                output
                    << '('
                    << formatAngle(
                        instruction.angleRadians.value()
                    )
                    << ')';
            }

            output << ' ';

            for (
                std::size_t operandIndex = 0U;
                operandIndex < instruction.operands.size();
                ++operandIndex
            ) {
                if (operandIndex > 0U) {
                    output << ", ";
                }

                output
                    << "q["
                    << instruction.operands[operandIndex]
                    << ']';
            }

            output << ";\n";
        }

        if (!output) {
            throw std::runtime_error{
                "Writing the OpenQASM file failed."
            };
        }
    }

    ProjectDocument OpenQasmFile::load(
        const std::filesystem::path &path
    ) {
        std::ifstream input{path};

        if (!input) {
            throw std::runtime_error{
                "Unable to open the OpenQASM file."
            };
        }

        std::ostringstream contents;
        contents << input.rdbuf();
        std::string source = contents.str();

        std::istringstream lines{source};
        std::string line;
        source.clear();

        while (std::getline(lines, line)) {
            const std::size_t comment =
                    line.find("//");

            source +=
                    line.substr(0U, comment) +
                    '\n';
        }

        std::vector<std::string> statements;
        std::size_t start{};

        while (start < source.size()) {
            const std::size_t semicolon =
                    source.find(';', start);

            if (semicolon == std::string::npos) {
                if (!trim(source.substr(start)).empty()) {
                    throw std::runtime_error{
                        "OpenQASM statement is missing a semicolon."
                    };
                }
                break;
            }

            const std::string statement =
                    trim(
                        source.substr(
                            start,
                            semicolon - start
                        )
                    );

            if (!statement.empty()) {
                statements.push_back(statement);
            }

            start = semicolon + 1U;
        }

        std::optional<std::size_t> qubitCount;
        std::vector<std::string> gateStatements;
        bool sawVersion = false;

        for (const std::string &statement : statements) {
            if (statement.rfind("OPENQASM", 0U) == 0U) {
                sawVersion =
                        statement.find("3.") !=
                        std::string::npos;
                continue;
            }

            if (statement.rfind("include", 0U) == 0U) {
                continue;
            }

            if (statement.rfind("qubit[", 0U) == 0U) {
                const std::size_t close =
                        statement.find(']');

                if (
                    close == std::string::npos ||
                    trim(statement.substr(close + 1U)) != "q"
                ) {
                    throw std::runtime_error{
                        "QubitCanvas expects one declaration such as qubit[4] q."
                    };
                }

                qubitCount =
                        static_cast<std::size_t>(
                            std::stoull(
                                statement.substr(
                                    6U,
                                    close - 6U
                                )
                            )
                        );

                if (
                    qubitCount.value() == 0U ||
                    qubitCount.value() > 10U
                ) {
                    throw std::runtime_error{
                        "OpenQASM register size is outside QubitCanvas' 1..10 range."
                    };
                }
                continue;
            }

            gateStatements.push_back(statement);
        }

        if (!sawVersion) {
            throw std::runtime_error{
                "QubitCanvas requires an OpenQASM 3.x header."
            };
        }

        if (!qubitCount.has_value()) {
            throw std::runtime_error{
                "OpenQASM file does not declare qubit[n] q."
            };
        }

        circuit::QuantumCircuit circuit{
            qubitCount.value()
        };

        for (const std::string &statement : gateStatements) {
            appendGate(circuit, statement);
        }

        return ProjectDocument{
            .circuit = std::move(circuit),
            .initialState =
                quantum::QuantumRegister::basisState(
                    qubitCount.value(),
                    0U
                )
        };
    }
}

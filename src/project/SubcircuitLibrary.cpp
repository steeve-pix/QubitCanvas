#include "quantum_sim/project/SubcircuitLibrary.hpp"

#include "quantum_sim/project/ProjectFile.hpp"
#include "quantum_sim/project/ProjectWorkspace.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string_view>
#include <system_error>

namespace quantum_sim::project {
    bool StoredSubcircuit::canInsertInto(
        const std::size_t destinationQubitCount
    ) const noexcept {
        for (const auto &instruction : instructions) {
            if (
                instruction.kind ==
                    circuit::CircuitInstructionKind::FullRegister ||
                instruction.kind ==
                    circuit::CircuitInstructionKind::Reflection
            ) {
                if (destinationQubitCount != sourceQubitCount) {
                    return false;
                }
            }

            for (const std::size_t operand : instruction.operands) {
                if (operand >= destinationQubitCount) {
                    return false;
                }
            }
        }

        return true;
    }

    SubcircuitLibrary::SubcircuitLibrary()
        : SubcircuitLibrary{
            ProjectWorkspace::defaultRoot() /
            "subcircuits"
        } {
    }

    SubcircuitLibrary::SubcircuitLibrary(
        std::filesystem::path directory
    )
        : directory_{std::move(directory)} {
    }

    void SubcircuitLibrary::save(
        const std::string &name,
        const std::size_t sourceQubitCount,
        const std::vector<circuit::CircuitInstructionSnapshot> &instructions
    ) const {
        if (name.find_first_not_of(" \t\r\n") == std::string::npos) {
            throw std::invalid_argument{
                "A reusable block needs a name."
            };
        }

        if (instructions.empty()) {
            throw std::invalid_argument{
                "A reusable block needs at least one instruction."
            };
        }

        std::filesystem::create_directories(directory_);

        circuit::QuantumCircuit blockCircuit{
            sourceQubitCount
        };

        for (const auto &instruction : instructions) {
            blockCircuit.insertInstructionSnapshot(
                blockCircuit.instructionCount(),
                instruction
            );
        }

        ProjectFile::save(
            directory_ /
                (safeFilename(name) + ".qcanvas"),
            blockCircuit,
            quantum::QuantumRegister::basisState(
                sourceQubitCount,
                0U
            )
        );
    }

    std::vector<StoredSubcircuit>
    SubcircuitLibrary::loadAll() const {
        std::vector<StoredSubcircuit> blocks;
        std::error_code error;

        if (
            !std::filesystem::is_directory(
                directory_,
                error
            )
        ) {
            return blocks;
        }

        for (
            const std::filesystem::directory_entry &entry :
            std::filesystem::directory_iterator(directory_, error)
        ) {
            if (
                error ||
                !entry.is_regular_file() ||
                entry.path().extension() != ".qcanvas"
            ) {
                continue;
            }

            try {
                ProjectDocument document =
                        ProjectFile::load(entry.path());

                blocks.push_back(
                    StoredSubcircuit{
                        entry.path().stem().string(),
                        document.circuit.qubitCount(),
                        document.circuit.instructionSnapshots()
                    }
                );
            } catch (const std::exception &) {
                // Personal libraries are best-effort: other valid blocks stay usable.
            }
        }

        std::sort(
            blocks.begin(),
            blocks.end(),
            [](const StoredSubcircuit &left, const StoredSubcircuit &right) {
                std::string leftName = left.name;
                std::string rightName = right.name;

                std::transform(
                    leftName.begin(),
                    leftName.end(),
                    leftName.begin(),
                    [](const unsigned char character) {
                        return static_cast<char>(
                            std::tolower(character)
                        );
                    }
                );

                std::transform(
                    rightName.begin(),
                    rightName.end(),
                    rightName.begin(),
                    [](const unsigned char character) {
                        return static_cast<char>(
                            std::tolower(character)
                        );
                    }
                );

                return leftName < rightName;
            }
        );

        return blocks;
    }

    std::string SubcircuitLibrary::safeFilename(
        const std::string &name
    ) {
        std::string filename = name;
        constexpr const char *invalidCharacters =
                "<>:\"/\\|?*";

        for (char &character : filename) {
            if (
                static_cast<unsigned char>(character) < 32U ||
                std::string_view{invalidCharacters}.find(character) !=
                    std::string_view::npos
            ) {
                character = '_';
            }
        }

        while (
            !filename.empty() &&
            (
                filename.back() == ' ' ||
                filename.back() == '.'
            )
        ) {
            filename.pop_back();
        }

        if (filename.empty()) {
            throw std::invalid_argument{
                "The reusable block name has no filename-safe characters."
            };
        }

        return filename;
    }
}

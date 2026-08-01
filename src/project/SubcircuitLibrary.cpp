#include "quantum_sim/project/SubcircuitLibrary.hpp"

#include "quantum_sim/project/ProjectFile.hpp"
#include "quantum_sim/project/ProjectWorkspace.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace quantum_sim::project {
    namespace {
        constexpr int metadataVersion = 1;

        [[nodiscard]] std::string lowercase(
            std::string value
        ) {
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
    }

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

    std::string SubcircuitLibrary::save(
        const std::string &name,
        const std::size_t sourceQubitCount,
        const std::vector<circuit::CircuitInstructionSnapshot> &instructions
    ) const {
        const std::string displayName = normalizeName(name);

        if (instructions.empty()) {
            throw std::invalid_argument{
                "A reusable block needs at least one instruction."
            };
        }

        std::filesystem::create_directories(directory_);

        std::string id;

        const std::vector<StoredSubcircuit> existingBlocks =
                loadAll();

        const auto existing = std::find_if(
            existingBlocks.begin(),
            existingBlocks.end(),
            [&displayName](const StoredSubcircuit &block) {
                return lowercase(block.name) ==
                       lowercase(displayName);
            }
        );

        if (existing != existingBlocks.end()) {
            id = existing->id;
        } else {
            id = createStorageId();
        }

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
            projectPath(id),
            blockCircuit,
            quantum::QuantumRegister::basisState(
                sourceQubitCount,
                0U
            )
        );

        try {
            writeMetadata(id, displayName);
        } catch (...) {
            if (existing == existingBlocks.end()) {
                std::error_code cleanupError;
                std::filesystem::remove(
                    projectPath(id),
                    cleanupError
                );
            }

            throw;
        }

        return id;
    }

    void SubcircuitLibrary::renameBlock(
        const std::string &id,
        const std::string &newName
    ) const {
        validateStorageId(id);
        const std::string displayName = normalizeName(newName);
        const std::vector<StoredSubcircuit> blocks = loadAll();

        const auto target = std::find_if(
            blocks.begin(),
            blocks.end(),
            [&id](const StoredSubcircuit &block) {
                return block.id == id;
            }
        );

        if (target == blocks.end()) {
            throw std::invalid_argument{
                "The reusable block no longer exists."
            };
        }

        const auto conflict = std::find_if(
            blocks.begin(),
            blocks.end(),
            [&id, &displayName](const StoredSubcircuit &block) {
                return block.id != id &&
                       lowercase(block.name) ==
                           lowercase(displayName);
            }
        );

        if (conflict != blocks.end()) {
            throw std::invalid_argument{
                "Another reusable block already has that name."
            };
        }

        writeMetadata(id, displayName);
    }

    void SubcircuitLibrary::erase(
        const std::string &id
    ) const {
        validateStorageId(id);
        const std::filesystem::path payload = projectPath(id);
        std::error_code error;

        if (!std::filesystem::is_regular_file(payload, error)) {
            throw std::invalid_argument{
                "The reusable block no longer exists."
            };
        }

        error.clear();

        if (!std::filesystem::remove(payload, error) || error) {
            throw std::runtime_error{
                "Unable to delete the reusable block."
            };
        }

        error.clear();
        std::filesystem::remove(metadataPath(id), error);

        if (error) {
            throw std::runtime_error{
                "The block was deleted, but its name metadata could not be removed."
            };
        }
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

                const std::string id =
                        entry.path().stem().string();

                blocks.push_back(
                    StoredSubcircuit{
                        id,
                        readDisplayName(id),
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
                return lowercase(left.name) < lowercase(right.name);
            }
        );

        return blocks;
    }

    std::string SubcircuitLibrary::normalizeName(
        const std::string &name
    ) {
        const std::size_t first =
                name.find_first_not_of(" \t\r\n");

        if (first == std::string::npos) {
            throw std::invalid_argument{
                "A reusable block needs a name."
            };
        }

        const std::size_t last =
                name.find_last_not_of(" \t\r\n");

        return name.substr(first, last - first + 1U);
    }

    std::string SubcircuitLibrary::createStorageId() const {
        std::array<std::uint32_t, 8U> seedData{};
        std::random_device entropy;

        for (std::uint32_t &seed : seedData) {
            seed = entropy();
        }

        std::seed_seq seedSequence{
            seedData.begin(),
            seedData.end()
        };

        std::mt19937_64 generator{seedSequence};

        for (std::size_t attempt = 0U; attempt < 64U; ++attempt) {
            std::ostringstream id;
            id
                << "block-"
                << std::hex
                << std::setfill('0')
                << std::setw(16)
                << generator()
                << std::setw(16)
                << generator();

            std::error_code error;

            if (
                !std::filesystem::exists(
                    projectPath(id.str()),
                    error
                ) &&
                !error &&
                !std::filesystem::exists(
                    metadataPath(id.str()),
                    error
                ) &&
                !error
            ) {
                return id.str();
            }
        }

        throw std::runtime_error{
            "Unable to allocate a reusable block storage ID."
        };
    }

    void SubcircuitLibrary::validateStorageId(
        const std::string &id
    ) {
        const std::filesystem::path candidate{id};

        if (
            id.empty() ||
            id == "." ||
            id == ".." ||
            candidate.has_root_path() ||
            candidate.has_parent_path() ||
            candidate.filename().string() != id
        ) {
            throw std::invalid_argument{
                "The reusable block storage ID is invalid."
            };
        }
    }

    std::filesystem::path SubcircuitLibrary::projectPath(
        const std::string &id
    ) const {
        validateStorageId(id);
        return directory_ / (id + ".qcanvas");
    }

    std::filesystem::path SubcircuitLibrary::metadataPath(
        const std::string &id
    ) const {
        validateStorageId(id);
        return directory_ / (id + ".qblock");
    }

    void SubcircuitLibrary::writeMetadata(
        const std::string &id,
        const std::string &name
    ) const {
        std::ofstream output{
            metadataPath(id),
            std::ios::binary | std::ios::trunc
        };

        if (!output) {
            throw std::runtime_error{
                "Unable to write reusable block metadata."
            };
        }

        output
            << "QUBITCANVAS_BLOCK "
            << metadataVersion
            << '\n'
            << "NAME "
            << std::quoted(name)
            << '\n';

        if (!output) {
            throw std::runtime_error{
                "Writing reusable block metadata failed."
            };
        }
    }

    std::string SubcircuitLibrary::readDisplayName(
        const std::string &id
    ) const {
        const std::filesystem::path path = metadataPath(id);
        std::error_code error;

        if (!std::filesystem::is_regular_file(path, error)) {
            return id;
        }

        std::ifstream input{path, std::ios::binary};
        std::string magic;
        int version = 0;
        std::string nameToken;
        std::string name;

        if (
            !(input >> magic >> version >> nameToken >> std::quoted(name)) ||
            magic != "QUBITCANVAS_BLOCK" ||
            version != metadataVersion ||
            nameToken != "NAME"
        ) {
            throw std::runtime_error{
                "Reusable block metadata is malformed."
            };
        }

        return normalizeName(name);
    }
}

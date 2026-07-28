#pragma once

#include "quantum_sim/circuit/QuantumCircuit.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace quantum_sim::project {
    /**
     * Named instruction range that can be inserted into another circuit.
     */
    struct StoredSubcircuit {
        std::string name;
        std::size_t sourceQubitCount{};
        std::vector<circuit::CircuitInstructionSnapshot> instructions;

        /**
         * Checks whether every stored operand and register-wide operation is
         * valid for a destination register.
         *
         * @param destinationQubitCount Destination circuit register size.
         */
        [[nodiscard]] bool canInsertInto(
            std::size_t destinationQubitCount
        ) const noexcept;
    };

    /**
     * Durable library of user-named circuit instruction ranges.
     *
     * Each entry is a normal `.qcanvas` document in a dedicated directory.
     * Reusing ProjectFile keeps one lossless instruction format and makes the
     * saved blocks inspectable with the same tools as complete projects.
     */
    class SubcircuitLibrary final {
    public:
        /**
         * Uses the platform-local QubitCanvas subcircuit directory.
         */
        SubcircuitLibrary();

        /**
         * Uses an explicit directory, primarily for deterministic tests.
         */
        explicit SubcircuitLibrary(std::filesystem::path directory);

        /**
         * Saves one non-empty named block, replacing a block with the same name.
         *
         * @param name Human-readable library name.
         * @param sourceQubitCount Register size where the block was authored.
         * @param instructions Lossless selected instruction snapshots.
         * @throws std::invalid_argument for an empty name or instruction list.
         */
        void save(
            const std::string &name,
            std::size_t sourceQubitCount,
            const std::vector<circuit::CircuitInstructionSnapshot> &instructions
        ) const;

        /**
         * Loads every valid stored block in case-insensitive name order.
         *
         * A malformed individual file is skipped so one damaged personal block
         * cannot prevent the application from opening.
         */
        [[nodiscard]] std::vector<StoredSubcircuit> loadAll() const;

    private:
        std::filesystem::path directory_;

        /**
         * Converts a user-facing name into a safe Windows/macOS/Linux filename.
         */
        [[nodiscard]] static std::string safeFilename(
            const std::string &name
        );
    };
}

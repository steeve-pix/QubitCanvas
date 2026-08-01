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
        /** Stable opaque key used for exact rename and delete operations. */
        std::string id;

        /** Human-readable name preserved independently of its filename. */
        std::string name;

        /** Register size on which the block was originally authored. */
        std::size_t sourceQubitCount{};

        /** Lossless circuit instructions stored in execution order. */
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
         * Saves one non-empty named block, replacing a block with the same
         * case-insensitive display name.
         *
         * @param name Human-readable library name.
         * @param sourceQubitCount Register size where the block was authored.
         * @param instructions Lossless selected instruction snapshots.
         * @return Stable storage ID for selecting the saved block after reload.
         * @throws std::invalid_argument for an empty name or instruction list.
         */
        [[nodiscard]] std::string save(
            const std::string &name,
            std::size_t sourceQubitCount,
            const std::vector<circuit::CircuitInstructionSnapshot> &instructions
        ) const;

        /**
         * Changes only a block's display name; its opaque storage ID and
         * `.qcanvas` payload remain unchanged.
         *
         * @param id Stable ID returned by save() or loadAll().
         * @param newName New non-empty display name.
         * @throws std::invalid_argument when the ID is unknown, the name is
         * empty, or another block already owns the requested name.
         */
        void renameBlock(
            const std::string &id,
            const std::string &newName
        ) const;

        /**
         * Permanently removes one exact block and its display-name metadata.
         *
         * @param id Stable ID returned by save() or loadAll().
         * @throws std::invalid_argument when the ID is malformed or unknown.
         * @throws std::runtime_error when filesystem removal fails.
         */
        void erase(const std::string &id) const;

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
         * Trims outer whitespace while preserving the user's visible name.
         */
        [[nodiscard]] static std::string normalizeName(
            const std::string &name
        );

        /**
         * Generates a collision-checked filename key with no user data in it.
         */
        [[nodiscard]] std::string createStorageId() const;

        /**
         * Rejects path separators and other values that could escape the
         * personal subcircuit directory.
         */
        static void validateStorageId(const std::string &id);

        /** Returns the normal project payload path for an opaque block ID. */
        [[nodiscard]] std::filesystem::path projectPath(
            const std::string &id
        ) const;

        /** Returns the small display-name metadata path for a block ID. */
        [[nodiscard]] std::filesystem::path metadataPath(
            const std::string &id
        ) const;

        /** Writes the versioned display-name record beside a block payload. */
        void writeMetadata(
            const std::string &id,
            const std::string &name
        ) const;

        /**
         * Reads display metadata, or returns the filename stem for a legacy
         * block created before opaque IDs were introduced.
         */
        [[nodiscard]] std::string readDisplayName(
            const std::string &id
        ) const;
    };
}

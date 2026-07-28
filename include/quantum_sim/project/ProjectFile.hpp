#pragma once

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

#include <filesystem>

namespace quantum_sim::project {
    /**
     * Complete editable document stored in a QubitCanvas project file.
     */
    struct ProjectDocument {
        circuit::QuantumCircuit circuit;
        quantum::QuantumRegister initialState;
    };

    /**
     * Reads and writes versioned, lossless `.qcanvas` project documents.
     */
    class ProjectFile final {
    public:
        /**
         * Writes a circuit and its normalized initial register.
         *
         * @param path Destination `.qcanvas` path.
         * @param circuit Circuit whose complete instruction data is stored.
         * @param initialState Register used at debugger step zero.
         * @throws std::runtime_error when the file cannot be written.
         * @throws std::invalid_argument when circuit and register sizes differ.
         */
        static void save(
            const std::filesystem::path &path,
            const circuit::QuantumCircuit &circuit,
            const quantum::QuantumRegister &initialState
        );

        /**
         * Loads and validates one project document.
         *
         * @param path Existing `.qcanvas` path.
         * @return Reconstructed circuit and initial register.
         * @throws std::runtime_error for unreadable or malformed documents.
         */
        [[nodiscard]] static ProjectDocument load(
            const std::filesystem::path &path
        );
    };
}

#pragma once

#include "quantum_sim/project/ProjectFile.hpp"

#include <filesystem>

namespace quantum_sim::project {
    /**
     * OpenQASM 3 interchange for QubitCanvas' losslessly represented gate subset.
     */
    class OpenQasmFile final {
    public:
        /**
         * Exports a standards-oriented OpenQASM 3 circuit.
         *
         * @throws std::invalid_argument when an instruction has no faithful
         *         OpenQASM spelling or lacks required parameter metadata.
         * @throws std::runtime_error when the destination cannot be written.
         */
        static void save(
            const std::filesystem::path &path,
            const circuit::QuantumCircuit &circuit
        );

        /**
         * Imports an OpenQASM 3 file into a zero-initialized register.
         *
         * The parser accepts the stdgates single-, controlled-, rotation-, swap-,
         * and three-qubit operations supported by QubitCanvas.
         *
         * @throws std::runtime_error for malformed or unsupported statements.
         */
        [[nodiscard]] static ProjectDocument load(
            const std::filesystem::path &path
        );
    };
}

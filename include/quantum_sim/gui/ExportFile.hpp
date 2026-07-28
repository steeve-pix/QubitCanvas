#pragma once

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeModel.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

#include <filesystem>

namespace quantum_sim::gui {
    /**
     * Human-readable circuit and numerical exports produced from exact models.
     */
    class ExportFile final {
    public:
        /**
         * Writes a standalone dark-theme SVG circuit diagram.
         */
        static void saveCircuitSvg(
            const std::filesystem::path &path,
            const circuit::QuantumCircuit &circuit
        );

        /**
         * Writes every complex amplitude and probability as CSV.
         */
        static void saveStateCsv(
            const std::filesystem::path &path,
            const quantum::QuantumRegister &state
        );

        /**
         * Writes every displayed density cell, including complex components.
         */
        static void saveDensityCsv(
            const std::filesystem::path &path,
            const density_volume::DensityLayer &layer
        );
    };
}

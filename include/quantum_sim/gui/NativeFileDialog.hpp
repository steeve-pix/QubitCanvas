#pragma once

#include <filesystem>
#include <optional>

namespace quantum_sim::gui {
    /**
     * Small platform adapter for choosing QubitCanvas project files.
     */
    class NativeFileDialog final {
    public:
        /**
         * Opens the platform file picker for an existing `.qcanvas` file.
         *
         * @return Selected path, or nullopt when cancelled or unsupported.
         */
        [[nodiscard]] static std::optional<std::filesystem::path>
        openProject();

        /**
         * Opens the platform file picker for a destination `.qcanvas` file.
         *
         * @return Selected path, or nullopt when cancelled or unsupported.
         */
        [[nodiscard]] static std::optional<std::filesystem::path>
        saveProject();

        /**
         * Chooses an existing OpenQASM 3 source file.
         */
        [[nodiscard]] static std::optional<std::filesystem::path>
        openQasm();

        /**
         * Chooses an OpenQASM 3 export destination.
         */
        [[nodiscard]] static std::optional<std::filesystem::path>
        saveQasm();

        /**
         * Chooses a standalone SVG circuit-diagram destination.
         */
        [[nodiscard]] static std::optional<std::filesystem::path>
        saveCircuitSvg();

        /**
         * Chooses a state-vector CSV destination.
         */
        [[nodiscard]] static std::optional<std::filesystem::path>
        saveStateCsv();

        /**
         * Chooses a density-matrix CSV destination.
         */
        [[nodiscard]] static std::optional<std::filesystem::path>
        saveDensityCsv();
    };
}

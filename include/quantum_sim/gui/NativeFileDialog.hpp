#pragma once

#include <filesystem>
#include <optional>

namespace quantum_sim::gui {
    /**
     * Small platform adapter for choosing QubitCanvas project and export files.
     *
     * Windows uses the Win32 common dialogs, macOS uses the system chooser,
     * and Linux uses zenity or kdialog when either desktop utility is present.
     */
    class NativeFileDialog final {
    public:
        /**
         * Opens the platform file picker for an existing `.qcanvas` file.
         *
         * @return Selected path, or nullopt when cancelled or unavailable.
         */
        [[nodiscard]] static std::optional<std::filesystem::path>
        openProject();

        /**
         * Opens the platform file picker for a destination `.qcanvas` file.
         *
         * @return Selected path, or nullopt when cancelled or unavailable.
         */
        [[nodiscard]] static std::optional<std::filesystem::path>
        saveProject();

        /**
         * Chooses an existing OpenQASM 3 source file.
         *
         * @return Selected path, or nullopt when cancelled or unavailable.
         */
        [[nodiscard]] static std::optional<std::filesystem::path>
        openQasm();

        /**
         * Chooses an OpenQASM 3 export destination.
         *
         * @return Selected path, or nullopt when cancelled or unavailable.
         */
        [[nodiscard]] static std::optional<std::filesystem::path>
        saveQasm();

        /**
         * Chooses a standalone SVG circuit-diagram destination.
         *
         * @return Selected path, or nullopt when cancelled or unavailable.
         */
        [[nodiscard]] static std::optional<std::filesystem::path>
        saveCircuitSvg();

        /**
         * Chooses a state-vector CSV destination.
         *
         * @return Selected path, or nullopt when cancelled or unavailable.
         */
        [[nodiscard]] static std::optional<std::filesystem::path>
        saveStateCsv();

        /**
         * Chooses a density-matrix CSV destination.
         *
         * @return Selected path, or nullopt when cancelled or unavailable.
         */
        [[nodiscard]] static std::optional<std::filesystem::path>
        saveDensityCsv();
    };
}

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
    };
}

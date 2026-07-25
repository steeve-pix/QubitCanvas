#pragma once
#include "GateLibraryStyle.hpp"

#include <cstddef>
#include <optional>
#include <string>

namespace quantum_sim::gui {
    /**
     * Gate palette entry shown as one selectable button.
     */
    struct GateDescriptor {
        const char *name;
        const char *tooltip;
    };

    /**
     * Left-side palette that lets the user choose gates for circuit placement.
     */
    class GateLibraryPanel {
    public:
        /**
         * Creates a gate library panel with a visual style.
         *
         * @param style Button size and selected-state colors.
         */
        explicit GateLibraryPanel(GateLibraryStyle style = {});

        /**
         * @return Current visual style.
         */
        [[nodiscard]] const GateLibraryStyle &style() const noexcept;

        /**
         * Replaces the visual style.
         *
         * @param style New style values.
         */
        void setStyle(GateLibraryStyle style);

        /**
         * Draws all gate categories and selection summary.
         */
        void draw();

        /**
         * @return Currently selected gate name, if a gate is waiting to be placed.
         */
        [[nodiscard]] const std::optional<std::string> &selectedGate() const noexcept;

        /**
         * @return Rotation angle captured for newly selected Rx, Ry, or Rz gates.
         */
        [[nodiscard]] double rotationAngleRadians() const noexcept;

        /**
         * Clears any selected gate.
         */
        void clearSelection() noexcept;

        /**
         * Moves the selected gate name out of the panel and clears selection.
         *
         * @return Selected gate name, or nullopt when nothing was selected.
         */
        [[nodiscard]] std::optional<std::string> consumeSelectedGate();

    private:
        GateLibraryStyle style_;
        std::optional<std::string> selectedGate_;
        float rotationAngleRadians_{1.57079632679F};

        /**
         * Draws one square gate button.
         *
         * @param label Button text and gate name.
         * @param tooltip Hover explanation.
         * @param selected Whether this gate is currently selected.
         * @return True when the button was clicked.
         */
        [[nodiscard]] bool drawGateButton(const char *label, const char *tooltip, bool selected);

        /**
         * @param gateName Gate name to test.
         * @return True when gateName matches selectedGate_.
         */
        [[nodiscard]] bool isGateSelected(const char *gateName) const noexcept;

        /**
         * @return True when another gate button fits on the current row.
         */
        [[nodiscard]] bool canPlaceNextGateButton() const;

        /**
         * Draws a titled group of gate buttons.
         *
         * @param title Category label.
         * @param gates Gate descriptors to draw.
         * @param gateCount Number of descriptors in gates.
         */
        void drawGateCategory(const char *title, const GateDescriptor *gates, std::size_t gateCount);

        /**
         * Draws the radians slider and common-angle shortcuts for rotation gates.
         */
        void drawRotationAngleControl();

        /**
         * Draws the selected-gate readout and clear button.
         */
        void drawSelectionSummary();
    };
}

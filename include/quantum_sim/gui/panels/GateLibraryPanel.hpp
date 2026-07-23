#pragma once
#include "GateLibraryStyle.hpp"
#include <optional>
#include <string>

namespace quantum_sim::gui {
    struct GateDescriptor {
        const char *name;
        const char *tooltip;
    };

    class GateLibraryPanel {
    public:
        explicit GateLibraryPanel(GateLibraryStyle style = {});

        [[nodiscard]] const GateLibraryStyle &style() const noexcept;

        void setStyle(GateLibraryStyle style);

        void draw();

        [[nodiscard]] const std::optional<std::string> &selectedGate() const noexcept;

        void clearSelection() noexcept;

        [[nodiscard]] std::optional<std::string> consumeSelectedGate();

    private:
        GateLibraryStyle style_;

        [[nodiscard]] bool drawGateButton(const char *label, const char *tooltip, bool selected);

        std::optional<std::string> selectedGate_;

        [[nodiscard]] bool isGateSelected(const char *gateName) const noexcept;

        [[nodiscard]] bool canPlaceNextGateButton() const;

        void drawGateCategory(const char *title, const GateDescriptor *gates, std::size_t gateCount);

        void drawSelectionSummary();
    };
}

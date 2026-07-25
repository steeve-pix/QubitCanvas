#pragma once

#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/gui/rendering/BlochSphereStyle.hpp"

namespace quantum_sim::gui {
    class BlochSphereRenderer {
    public:
        explicit BlochSphereRenderer(BlochSphereStyle style = {});

        void draw(const quantum::BlochVector &bloch);

        [[nodiscard]] const BlochSphereStyle &style() const noexcept;

        void setStyle(BlochSphereStyle style);

        void drawSphereGeometry(ImDrawList *drawList, const ImVec2 &center, float radius, int sphereSegments,
                                ImU32 sphereOutlineColor, const quantum::BlochVector &bloch) const;

    private:
        BlochSphereStyle style_;
        bool detailsPinned_{false};

        void drawPinnedDetails(const quantum::BlochVector &bloch) const;

        void drawHoverTooltip(const quantum::BlochVector &bloch) const;

        void drawCoordinates(const quantum::BlochVector &bloch) const;

        [[nodiscard]] bool handleCanvasInteraction(float canvasSize, const quantum::BlochVector &bloch);

        [[nodiscard]] float calculateCanvasSize() const;

        [[nodiscard]] float calculateHorizontalOffset(float availableWidth, float canvasSize) const noexcept;

        [[nodiscard]] float calculateRadius(float canvasSize) const noexcept;

        [[nodiscard]] ImU32 chooseSphereOutlineColor(bool canvasHovered) const noexcept;

        [[nodiscard]]int calculateSphereSegments() const noexcept;
    };
}

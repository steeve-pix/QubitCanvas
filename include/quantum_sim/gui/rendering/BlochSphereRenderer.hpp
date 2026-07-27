#pragma once

#include "quantum_sim/gui/rendering/BlochSphereStyle.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

namespace quantum_sim::gui {
    /**
     * Interactive ImGui renderer for a Bloch-vector projection.
     */
    class BlochSphereRenderer {
    public:
        /**
         * Creates a renderer with a visual style.
         *
         * @param style Geometry and color values.
         */
        explicit BlochSphereRenderer(BlochSphereStyle style = {});

        /**
         * Draws the sphere and handles hover/click interactions.
         *
         * @param bloch Bloch vector to render.
         */
        void draw(const quantum::BlochVector &bloch);

        /**
         * @return Current style values.
         */
        [[nodiscard]] const BlochSphereStyle &style() const noexcept;

        /**
         * Replaces style values.
         *
         * @param style New style values.
         */
        void setStyle(BlochSphereStyle style);

        /**
         * Draws the actual sphere geometry into an existing draw list.
         *
         * @param drawList ImGui draw target.
         * @param center Sphere center in screen coordinates.
         * @param radius Sphere radius.
         * @param sphereSegments Circle segment count.
         * @param sphereOutlineColor Outline color selected from interaction state.
         * @param bloch Bloch vector to project.
         */
        void drawSphereGeometry(ImDrawList *drawList, const ImVec2 &center, float radius, int sphereSegments,
                                ImU32 sphereOutlineColor, const quantum::BlochVector &bloch) const;

    private:
        BlochSphereStyle style_;
        bool detailsPinned_{false};

        /**
         * Draws persistent coordinate details below the sphere.
         *
         * @param bloch Vector being displayed.
         */
        void drawPinnedDetails(const quantum::BlochVector &bloch) const;

        /**
         * Draws hover tooltip details for the sphere.
         *
         * @param bloch Vector being displayed.
         */
        void drawHoverTooltip(const quantum::BlochVector &bloch) const;

        /**
         * Draws numeric vector coordinates.
         *
         * @param bloch Vector being displayed.
         */
        void drawCoordinates(const quantum::BlochVector &bloch) const;

        /**
         * Creates the invisible hit target and updates pinned-state interaction.
         *
         * @param canvasSize Square canvas side length.
         * @param bloch Vector used by hover details.
         * @return True when the canvas is hovered.
         */
        [[nodiscard]] bool handleCanvasInteraction(float canvasSize, const quantum::BlochVector &bloch);

        /**
         * @return Canvas side length that fits the current content region.
         */
        [[nodiscard]] float calculateCanvasSize() const;

        /**
         * Computes x offset used to center the canvas.
         *
         * @param availableWidth Current ImGui content width.
         * @param canvasSize Computed canvas size.
         * @return Horizontal offset in pixels.
         */
        [[nodiscard]] float calculateHorizontalOffset(float availableWidth, float canvasSize) const noexcept;

        /**
         * Computes the effective radius after padding is applied.
         *
         * @param canvasSize Current square canvas size.
         * @return Sphere radius in pixels.
         */
        [[nodiscard]] float calculateRadius(float canvasSize) const noexcept;

        /**
         * Chooses outline color based on hover and pinned state.
         *
         * @param canvasHovered Whether the invisible canvas is hovered.
         * @return Packed ImGui color.
         */
        [[nodiscard]] ImU32 chooseSphereOutlineColor(bool canvasHovered) const noexcept;

        /**
         * @return Circle segment count clamped to a sensible minimum.
         */
        [[nodiscard]] int calculateSphereSegments() const noexcept;
    };
}

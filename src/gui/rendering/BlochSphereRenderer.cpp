#include "quantum_sim/gui/rendering/BlochSphereRenderer.hpp"

#include "imgui.h"
#include <utility>
#include <algorithm>

namespace quantum_sim::gui {
    BlochSphereRenderer::BlochSphereRenderer(
        BlochSphereStyle style
    )
        : style_{std::move(style)} {
    }

    void BlochSphereRenderer::draw(
        const quantum::BlochVector &bloch
    ) {
        const float availableWidth =
                std::max(0.0F, ImGui::GetContentRegionAvail().x);

        // The sphere is drawn in a square canvas centered inside the inspector.
        const float canvasSize =
                calculateCanvasSize();

        if (canvasSize <= 0.0F) {
            return;
        }

        const float horizontalOffset =
                calculateHorizontalOffset(availableWidth, canvasSize);

        if (horizontalOffset > 0.0F) {
            ImGui::SetCursorPosX(
                ImGui::GetCursorPosX() + horizontalOffset
            );
        }

        const ImVec2 canvasPosition =
                ImGui::GetCursorScreenPos();

        const bool canvasHovered =
                handleCanvasInteraction(canvasSize, bloch);

        // From here on, all drawing uses absolute screen coordinates.
        ImDrawList *drawList =
                ImGui::GetWindowDrawList();

        const ImVec2 center{
            canvasPosition.x + canvasSize / 2.0F,
            canvasPosition.y + canvasSize / 2.0F
        };

        const int sphereSegments =
                calculateSphereSegments();

        const float radius =
                calculateRadius(canvasSize);

        const ImU32 sphereOutlineColor =
                chooseSphereOutlineColor(canvasHovered);

        drawSphereGeometry(drawList, center, radius, sphereSegments, sphereOutlineColor, bloch);

        if (detailsPinned_) {
            drawPinnedDetails(bloch);
        }
    }

    const BlochSphereStyle &BlochSphereRenderer::style() const noexcept {
        return style_;
    }

    void BlochSphereRenderer::setStyle(
        BlochSphereStyle style
    ) {
        style_ = std::move(style);
    }

    void BlochSphereRenderer::drawSphereGeometry(ImDrawList *drawList, const ImVec2 &center, float radius,
                                                 int sphereSegments, ImU32 sphereOutlineColor,
                                                 const quantum::BlochVector &bloch) const {
        const auto drawEllipse =
                [&](const ImVec2 &ellipseCenter, const float radiusX, const float radiusY,
                    const float rotation, const ImU32 color, const float thickness) {
            constexpr int segmentCount = 96;

            // Dear ImGui has circle helpers, but custom ellipses need a path.
            const float cosine =
                    std::cos(rotation);

            const float sine =
                    std::sin(rotation);

            drawList->PathClear();

            for (int segment = 0; segment <= segmentCount; ++segment) {
                const float angle =
                        (static_cast<float>(segment) / static_cast<float>(segmentCount)) *
                        2.0F *
                        3.1415926535F;

                const float x =
                        std::cos(angle) * radiusX;

                const float y =
                        std::sin(angle) * radiusY;

                drawList->PathLineTo(
                    ImVec2{
                        ellipseCenter.x + x * cosine - y * sine,
                        ellipseCenter.y + x * sine + y * cosine
                    }
                );
            }

            drawList->PathStroke(
                color,
                ImDrawFlags_Closed,
                thickness
            );
        };

        const auto drawFilledEllipse =
                [&](const ImVec2 &ellipseCenter, const float radiusX, const float radiusY,
                    const ImU32 color) {
            constexpr int segmentCount = 64;

            // Filled ellipse is used for the soft ground shadow.
            drawList->PathClear();

            for (int segment = 0; segment < segmentCount; ++segment) {
                const float angle =
                        (static_cast<float>(segment) / static_cast<float>(segmentCount)) *
                        2.0F *
                        3.1415926535F;

                drawList->PathLineTo(
                    ImVec2{
                        ellipseCenter.x + std::cos(angle) * radiusX,
                        ellipseCenter.y + std::sin(angle) * radiusY
                    }
                );
            }

            drawList->PathFillConvex(color);
        };

        drawFilledEllipse(
            ImVec2{
                center.x,
                center.y + radius * 0.72F
            },
            radius * 0.86F,
            radius * 0.18F,
            style_.shadowColor
        );

        // Layer filled circles to fake a softly lit sphere.
        drawList->AddCircleFilled(
            center,
            radius,
            style_.sphereFillBottomColor,
            sphereSegments
        );

        drawList->AddCircleFilled(
            ImVec2{
                center.x - radius * 0.16F,
                center.y - radius * 0.20F
            },
            radius * 0.78F,
            style_.sphereFillTopColor,
            sphereSegments
        );

        drawEllipse(
            center,
            radius,
            radius * style_.equatorSquash,
            0.0F,
            style_.meridianColor,
            1.2F
        );

        // Meridians make the 2D projection read as a 3D object.
        drawEllipse(
            center,
            radius * style_.meridianSquash,
            radius,
            0.0F,
            style_.meridianColor,
            1.1F
        );

        drawEllipse(
            center,
            radius * style_.meridianSquash,
            radius,
            0.78F,
            style_.meridianColor,
            1.0F
        );

        drawList->AddCircle(
            center,
            radius,
            sphereOutlineColor,
            sphereSegments,
            style_.sphereOutlineThickness
        );

        const ImU32 axisColor =
                style_.axisColor;

        drawList->AddLine(
            ImVec2{center.x - radius, center.y},
            ImVec2{center.x + radius, center.y},
            axisColor,
            style_.axisThickness
        );

        drawList->AddLine(
            ImVec2{center.x, center.y - radius},
            ImVec2{center.x, center.y + radius},
            axisColor,
            style_.axisThickness
        );

        drawList->AddText(
            ImVec2{
                center.x + radius + style_.positiveXLabelOffsetX,
                center.y - style_.horizontalLabelOffsetY
            }, style_.axisColor,
            style_.positiveXLabel.data(),
            style_.positiveXLabel.data() + style_.positiveXLabel.size()
        );

        drawList->AddText(
            ImVec2{
                center.x - radius - style_.negativeXLabelOffsetX,
                center.y - style_.horizontalLabelOffsetY
            },
            style_.axisColor, style_.negativeXLabel.data(),
            style_.negativeXLabel.data() + style_.negativeXLabel.size()
        );

        drawList->AddText(
            ImVec2{
                center.x - style_.verticalLabelOffsetX,
                center.y - radius - style_.positiveZLabelOffsetY
            },
            style_.axisColor, style_.positiveZLabel.data(),
            style_.positiveZLabel.data() + style_.positiveZLabel.size()
        );

        drawList->AddText(
            ImVec2{
                center.x - style_.verticalLabelOffsetX,
                center.y + radius + style_.negativeZLabelOffsetY
            },
            style_.axisColor, style_.negativeZLabel.data(),
            style_.negativeZLabel.data() + style_.negativeZLabel.size()
        );

        const float depth =
                std::clamp(
                    static_cast<float>(bloch.y),
                    -1.0F,
                    1.0F
                );

        // Y is represented as a screen-space depth offset.
        const ImVec2 vectorEnd{
            center.x +
            static_cast<float>(bloch.x) * radius +
            depth * radius * 0.24F,

            center.y -
            static_cast<float>(bloch.z) * radius +
            depth * radius * 0.10F
        };

        const ImU32 vectorColor =
                style_.vectorColor;

        drawList->AddLine(
            ImVec2{center.x + 3.0F, center.y + 3.0F},
            ImVec2{vectorEnd.x + 3.0F, vectorEnd.y + 3.0F},
            IM_COL32(0, 0, 0, 95),
            style_.vectorThickness + 2.0F
        );

        drawList->AddLine(
            center,
            vectorEnd,
            vectorColor,
            style_.vectorThickness
        );

        const float rawDepthMarkerRadius =
                style_.depthMarkerBaseRadius
                + depth
                * style_.depthMarkerScale;

        // The endpoint marker gets bigger when the vector points toward the viewer.
        const float depthMarkerRadius =
                std::clamp(
                    rawDepthMarkerRadius,
                    style_.minimumDepthMarkerRadius,
                    style_.maximumDepthMarkerRadius
                );

        drawList->AddCircleFilled(
            vectorEnd,
            depthMarkerRadius,
            vectorColor
        );
    }

    void BlochSphereRenderer::drawPinnedDetails(const quantum::BlochVector &bloch) const {
        ImGui::Spacing();
        ImGui::TextDisabled("Pinned Bloch vector");

        drawCoordinates(bloch);
    }

    void BlochSphereRenderer::drawHoverTooltip(const quantum::BlochVector &bloch) const {
        ImGui::BeginTooltip();

        ImGui::TextUnformatted(
            detailsPinned_
                ? "Click to unpin Bloch details"
                : "Click to pin Bloch details"
        );

        ImGui::Separator();

        drawCoordinates(bloch);

        ImGui::EndTooltip();
    }

    void BlochSphereRenderer::drawCoordinates(const quantum::BlochVector &bloch) const {
        ImGui::Text("X = %.4f", bloch.x);
        ImGui::Text("Y = %.4f", bloch.y);
        ImGui::Text("Z = %.4f", bloch.z);
    }

    bool BlochSphereRenderer::handleCanvasInteraction(float canvasSize, const quantum::BlochVector &bloch) {
        ImGui::PushID(this);

        // InvisibleButton reserves the canvas area and gives us hover/click state.
        ImGui::InvisibleButton(
            style_.canvasId.data(),
            ImVec2{
                canvasSize,
                canvasSize
            }
        );

        const bool canvasHovered =
                ImGui::IsItemHovered();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            detailsPinned_ = !detailsPinned_;
        }

        ImGui::PopID();

        if (canvasHovered) {
            ImGui::SetMouseCursor(
                ImGuiMouseCursor_Hand
            );

            drawHoverTooltip(bloch);
        }

        return canvasHovered;
    }

    float BlochSphereRenderer::calculateCanvasSize() const {
        const float availableWidth =
                std::max(
                    0.0F,
                    ImGui::GetContentRegionAvail().x
                );

        if (availableWidth < style_.minimumCanvasSize) {
            ImGui::TextDisabled(
                "%.*s",
                static_cast<int>(
                    style_.narrowCanvasMessage.size()
                ),
                style_.narrowCanvasMessage.data()
            );

            return 0.0F;
        }

        return std::min(
            style_.canvasSize,
            availableWidth
        );
    }

    float BlochSphereRenderer::calculateHorizontalOffset(float availableWidth, float canvasSize) const noexcept {
        return std::max(
            0.0F,
            (availableWidth - canvasSize) / 2.0F
        );
    }

    float BlochSphereRenderer::calculateRadius(float canvasSize) const noexcept {
        const float effectivePadding =
                std::min(
                    style_.canvasPadding,
                    canvasSize / 4.0F
                );

        const float maximumRadius =
                std::max(
                    0.0F,
                    canvasSize / 2.0F - effectivePadding
                );

        return std::min(
            style_.radius,
            maximumRadius
        );
    }

    ImU32 BlochSphereRenderer::chooseSphereOutlineColor(const bool canvasHovered) const noexcept {
        if (detailsPinned_) {
            return style_.pinnedSphereOutlineColor;
        }

        if (canvasHovered) {
            return style_.hoveredSphereOutlineColor;
        }

        return style_.sphereOutlineColor;
    }

    int BlochSphereRenderer::calculateSphereSegments() const noexcept {
        constexpr int minimumSphereSegments = 12;

        return std::max(
            style_.sphereSegments,
            minimumSphereSegments
        );
    }
}

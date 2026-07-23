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

        const ImVec2 vectorEnd{
            center.x + static_cast<float>(bloch.x) * radius,
            center.y - static_cast<float>(bloch.z) * radius
        };

        const ImU32 vectorColor =
                style_.vectorColor;

        drawList->AddLine(
            center,
            vectorEnd,
            vectorColor,
            style_.vectorThickness
        );

        const float rawDepthMarkerRadius =
                style_.depthMarkerBaseRadius
                + static_cast<float>(bloch.y)
                * style_.depthMarkerScale;

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

#pragma once

#include "imgui.h"
#include <string_view>

namespace quantum_sim::gui {
    /**
     * Geometry, color, and label constants for BlochSphereRenderer.
     */
    struct BlochSphereStyle {
        // Canvas and main sphere geometry.
        float canvasSize{240.0F};
        float radius{95.0F};
        float canvasPadding{16.0F};
        float equatorSquash{0.36F};
        float meridianSquash{0.56F};
        float minimumVisibleRadius{12.0F};
        float minimumCanvasSize{80.0F};
        int sphereSegments{64};

        // Stroke and marker sizes.
        float sphereOutlineThickness{2.0F};
        float axisThickness{1.5F};
        float vectorThickness{4.0F};
        float depthMarkerBaseRadius{6.0F};
        float depthMarkerScale{3.0F};
        float minimumDepthMarkerRadius{2.0F};
        float maximumDepthMarkerRadius{12.0F};

        // Sphere, vector, and interaction colors.
        ImU32 sphereOutlineColor{IM_COL32(210, 210, 220, 255)};
        ImU32 sphereFillTopColor{IM_COL32(35, 78, 118, 120)};
        ImU32 sphereFillBottomColor{IM_COL32(17, 24, 43, 180)};
        ImU32 meridianColor{IM_COL32(90, 150, 190, 105)};
        ImU32 shadowColor{IM_COL32(2, 5, 10, 135)};
        ImU32 axisColor{IM_COL32(125, 135, 155, 255)};
        ImU32 vectorColor{IM_COL32(90, 180, 255, 255)};
        ImU32 hoveredSphereOutlineColor{IM_COL32(235, 235, 245, 255)};
        ImU32 pinnedSphereOutlineColor{IM_COL32(255, 200, 90, 255)};

        // Axis label layout.
        float positiveXLabelOffsetX{6.0F};
        float horizontalLabelOffsetY{8.0F};
        float negativeXLabelOffsetX{24.0F};
        float verticalLabelOffsetX{8.0F};
        float positiveZLabelOffsetY{20.0F};
        float negativeZLabelOffsetY{5.0F};
        std::string_view positiveXLabel{"+X"};
        std::string_view negativeXLabel{"-X"};
        std::string_view positiveZLabel{"+Z"};
        std::string_view negativeZLabel{"-Z"};

        std::string_view narrowCanvasMessage{
            "Inspector is too narrow to display the Bloch sphere."
        };

        std::string_view canvasId{
            "BlochSphereCanvas"
        };
    };
}

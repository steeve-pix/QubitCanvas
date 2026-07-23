#pragma once

#include "imgui.h"
#include <string_view>

namespace quantum_sim::gui {
    struct BlochSphereStyle {
        float canvasSize{240.0F};
        float radius{95.0F};

        float sphereOutlineThickness{2.0F};
        float axisThickness{1.5F};
        float vectorThickness{4.0F};

        float depthMarkerBaseRadius{6.0F};
        float depthMarkerScale{3.0F};

        ImU32 sphereOutlineColor{
            IM_COL32(210, 210, 220, 255)
        };

        ImU32 axisColor{
            IM_COL32(125, 135, 155, 255)
        };

        ImU32 vectorColor{
            IM_COL32(90, 180, 255, 255)
        };

        float positiveXLabelOffsetX{6.0F};
        float horizontalLabelOffsetY{8.0F};

        float negativeXLabelOffsetX{24.0F};

        float verticalLabelOffsetX{8.0F};
        float positiveZLabelOffsetY{20.0F};
        float negativeZLabelOffsetY{5.0F};

        float canvasPadding{16.0F};

        float minimumDepthMarkerRadius{2.0F};
        float maximumDepthMarkerRadius{12.0F};

        int sphereSegments{64};

        std::string_view positiveXLabel{"+X"};
        std::string_view negativeXLabel{"-X"};
        std::string_view positiveZLabel{"+Z"};
        std::string_view negativeZLabel{"-Z"};

        float minimumVisibleRadius{12.0F};
        float minimumCanvasSize{80.0F};

        std::string_view narrowCanvasMessage{
            "Inspector is too narrow to display the Bloch sphere."
        };

        std::string_view canvasId{
            "BlochSphereCanvas"
        };

        ImU32 hoveredSphereOutlineColor{
            IM_COL32(235, 235, 245, 255)
        };

        ImU32 pinnedSphereOutlineColor{
            IM_COL32(255, 200, 90, 255)
        };
    };
}

#pragma once
#include "imgui.h"

namespace quantum_sim::gui {
    struct CircuitStyle {
        float topMargin{64.0F};
        float wireSpacing{70.0F};
        float gateSpacing{90.0F};
        float minimumGateSpacing{50.0F};

        float wireStartOffset{40.0F};
        float firstGateOffset{55.0F};
        float rightPadding{65.0F};

        float wireThickness{1.7F};

        float gateHalfWidth{18.0F};
        float gateHalfHeight{18.0F};
        float gateCornerRadius{5.0F};
        float gateOutlineThickness{1.8F};

        float controlRadius{6.0F};
        float targetRadius{10.0F};
        float controlledConnectionThickness{2.5F};

        float wireGapHalfWidth{19.0F};
        float wireGapHalfHeight{3.0F};

        float symbolGapHalfWidth{12.0F};
        float symbolGapHalfHeight{3.0F};

        float canvasWidth{0.0F};
        float canvasHeight{0.0F};

        float canvasPaddingX{16.0F};
        float canvasPaddingY{16.0F};

        float minimumCanvasHeight{220.0F};

        float timelineLabelOffsetY{2.0F};
        float executionStemStartOffsetY{19.0F};
        float executionStemEndGap{23.0F};
        float executionStemThickness{1.5F};

        float columnGuideVerticalPadding{18.0F};
        float columnGuideThickness{1.0F};
        float activeColumnGuideThickness{1.2F};

        float qubitLabelOffsetY{8.0F};

        float outerGateGlowPadding{7.0F};
        float innerGateGlowPadding{3.0F};

        float outerGateGlowCornerRadius{9.0F};
        float innerGateGlowCornerRadius{7.0F};

        float controlledGlowLineThickness{8.0F};
        float controlGlowRadius{11.0F};
        float targetGlowPadding{6.0F};

        float targetCrossInset{3.0F};

        float animationSpeed{4.0F};

        float gateGlowBaseAlpha{25.0F};
        float gateGlowPulseAlpha{25.0F};
        float innerGateGlowAlphaBoost{20.0F};

        float controlledGlowBaseAlpha{20.0F};
        float controlledGlowPulseAlpha{25.0F};

        int activeRed{255};

        float activeGreenBase{170.0F};
        float activeGreenPulse{60.0F};

        float activeBlueBase{40.0F};
        float activeBluePulse{60.0F};

        int executionStemAlpha{175};

        ImU32 wireColor{IM_COL32(140, 150, 165, 255)};
        ImU32 qubitLabelColor{IM_COL32(220, 220, 230, 255)};
        ImU32 inactiveTimelineColor{IM_COL32(130, 145, 165, 255)};
        ImU32 inactiveGateColor{IM_COL32(45, 190, 245, 255)};

        ImU32 activeColumnGuideColor{IM_COL32(80, 115, 145, 28)};
        ImU32 inactiveColumnGuideColor{IM_COL32(70, 105, 135, 20)};

        ImU32 gateFillColor{IM_COL32(28, 48, 68, 255)};
        ImU32 activeGateOutlineColor{IM_COL32(255, 225, 150, 255)};
        ImU32 gateTextColor{IM_COL32(235, 245, 255, 255)};

        ImU32 outerGateGlowColor{IM_COL32(255, 175, 45, 255)};
        ImU32 innerGateGlowColor{IM_COL32(255, 190, 60, 255)};
        ImU32 controlledGlowColor{IM_COL32(255, 180, 45, 255)};

        ImU32 canvasBackgroundColor{IM_COL32(15, 18, 24, 255)};

        ImU32 hoveredGateOutlineColor{IM_COL32(120, 220, 255, 255)};

        ImU32 hoveredGateFillColor{IM_COL32(34, 64, 88, 255)};

        ImU32 selectedGateOutlineColor{IM_COL32(185, 235, 255, 255)};

        float selectedGateOutlinePadding{4.0F};
        float selectedGateOutlineThickness{2.0F};
        float selectedGateOutlineCornerRadius{8.0F};

        ImU32 placementGuideColor{IM_COL32(70, 170, 220, 90)};

        float placementGuideThickness{1.5F};

        ImU32 placementPreviewFillColor{IM_COL32(24, 70, 92, 220)};

        ImU32 placementPreviewOutlineColor{IM_COL32(70, 190, 235, 255)};

        float placementGuideDashLength{6.0F};
        float placementGuideGapLength{5.0F};

        ImU32 controlledGateColor{
            IM_COL32(175, 100, 255, 255)
        };

        ImU32 hoveredControlledGateColor{
            IM_COL32(210, 150, 255, 255)
        };

        ImU32 stepBadgeFillColor{
            IM_COL32(28, 38, 52, 230)
        };

        ImU32 activeStepBadgeFillColor{
            IM_COL32(90, 55, 20, 240)
        };

        float stepBadgePaddingX{8.0F};
        float stepBadgePaddingY{3.0F};
        float stepBadgeCornerRadius{5.0F};
        float executionOrderLabelOffsetY{-18.0F};
    };
}

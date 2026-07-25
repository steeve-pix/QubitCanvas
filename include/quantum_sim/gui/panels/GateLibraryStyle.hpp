#pragma once
#include "imgui.h"

namespace quantum_sim::gui {
    struct GateLibraryStyle {
        float gateButtonSize{48.0F};

        ImU32 selectedButtonColor{
            IM_COL32(45, 110, 145, 255)
        };

        ImU32 selectedButtonHoveredColor{
            IM_COL32(55, 135, 175, 255)
        };

        ImU32 selectedButtonActiveColor{
            IM_COL32(65, 150, 190, 255)
        };

    };
}

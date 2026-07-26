#pragma once

#include "imgui.h"
#include "quantum_sim/debug/DebuggerSnapshot.hpp"

namespace quantum_sim::gui {
    /**
     * Draws debugger-step context over the OpenGL density-volume texture.
     *
     * The inspector submits only Dear ImGui overlay primitives. It does not
     * participate in voxel rendering or create an input item, so the viewport
     * retains its complete camera and picking surface.
     */
    class DensityStepInspector final {
    public:
        /**
         * Draws the current instruction and a compact affected-qubit diagram.
         *
         * @param snapshot Current debugger state and instruction metadata.
         * @param imageOrigin Screen-space top-left corner of the rendered texture.
         * @param imageSize Screen-space size of the rendered texture.
         */
        static void draw(
            const debug::DebuggerSnapshot &snapshot,
            const ImVec2 &imageOrigin,
            const ImVec2 &imageSize
        );
    };
}

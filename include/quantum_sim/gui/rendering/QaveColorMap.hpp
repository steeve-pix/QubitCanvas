#pragma once

namespace quantum_sim::gui::qave {
    /**
     * Linear RGB color used by QAVE mesh vertices.
     */
    struct Color {
        float red{};
        float green{};
        float blue{};
    };

    /**
     * Maps density phase to the QubitCanvas cyan/violet/amber/magenta wheel.
     *
     * Magnitude controls brightness without changing the phase hue.
     *
     * @param normalizedMagnitude Density magnitude normalized to [0, 1].
     * @param phaseRadians Density phase in radians.
     * @return Linear RGB color suitable for the OpenGL shader.
     */
    [[nodiscard]] Color phaseColor(
        double normalizedMagnitude,
        double phaseRadians
    ) noexcept;
}

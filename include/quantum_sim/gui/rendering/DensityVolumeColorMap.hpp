#pragma once

namespace quantum_sim::gui::density_volume {
    /**
     * Linear RGB color used by Density Volume mesh vertices.
     */
    struct Color {
        float red{};
        float green{};
        float blue{};
    };

    /**
     * Converts an absolute density magnitude into the shared display range.
     *
     * Every view divides by the same reference magnitude, normally the
     * strongest cell in the complete density history. This keeps the 2D
     * inspector, Layer Stack, and Floor Field synchronized while retaining
     * meaningful color differences between circuit steps. Numerical density
     * values are never modified.
     *
     * @param magnitude Absolute magnitude of one density-matrix cell.
     * @param referenceMaximum Shared strongest absolute display magnitude.
     * @return Relative display magnitude clamped to [0, 1].
     */
    [[nodiscard]] double normalizeMagnitude(
        double magnitude,
        double referenceMaximum
    ) noexcept;

    /**
     * Maps density magnitude through the Stella-inspired Inferno tone curve.
     *
     * The response preserves Inferno's violet and crimson bands for weak and
     * medium cells while reserving gold and ivory for probability peaks. It
     * changes only the display color, never the underlying density value.
     * Returned components are sRGB values in [0, 1].
     *
     * @param normalizedMagnitude Density magnitude normalized to [0, 1].
     * @return Tone-shaped Inferno color in sRGB space.
     */
    [[nodiscard]] Color magnitudeColor(
        double normalizedMagnitude
    ) noexcept;

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

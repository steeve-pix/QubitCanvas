#pragma once

#include "quantum_sim/math/Complex.hpp"

#include <string>

namespace quantum_sim::gui::notation {
    /**
     * Formats a real quantum value using familiar exact notation when possible.
     *
     * Recognized forms include reduced fractions, square-root ratios, and
     * common half-angle radicals. Values without a clean symbolic form use a
     * trimmed fixed-point representation.
     *
     * @param value Numerical value to display.
     * @param decimalPlaces Maximum fallback decimal precision.
     * @return Human-readable exact or decimal representation.
     */
    [[nodiscard]] std::string formatReal(
        double value,
        int decimalPlaces = 8
    );

    /**
     * Formats a complex value in rectangular form.
     *
     * @param value Complex value to display.
     * @param displayMultiplier Optional multiplier applied before formatting.
     * @param decimalPlaces Maximum fallback decimal precision.
     * @return Representation such as "√2/2-√2/2i".
     */
    [[nodiscard]] std::string formatComplex(
        const math::Complex &value,
        double displayMultiplier = 1.0,
        int decimalPlaces = 8
    );

    /**
     * Formats a radian angle as a multiple of pi.
     *
     * Common rational multiples retain exact notation. Other finite angles use
     * a trimmed decimal coefficient, so an angle never falls back to a raw
     * radian value.
     *
     * @param radians Angle in radians.
     * @param decimalPlaces Maximum precision for a decimal pi coefficient.
     * @param includeUnit Whether to append the optional " rad" unit.
     * @return Representation such as "-3π/4" or "0.74π".
     */
    [[nodiscard]] std::string formatRadians(
        double radians,
        int decimalPlaces = 3,
        bool includeUnit = false
    );

    /**
     * Formats one amplitude in compact polar/exponential form.
     *
     * @param magnitude Non-negative amplitude magnitude.
     * @param phaseRadians Amplitude phase in radians.
     * @param decimalPlaces Maximum fallback decimal precision.
     * @return Representation such as "√2/2 e^(iπ/4)".
     */
    [[nodiscard]] std::string formatPolarAmplitude(
        double magnitude,
        double phaseRadians,
        int decimalPlaces = 8
    );
}

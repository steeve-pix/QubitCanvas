#pragma once

#include <string_view>

namespace quantum_sim::gui::gate_notation {
    /**
     * Returns the typographic gate name used by buttons, circuit boxes, and
     * placement previews.
     *
     * Dagger and square-root operations use their mathematical symbols.
     * Unknown names are returned unchanged so custom circuit operations remain
     * visible.
     *
     * @param gateName Internal gate identifier.
     * @return Human-readable gate label.
     */
    [[nodiscard]] std::string_view displayName(
        std::string_view gateName
    ) noexcept;

    /**
     * Returns the compact identity displayed at an exchange-path crossing.
     *
     * @param gateName SWAP-family internal gate identifier.
     * @return Unique compact exchange label, or displayName() for other gates.
     */
    [[nodiscard]] std::string_view exchangeBadge(
        std::string_view gateName
    ) noexcept;

    /**
     * Returns the unique compact identity used on the circuit.
     *
     * Exchange operations use their crossing badge while all other operations
     * use displayName().
     *
     * @param gateName Internal gate identifier.
     * @return Circuit label that does not alias another built-in gate.
     */
    [[nodiscard]] std::string_view circuitLabel(
        std::string_view gateName
    ) noexcept;
}

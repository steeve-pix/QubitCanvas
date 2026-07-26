#include "quantum_sim/gui/rendering/QaveColorMap.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace quantum_sim::gui::qave {
    namespace {
        [[nodiscard]] Color mix(
            const Color &left,
            const Color &right,
            const float amount
        ) noexcept {
            return Color{
                left.red + (right.red - left.red) * amount,
                left.green + (right.green - left.green) * amount,
                left.blue + (right.blue - left.blue) * amount
            };
        }
    }

    Color phaseColor(
        const double normalizedMagnitude,
        const double phaseRadians
    ) noexcept {
        constexpr std::array<Color, 5> phaseStops{
            Color{0.10F, 0.72F, 0.95F},
            Color{0.35F, 0.10F, 0.78F},
            Color{1.00F, 0.78F, 0.16F},
            Color{0.94F, 0.17F, 0.52F},
            Color{0.10F, 0.72F, 0.95F}
        };

        const double wrappedPhase =
                std::remainder(
                    phaseRadians,
                    2.0 * std::numbers::pi
                );

        const float phasePosition =
                static_cast<float>(
                    (wrappedPhase + std::numbers::pi) /
                    (2.0 * std::numbers::pi)
                ) * 4.0F;

        const int firstStop =
                std::clamp(
                    static_cast<int>(std::floor(phasePosition)),
                    0,
                    3
                );

        const float localAmount =
                std::clamp(
                    phasePosition - static_cast<float>(firstStop),
                    0.0F,
                    1.0F
                );

        const Color hue =
                mix(
                    phaseStops[static_cast<std::size_t>(firstStop)],
                    phaseStops[static_cast<std::size_t>(firstStop + 1)],
                    localAmount
                );

        const float brightness =
                0.20F +
                0.80F * std::sqrt(
                    std::clamp(
                        static_cast<float>(normalizedMagnitude),
                        0.0F,
                        1.0F
                    )
                );

        return Color{
            hue.red * brightness,
            hue.green * brightness,
            hue.blue * brightness
        };
    }
}

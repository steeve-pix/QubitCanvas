#include "quantum_sim/gui/rendering/DensityVolumeColorMap.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace quantum_sim::gui::density_volume {
    namespace {
        constexpr float infernoToneExponent =
                0.60F;

        [[nodiscard]] float clamp01(const float value) noexcept {
            return std::clamp(value, 0.0F, 1.0F);
        }

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

    Color magnitudeColor(
        const double normalizedMagnitude
    ) noexcept {
        constexpr float channelScale =
                1.0F / 255.0F;

        constexpr std::array<Color, 9U> infernoStops{
            Color{26.0F * channelScale, 12.0F * channelScale, 54.0F * channelScale},
            Color{58.0F * channelScale, 18.0F * channelScale, 99.0F * channelScale},
            Color{101.0F * channelScale, 26.0F * channelScale, 123.0F * channelScale},
            Color{151.0F * channelScale, 41.0F * channelScale, 107.0F * channelScale},
            Color{201.0F * channelScale, 62.0F * channelScale, 74.0F * channelScale},
            Color{233.0F * channelScale, 109.0F * channelScale, 38.0F * channelScale},
            Color{248.0F * channelScale, 168.0F * channelScale, 40.0F * channelScale},
            Color{251.0F * channelScale, 221.0F * channelScale, 96.0F * channelScale},
            Color{255.0F * channelScale, 250.0F * channelScale, 214.0F * channelScale}
        };

        const float magnitude =
                clamp01(
                    static_cast<float>(normalizedMagnitude)
                );

        // Keep weak and medium density values in Inferno's violet and crimson
        // bands. A smaller exponent promotes nearly every non-zero value into
        // orange, flattening the visual hierarchy between background cells
        // and genuinely bright probability peaks.
        const float toneWeight =
                std::pow(
                    magnitude,
                    infernoToneExponent
                );

        const float rampPosition =
                (0.01F + 0.99F * toneWeight) *
                static_cast<float>(infernoStops.size() - 1U);

        const std::size_t firstStop =
                std::min(
                    static_cast<std::size_t>(
                        std::floor(rampPosition)
                    ),
                    infernoStops.size() - 2U
                );

        const float localAmount =
                clamp01(
                    rampPosition -
                    static_cast<float>(firstStop)
                );

        return mix(
            infernoStops[firstStop],
            infernoStops[firstStop + 1U],
            localAmount
        );
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

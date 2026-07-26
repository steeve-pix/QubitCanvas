#include "quantum_sim/gui/rendering/QaveLayerStackLayout.hpp"

#include <algorithm>
#include <cmath>

namespace quantum_sim::gui::qave {
    SceneLayout LayerStackLayout::build(
        const DensityStack &stack,
        const std::size_t visibleThroughLayer
    ) {
        constexpr float matrixSide = 10.5F;
        constexpr float baseThickness = 0.018F;
        constexpr float maximumVoxelHeight = 0.24F;
        constexpr float minimumVisibleHeight = 0.045F;
        constexpr float layerSpacing = 0.32F;
        constexpr float layerDepthOffset = 0.045F;
        constexpr double visibleMagnitudeThreshold = 0.006;

        SceneLayout layout;

        if (stack.layers.empty()) {
            return layout;
        }

        const std::size_t lastLayer =
                std::min(
                    visibleThroughLayer,
                    stack.layers.size() - 1U
                );

        std::size_t voxelCapacity{};

        for (std::size_t index = 0; index <= lastLayer; ++index) {
            voxelCapacity +=
                    stack.layers[index].cells.size() * 2U;
        }

        layout.voxels.reserve(voxelCapacity);

        for (std::size_t index = 0; index <= lastLayer; ++index) {
            const DensityLayer &layer =
                    stack.layers[index];

            double layerMaximumMagnitude{};

            for (const DensityCell &cell : layer.cells) {
                layerMaximumMagnitude =
                        std::max(
                            layerMaximumMagnitude,
                            cell.magnitude
                        );
            }

            layerMaximumMagnitude =
                    std::max(layerMaximumMagnitude, 1e-12);

            const float cellPitch =
                    matrixSide / static_cast<float>(layer.dimension);

            const float tileSide =
                    cellPitch * 0.88F;

            const float voxelSide =
                    cellPitch * 0.76F;

            const float layerBaseY =
                    static_cast<float>(layer.index) * layerSpacing;

            const float layerCenterZ =
                    -static_cast<float>(layer.index) * layerDepthOffset;

            for (const DensityCell &cell : layer.cells) {
                const float x =
                        (
                            static_cast<float>(cell.column) + 0.5F
                        ) * cellPitch - matrixSide * 0.5F;

                const float z =
                        (
                            static_cast<float>(cell.row) + 0.5F
                        ) * cellPitch - matrixSide * 0.5F + layerCenterZ;

                layout.voxels.push_back(
                    PlacedVoxel{
                        .center = Vector3{
                            x,
                            layerBaseY + baseThickness * 0.5F,
                            z
                        },
                        .size = Vector3{
                            tileSide,
                            baseThickness,
                            tileSide
                        },
                        .color = Color{0.055F, 0.025F, 0.135F},
                        .layer = layer.index,
                        .row = cell.row,
                        .column = cell.column,
                        .magnitudeVoxel = false
                    }
                );

                const double normalizedMagnitude =
                        std::clamp(
                            cell.magnitude / layerMaximumMagnitude,
                            0.0,
                            1.0
                        );

                if (normalizedMagnitude < visibleMagnitudeThreshold) {
                    continue;
                }

                const float height =
                        std::max(
                            minimumVisibleHeight,
                            static_cast<float>(normalizedMagnitude) *
                            maximumVoxelHeight
                        );

                layout.voxels.push_back(
                    PlacedVoxel{
                        .center = Vector3{
                            x,
                            layerBaseY + baseThickness + height * 0.5F,
                            z
                        },
                        .size = Vector3{
                            voxelSide,
                            height,
                            voxelSide
                        },
                        .color = phaseColor(
                            normalizedMagnitude,
                            cell.phaseRadians
                        ),
                        .layer = layer.index,
                        .row = cell.row,
                        .column = cell.column,
                        .magnitudeVoxel = true
                    }
                );
            }
        }

        const float historyHeight =
                static_cast<float>(lastLayer) *
                layerSpacing +
                baseThickness +
                maximumVoxelHeight;

        const float historyDepth =
                matrixSide +
                static_cast<float>(lastLayer) *
                layerDepthOffset;

        layout.center = Vector3{
            0.0F,
            historyHeight * 0.5F,
            -static_cast<float>(lastLayer) *
            layerDepthOffset * 0.5F
        };

        layout.radius =
                std::sqrt(
                    std::pow(matrixSide * 0.5F, 2.0F) +
                    std::pow(historyHeight * 0.5F, 2.0F) +
                    std::pow(historyDepth * 0.5F, 2.0F)
                );

        return layout;
    }
}

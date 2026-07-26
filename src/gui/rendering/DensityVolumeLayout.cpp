#include "quantum_sim/gui/rendering/DensityVolumeLayout.hpp"

#include <algorithm>
#include <cmath>

namespace quantum_sim::gui::density_volume {
    namespace {
        constexpr float matrixSide = 10.5F;
        constexpr float stackBaseThickness = 0.020F;
        constexpr float stackMaximumVoxelHeight = 0.24F;
        constexpr float stackMinimumVisibleHeight = 0.030F;
        constexpr float stackLayerSpacing = 0.285F;
        constexpr float floorBaseThickness = 0.055F;
        constexpr float floorMaximumVoxelHeight = 2.80F;
        constexpr float floorMinimumVisibleHeight = 0.050F;
        constexpr double visibleMagnitudeThreshold = 0.006;
        constexpr Color baseTileColor{0.110F, 0.018F, 0.240F};

        void appendDensityLayer(
            SceneLayout &layout,
            const DensityLayer &layer,
            const float baseY,
            const float baseThickness,
            const float maximumVoxelHeight,
            const float minimumVisibleHeight
        ) {
            if (layer.dimension == 0U) {
                return;
            }

            const float cellPitch =
                    matrixSide / static_cast<float>(layer.dimension);

            const float tileSide =
                    cellPitch * 0.90F;

            const float voxelSide =
                    cellPitch * 0.78F;

            for (const DensityCell &cell : layer.cells) {
                const float x =
                        (
                            static_cast<float>(cell.column) + 0.5F
                        ) * cellPitch - matrixSide * 0.5F;

                const float z =
                        (
                            static_cast<float>(cell.row) + 0.5F
                        ) * cellPitch - matrixSide * 0.5F;

                layout.voxels.push_back(
                    PlacedVoxel{
                        .center = Vector3{
                            x,
                            baseY + baseThickness * 0.5F,
                            z
                        },
                        .size = Vector3{
                            tileSide,
                            baseThickness,
                            tileSide
                        },
                        .color = baseTileColor,
                        .layer = layer.index,
                        .row = cell.row,
                        .column = cell.column,
                        .magnitudeVoxel = false
                    }
                );

                const double normalizedMagnitude =
                        std::clamp(
                            cell.magnitude,
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
                            baseY + baseThickness + height * 0.5F,
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

        [[nodiscard]] SceneLayout buildFloorField(
            const DensityLayer &layer
        ) {
            SceneLayout layout;
            layout.voxels.reserve(layer.cells.size() * 2U);

            appendDensityLayer(
                layout,
                layer,
                0.0F,
                floorBaseThickness,
                floorMaximumVoxelHeight,
                floorMinimumVisibleHeight
            );

            const float totalHeight =
                    floorBaseThickness + floorMaximumVoxelHeight;

            layout.center = Vector3{
                0.0F,
                totalHeight * 0.5F,
                0.0F
            };

            layout.radius =
                    std::sqrt(
                        std::pow(matrixSide * 0.5F, 2.0F) * 2.0F +
                        std::pow(totalHeight * 0.5F, 2.0F)
                    );

            return layout;
        }

        [[nodiscard]] SceneLayout buildLayerStack(
            const DensityStack &stack,
            const std::size_t lastLayer
        ) {
            SceneLayout layout;
            std::size_t voxelCapacity{};

            for (std::size_t index = 0; index <= lastLayer; ++index) {
                voxelCapacity +=
                        stack.layers[index].cells.size() * 2U;
            }

            layout.voxels.reserve(voxelCapacity);

            for (std::size_t index = 0; index <= lastLayer; ++index) {
                appendDensityLayer(
                    layout,
                    stack.layers[index],
                    static_cast<float>(index) * stackLayerSpacing,
                    stackBaseThickness,
                    stackMaximumVoxelHeight,
                    stackMinimumVisibleHeight
                );
            }

            const float historyHeight =
                    static_cast<float>(lastLayer) *
                    stackLayerSpacing +
                    stackBaseThickness +
                    stackMaximumVoxelHeight;

            layout.center = Vector3{
                0.0F,
                historyHeight * 0.5F,
                0.0F
            };

            layout.radius =
                    std::sqrt(
                        std::pow(matrixSide * 0.5F, 2.0F) * 2.0F +
                        std::pow(historyHeight * 0.5F, 2.0F)
                    );

            return layout;
        }
    }

    SceneLayout LayerStackLayout::build(
        const DensityStack &stack,
        const std::size_t selectedLayer,
        const VisualizationMode mode
    ) {
        if (stack.layers.empty()) {
            return SceneLayout{};
        }

        const std::size_t lastLayer =
                std::min(
                    selectedLayer,
                    stack.layers.size() - 1U
                );

        return mode == VisualizationMode::FloorField
                   ? buildFloorField(stack.layers[lastLayer])
                   : buildLayerStack(stack, lastLayer);
    }
}

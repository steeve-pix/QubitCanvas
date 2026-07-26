#include "quantum_sim/gui/rendering/DensityVolumeLayout.hpp"

#include <algorithm>
#include <cmath>

namespace quantum_sim::gui::density_volume {
    namespace {
        constexpr float cellPitch = 0.70F;
        constexpr float stackBaseThickness = 0.025F;
        constexpr float stackMaximumVoxelHeight = 0.58F;
        constexpr float stackMinimumVisibleHeight = 0.42F;
        constexpr float stackLayerSpacing = 0.70F;
        constexpr float floorBaseThickness = 0.035F;
        constexpr float floorMaximumVoxelHeight = 0.64F;
        constexpr float floorMinimumVisibleHeight = 0.42F;
        constexpr double visibleMagnitudeThreshold = 0.006;
        constexpr Color baseTileColor{0.018F, 0.026F, 0.055F};

        [[nodiscard]] float matrixSide(
            const DensityLayer &layer
        ) noexcept {
            return cellPitch *
                   static_cast<float>(layer.dimension);
        }

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

            const float layerSide =
                    matrixSide(layer);

            const float tileSide =
                    cellPitch * 0.90F;

            const float voxelSide =
                    cellPitch * 0.74F;

            for (const DensityCell &cell : layer.cells) {
                const float x =
                        (
                            static_cast<float>(cell.column) + 0.5F
                        ) * cellPitch - layerSide * 0.5F;

                const float z =
                        (
                            static_cast<float>(cell.row) + 0.5F
                        ) * cellPitch - layerSide * 0.5F;

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

        void setFloorFieldBounds(
            SceneLayout &layout,
            const DensityLayer &layer
        ) noexcept {
            const float totalHeight =
                    floorBaseThickness + floorMaximumVoxelHeight;

            const float layerSide =
                    matrixSide(layer);

            layout.center = Vector3{
                0.0F,
                totalHeight * 0.5F,
                0.0F
            };

            layout.radius =
                    std::sqrt(
                        std::pow(layerSide * 0.5F, 2.0F) * 2.0F +
                        std::pow(totalHeight * 0.5F, 2.0F)
                    );
        }

        void setLayerStackBounds(
            SceneLayout &layout,
            const DensityLayer &lastLayer
        ) noexcept {
            const float historyHeight =
                    static_cast<float>(lastLayer.index) *
                    stackLayerSpacing +
                    stackBaseThickness +
                    stackMaximumVoxelHeight;

            const float layerSide =
                    matrixSide(lastLayer);

            layout.center = Vector3{
                0.0F,
                historyHeight * 0.5F,
                0.0F
            };

            layout.radius =
                    std::sqrt(
                        std::pow(layerSide * 0.5F, 2.0F) * 2.0F +
                        std::pow(historyHeight * 0.5F, 2.0F)
                    );
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

            setFloorFieldBounds(layout, layer);

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

            setLayerStackBounds(layout, stack.layers[lastLayer]);

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

    SceneLayout LayerStackLayout::buildLayer(
        const DensityLayer &layer,
        const VisualizationMode mode
    ) {
        if (mode == VisualizationMode::FloorField) {
            return buildFloorField(layer);
        }

        SceneLayout layout;
        layout.voxels.reserve(layer.cells.size() * 2U);

        appendDensityLayer(
            layout,
            layer,
            static_cast<float>(layer.index) * stackLayerSpacing,
            stackBaseThickness,
            stackMaximumVoxelHeight,
            stackMinimumVisibleHeight
        );

        setLayerStackBounds(layout, layer);
        return layout;
    }
}

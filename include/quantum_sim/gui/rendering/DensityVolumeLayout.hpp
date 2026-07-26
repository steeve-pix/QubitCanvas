#pragma once

#include "quantum_sim/gui/rendering/DensityVolumeColorMap.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeModel.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeMath.hpp"

#include <cstddef>
#include <vector>

namespace quantum_sim::gui::density_volume {
    /**
     * Spatial presentation used for the shared density-matrix data.
     */
    enum class VisualizationMode {
        LayerStack,
        FloorField
    };

    /**
     * One cuboid placed in the 3D density-history scene.
     */
    struct PlacedVoxel {
        Vector3 center;
        Vector3 size;
        Color color;
        std::size_t layer{};
        std::size_t row{};
        std::size_t column{};
        bool magnitudeVoxel{};
    };

    /**
     * Fully positioned layer stack ready for mesh generation.
     */
    struct SceneLayout {
        std::vector<PlacedVoxel> voxels;
        Vector3 center;
        float radius{1.0F};
    };

    /**
     * Places density matrices into the history-stack or selected-floor view.
     */
    class LayerStackLayout {
    public:
        /**
         * Converts numerical cells into base tiles and magnitude-height voxels.
         *
         * Layer-stack mode includes every matrix through the selected layer.
         * Floor-field mode includes only the selected matrix at ground level.
         * Both modes preserve the absolute [0, 1] density magnitude for height.
         *
         * @param stack Shared density history.
         * @param selectedLayer Last visible history layer or selected floor.
         * @param mode Spatial presentation to build.
         * @return Positioned scene with stable bounds for camera framing.
         */
        [[nodiscard]] static SceneLayout build(
            const DensityStack &stack,
            std::size_t selectedLayer,
            VisualizationMode mode
        );
    };
}

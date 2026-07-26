#pragma once

#include "quantum_sim/gui/rendering/QaveColorMap.hpp"
#include "quantum_sim/gui/rendering/QaveDensityModel.hpp"
#include "quantum_sim/gui/rendering/QaveMath.hpp"

#include <cstddef>
#include <vector>

namespace quantum_sim::gui::qave {
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
     * Places density matrices into a separated vertical history stack.
     */
    class LayerStackLayout {
    public:
        /**
         * Converts numerical cells into base tiles and magnitude-height voxels.
         *
         * Every density layer remains a complete square X-Z grid. Near-zero
         * magnitudes keep only their thin base tile, avoiding noisy tiny boxes.
         *
         * @param stack Shared density history.
         * @param visibleThroughLayer Last layer included in the built history.
         * @return Positioned scene with stable bounds for camera framing.
         */
        [[nodiscard]] static SceneLayout build(
            const DensityStack &stack,
            std::size_t visibleThroughLayer
        );
    };
}

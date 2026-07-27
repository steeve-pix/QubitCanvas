#pragma once

#include "quantum_sim/gui/rendering/DensityVolumeColorMap.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeMath.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeModel.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace quantum_sim::gui::density_volume {
    /**
     * Spatial presentation used by the density visualization.
     */
    enum class VisualizationMode {
        LayerStack,
        FloorField
    };

    /**
     * Cell identity encoded in the OpenGL integer picking attachment.
     */
    struct Selection {
        std::size_t layer{};
        std::size_t row{};
        std::size_t column{};
        bool magnitudeVoxel{true};
    };

    /**
     * One compact GPU instance for the shared rounded-cube geometry.
     */
    struct VoxelInstance {
        Vector3 center;
        Vector3 size;
        Color color;
        float emissive{};
        float magnitude{};
        float layer{};
        float pickId{};
    };

    /**
     * One vertex in the shared unit rounded-cube mesh.
     */
    struct VoxelVertex {
        float position[3]{};
        float normal[3]{};
    };

    /**
     * Indexed geometry uploaded once and reused by every voxel instance.
     */
    struct VoxelGeometry {
        std::vector<VoxelVertex> vertices;
        std::vector<std::uint32_t> indices;
    };

    /**
     * Complete instanced scene plus stable picking and camera metadata.
     */
    struct InstanceScene {
        std::vector<VoxelInstance> voxels;
        std::vector<VoxelInstance> ghostVoxels;
        std::vector<Selection> pickRecords;
        std::vector<std::size_t> layerEndInstanceCounts;
        std::vector<std::size_t> layerEndGhostCounts;
        std::vector<Vector3> layerCenters;
        Vector3 center;
        float radius{1.0F};
        Vector3 framingMinimum{-1.0F, -1.0F, -1.0F};
        Vector3 framingMaximum{1.0F, 1.0F, 1.0F};
        float matrixSpan{1.0F};
        float layerSpacing{1.0F};
        float voxelSide{1.0F};
        Vector3 groundCenter;
        float groundHalfExtentX{4.0F};
        float groundHalfExtentZ{4.0F};
    };

    /**
     * Builds one reusable softly rounded unit cube.
     */
    class VoxelGeometryBuilder {
    public:
        /**
         * Generates a unit cube with broad planar faces and rounded transitions.
         *
         * Face vertices are intentionally duplicated so each face retains a
         * stable tangent orientation. A densely sampled, deep bevel blends
         * normals around the silhouette without replacing the real mesh with
         * a screen-facing sprite.
         *
         * @return Counter-clockwise indexed geometry centered on the origin.
         */
        [[nodiscard]] static VoxelGeometry buildRoundedCube();
    };

    /**
     * Converts numerical density layers into compact rendering instances.
     */
    class SceneBuilder {
    public:
        /**
         * Builds either the complete horizontal history or one floor matrix.
         *
         * Layer-stack mode creates fixed rounded cubes only for numerically
         * visible cells. Exact small matrices retain faint edge-only ghosts for
         * near-zero cells; bucketed large matrices omit those ghosts to avoid
         * visual and GPU noise. Layers advance along X while every density
         * matrix remains a complete vertical Y-Z slice. Floor-field mode keeps
         * the selected matrix on the X-Z plane and maps magnitude to cube
         * height.
         *
         * @param stack Shared numerical density history.
         * @param selectedLayer Selected debugger layer.
         * @param mode Requested spatial presentation.
         * @return Upload-ready instance scene.
         */
        [[nodiscard]] static InstanceScene build(
            const DensityStack &stack,
            std::size_t selectedLayer,
            VisualizationMode mode
        );
    };
}

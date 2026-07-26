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
        std::vector<Selection> pickRecords;
        std::vector<std::size_t> layerEndInstanceCounts;
        Vector3 center;
        float radius{1.0F};
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
         * Face vertices are intentionally duplicated so each face can retain a
         * stable tangent orientation while bevel normals blend around edges.
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
         * Layer-stack mode creates one opaque cube per matrix cell per layer.
         * Layers advance along X while each matrix remains a square Y-Z slab,
         * producing a coherent volume instead of diagonal overlapping floors.
         * Floor-field mode keeps the selected matrix on the X-Z plane and maps
         * magnitude to cube height.
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

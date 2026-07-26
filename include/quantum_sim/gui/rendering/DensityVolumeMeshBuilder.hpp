#pragma once

#include "quantum_sim/gui/rendering/DensityVolumeLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace quantum_sim::gui::density_volume {
    /**
     * Interleaved vertex uploaded to the Density Volume OpenGL VBO.
     */
    struct MeshVertex {
        float position[3]{};
        float normal[3]{};
        float color[3]{};
        float layer{};
        float pickId{};
        float magnitudeVoxel{};
    };

    /**
     * Cell identity returned by the integer picking attachment.
     */
    struct Selection {
        std::size_t layer{};
        std::size_t row{};
        std::size_t column{};
        bool magnitudeVoxel{};
    };

    /**
     * Indexed cuboid mesh and matching pick-record table.
     */
    struct Mesh {
        std::vector<MeshVertex> vertices;
        std::vector<std::uint32_t> indices;
        std::vector<Selection> pickRecords;
    };

    /**
     * Expands placed voxels into indexed, softly beveled cuboid geometry.
     */
    class MeshBuilder {
    public:
        /**
         * Creates 24 shared face vertices plus indexed face, edge, and corner
         * triangles for each beveled cuboid. Face-aligned normals blend across
         * the bevel geometry to produce soft highlights without fake sprites.
         *
         * @param layout Positioned density voxels.
         * @return Upload-ready mesh and one-based GPU pick mapping.
         */
        [[nodiscard]] static Mesh build(const SceneLayout &layout);
    };
}

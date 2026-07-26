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
        static constexpr std::size_t verticesPerVoxel = 24U;
        static constexpr std::size_t indicesPerBeveledVoxel = 132U;

        /**
         * Creates 24 shared face vertices plus indexed face, edge, and corner
         * triangles for each beveled cuboid. Face-aligned normals blend across
         * the bevel geometry to produce soft highlights without fake sprites.
         *
         * @param layout Positioned density voxels.
         * @return Upload-ready mesh and one-based GPU pick mapping.
         */
        [[nodiscard]] static Mesh build(const SceneLayout &layout);

        /**
         * Appends positioned voxels to an existing indexed mesh.
         *
         * Existing vertex offsets and one-based picking identifiers are
         * preserved so playback can add one history layer without rebuilding
         * geometry for earlier layers.
         *
         * @param mesh Existing mesh receiving the new geometry.
         * @param layout Positioned voxels to append.
         */
        static void append(Mesh &mesh, const SceneLayout &layout);
    };
}

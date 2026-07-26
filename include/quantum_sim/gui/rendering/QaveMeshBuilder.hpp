#pragma once

#include "quantum_sim/gui/rendering/QaveLayerStackLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace quantum_sim::gui::qave {
    /**
     * Interleaved vertex uploaded to the QAVE OpenGL VBO.
     */
    struct MeshVertex {
        float position[3]{};
        float normal[3]{};
        float color[3]{};
        float layer{};
        float pickId{};
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
     * Expands placed voxels into indexed, flat-shaded cuboid geometry.
     */
    class MeshBuilder {
    public:
        /**
         * Creates 24 face vertices and 36 triangle indices per placed cuboid.
         *
         * @param layout Positioned density voxels.
         * @return Upload-ready mesh and one-based GPU pick mapping.
         */
        [[nodiscard]] static Mesh build(const SceneLayout &layout);
    };
}

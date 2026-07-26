#include "quantum_sim/gui/rendering/DensityVolumeMeshBuilder.hpp"

#include <array>

namespace quantum_sim::gui::density_volume {
    namespace {
        struct UnitVertex {
            float x;
            float y;
            float z;
            float normalX;
            float normalY;
            float normalZ;
        };

        constexpr std::array<UnitVertex, 24> cuboidVertices{
            UnitVertex{-1.0F, -1.0F,  1.0F,  0.0F,  0.0F,  1.0F},
            UnitVertex{ 1.0F, -1.0F,  1.0F,  0.0F,  0.0F,  1.0F},
            UnitVertex{ 1.0F,  1.0F,  1.0F,  0.0F,  0.0F,  1.0F},
            UnitVertex{-1.0F,  1.0F,  1.0F,  0.0F,  0.0F,  1.0F},

            UnitVertex{ 1.0F, -1.0F, -1.0F,  0.0F,  0.0F, -1.0F},
            UnitVertex{-1.0F, -1.0F, -1.0F,  0.0F,  0.0F, -1.0F},
            UnitVertex{-1.0F,  1.0F, -1.0F,  0.0F,  0.0F, -1.0F},
            UnitVertex{ 1.0F,  1.0F, -1.0F,  0.0F,  0.0F, -1.0F},

            UnitVertex{-1.0F, -1.0F, -1.0F, -1.0F,  0.0F,  0.0F},
            UnitVertex{-1.0F, -1.0F,  1.0F, -1.0F,  0.0F,  0.0F},
            UnitVertex{-1.0F,  1.0F,  1.0F, -1.0F,  0.0F,  0.0F},
            UnitVertex{-1.0F,  1.0F, -1.0F, -1.0F,  0.0F,  0.0F},

            UnitVertex{ 1.0F, -1.0F,  1.0F,  1.0F,  0.0F,  0.0F},
            UnitVertex{ 1.0F, -1.0F, -1.0F,  1.0F,  0.0F,  0.0F},
            UnitVertex{ 1.0F,  1.0F, -1.0F,  1.0F,  0.0F,  0.0F},
            UnitVertex{ 1.0F,  1.0F,  1.0F,  1.0F,  0.0F,  0.0F},

            UnitVertex{-1.0F,  1.0F,  1.0F,  0.0F,  1.0F,  0.0F},
            UnitVertex{ 1.0F,  1.0F,  1.0F,  0.0F,  1.0F,  0.0F},
            UnitVertex{ 1.0F,  1.0F, -1.0F,  0.0F,  1.0F,  0.0F},
            UnitVertex{-1.0F,  1.0F, -1.0F,  0.0F,  1.0F,  0.0F},

            UnitVertex{-1.0F, -1.0F, -1.0F,  0.0F, -1.0F,  0.0F},
            UnitVertex{ 1.0F, -1.0F, -1.0F,  0.0F, -1.0F,  0.0F},
            UnitVertex{ 1.0F, -1.0F,  1.0F,  0.0F, -1.0F,  0.0F},
            UnitVertex{-1.0F, -1.0F,  1.0F,  0.0F, -1.0F,  0.0F}
        };

        constexpr std::array<std::uint32_t, 36> cuboidIndices{
             0U,  1U,  2U,  2U,  3U,  0U,
             4U,  5U,  6U,  6U,  7U,  4U,
             8U,  9U, 10U, 10U, 11U,  8U,
            12U, 13U, 14U, 14U, 15U, 12U,
            16U, 17U, 18U, 18U, 19U, 16U,
            20U, 21U, 22U, 22U, 23U, 20U
        };

        void appendCuboid(
            Mesh &mesh,
            const PlacedVoxel &voxel,
            const std::uint32_t pickId
        ) {
            const std::uint32_t baseVertex =
                    static_cast<std::uint32_t>(mesh.vertices.size());

            const Vector3 halfSize =
                    voxel.size * 0.5F;

            for (const UnitVertex &unit : cuboidVertices) {
                mesh.vertices.push_back(
                    MeshVertex{
                        .position = {
                            voxel.center.x + unit.x * halfSize.x,
                            voxel.center.y + unit.y * halfSize.y,
                            voxel.center.z + unit.z * halfSize.z
                        },
                        .normal = {
                            unit.normalX,
                            unit.normalY,
                            unit.normalZ
                        },
                        .color = {
                            voxel.color.red,
                            voxel.color.green,
                            voxel.color.blue
                        },
                        .layer = static_cast<float>(voxel.layer),
                        .pickId = static_cast<float>(pickId),
                        .magnitudeVoxel =
                            voxel.magnitudeVoxel ? 1.0F : 0.0F
                    }
                );
            }

            for (const std::uint32_t index : cuboidIndices) {
                mesh.indices.push_back(baseVertex + index);
            }
        }
    }

    Mesh MeshBuilder::build(const SceneLayout &layout) {
        Mesh mesh;
        mesh.vertices.reserve(layout.voxels.size() * cuboidVertices.size());
        mesh.indices.reserve(layout.voxels.size() * cuboidIndices.size());
        mesh.pickRecords.reserve(layout.voxels.size());

        for (const PlacedVoxel &voxel : layout.voxels) {
            mesh.pickRecords.push_back(
                Selection{
                    .layer = voxel.layer,
                    .row = voxel.row,
                    .column = voxel.column,
                    .magnitudeVoxel = voxel.magnitudeVoxel
                }
            );

            appendCuboid(
                mesh,
                voxel,
                static_cast<std::uint32_t>(mesh.pickRecords.size())
            );
        }

        return mesh;
    }
}

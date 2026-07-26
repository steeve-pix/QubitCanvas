#include "quantum_sim/gui/rendering/DensityVolumeMeshBuilder.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace quantum_sim::gui::density_volume {
    namespace {
        constexpr std::size_t verticesPerVoxel = 24U;
        constexpr std::size_t indicesPerBeveledVoxel = 132U;

        [[nodiscard]] float component(
            const Vector3 &value,
            const std::size_t axis
        ) noexcept {
            if (axis == 0U) {
                return value.x;
            }

            if (axis == 1U) {
                return value.y;
            }

            return value.z;
        }

        void setComponent(
            Vector3 &value,
            const std::size_t axis,
            const float componentValue
        ) noexcept {
            if (axis == 0U) {
                value.x = componentValue;
            } else if (axis == 1U) {
                value.y = componentValue;
            } else {
                value.z = componentValue;
            }
        }

        [[nodiscard]] std::size_t cornerKey(
            const std::array<std::size_t, 3> &signs
        ) noexcept {
            return signs[0] * 4U +
                   signs[1] * 2U +
                   signs[2];
        }

        [[nodiscard]] Vector3 positionAt(
            const Mesh &mesh,
            const std::uint32_t index
        ) noexcept {
            const MeshVertex &vertex =
                    mesh.vertices[index];

            return Vector3{
                vertex.position[0],
                vertex.position[1],
                vertex.position[2]
            };
        }

        void appendOrientedTriangle(
            Mesh &mesh,
            const std::uint32_t first,
            std::uint32_t second,
            std::uint32_t third,
            const Vector3 &outward
        ) {
            const Vector3 firstPosition =
                    positionAt(mesh, first);

            const Vector3 windingNormal =
                    cross(
                        positionAt(mesh, second) - firstPosition,
                        positionAt(mesh, third) - firstPosition
                    );

            if (dot(windingNormal, outward) < 0.0F) {
                std::swap(second, third);
            }

            mesh.indices.push_back(first);
            mesh.indices.push_back(second);
            mesh.indices.push_back(third);
        }

        void appendOrientedQuad(
            Mesh &mesh,
            const std::array<std::uint32_t, 4> &vertices,
            const Vector3 &outward
        ) {
            appendOrientedTriangle(
                mesh,
                vertices[0],
                vertices[1],
                vertices[2],
                outward
            );

            appendOrientedTriangle(
                mesh,
                vertices[0],
                vertices[2],
                vertices[3],
                outward
            );
        }

        void appendBeveledCuboid(
            Mesh &mesh,
            const PlacedVoxel &voxel,
            const std::uint32_t pickId
        ) {
            const Vector3 halfSize =
                    voxel.size * 0.5F;

            const float smallestDimension =
                    std::min({
                        voxel.size.x,
                        voxel.size.y,
                        voxel.size.z
                    });

            const float bevelFraction =
                    voxel.magnitudeVoxel
                        ? 0.18F
                        : 0.12F;

            const float bevel =
                    std::min(
                        std::max(
                            smallestDimension * bevelFraction,
                            0.001F
                        ),
                        smallestDimension * 0.24F
                    );

            const Vector3 inner{
                halfSize.x - bevel,
                halfSize.y - bevel,
                halfSize.z - bevel
            };

            std::array<std::uint32_t, verticesPerVoxel> faceVertices{};

            for (std::size_t axis = 0U; axis < 3U; ++axis) {
                for (std::size_t xSign = 0U; xSign < 2U; ++xSign) {
                    for (std::size_t ySign = 0U; ySign < 2U; ++ySign) {
                        for (std::size_t zSign = 0U; zSign < 2U; ++zSign) {
                            const std::array<std::size_t, 3> signs{
                                xSign,
                                ySign,
                                zSign
                            };

                            Vector3 position{
                                (xSign == 0U ? -1.0F : 1.0F) * inner.x,
                                (ySign == 0U ? -1.0F : 1.0F) * inner.y,
                                (zSign == 0U ? -1.0F : 1.0F) * inner.z
                            };

                            const float faceSign =
                                    signs[axis] == 0U
                                        ? -1.0F
                                        : 1.0F;

                            setComponent(
                                position,
                                axis,
                                faceSign * component(halfSize, axis)
                            );

                            Vector3 normal{};
                            setComponent(normal, axis, faceSign);

                            const std::uint32_t vertexIndex =
                                    static_cast<std::uint32_t>(
                                        mesh.vertices.size()
                                    );

                            faceVertices[
                                axis * 8U + cornerKey(signs)
                            ] = vertexIndex;

                            mesh.vertices.push_back(
                                MeshVertex{
                                    .position = {
                                        voxel.center.x + position.x,
                                        voxel.center.y + position.y,
                                        voxel.center.z + position.z
                                    },
                                    .normal = {
                                        normal.x,
                                        normal.y,
                                        normal.z
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
                    }
                }
            }

            const auto vertexIndex =
                    [&faceVertices](
                        const std::size_t axis,
                        const std::array<std::size_t, 3> &signs
                    ) {
                return faceVertices[
                    axis * 8U + cornerKey(signs)
                ];
            };

            constexpr std::array<std::array<std::size_t, 2>, 4> quadSigns{
                std::array<std::size_t, 2>{0U, 0U},
                std::array<std::size_t, 2>{1U, 0U},
                std::array<std::size_t, 2>{1U, 1U},
                std::array<std::size_t, 2>{0U, 1U}
            };

            // Six inset face quads retain broad readable top and side surfaces.
            for (std::size_t axis = 0U; axis < 3U; ++axis) {
                const std::size_t firstOther =
                        (axis + 1U) % 3U;

                const std::size_t secondOther =
                        (axis + 2U) % 3U;

                for (std::size_t faceSign = 0U; faceSign < 2U; ++faceSign) {
                    std::array<std::uint32_t, 4> face{};

                    for (std::size_t corner = 0U; corner < 4U; ++corner) {
                        std::array<std::size_t, 3> signs{};
                        signs[axis] = faceSign;
                        signs[firstOther] = quadSigns[corner][0];
                        signs[secondOther] = quadSigns[corner][1];
                        face[corner] = vertexIndex(axis, signs);
                    }

                    Vector3 outward{};
                    setComponent(
                        outward,
                        axis,
                        faceSign == 0U ? -1.0F : 1.0F
                    );

                    appendOrientedQuad(mesh, face, outward);
                }
            }

            constexpr std::array<std::array<std::size_t, 3>, 3> edgeAxes{
                std::array<std::size_t, 3>{0U, 1U, 2U},
                std::array<std::size_t, 3>{0U, 2U, 1U},
                std::array<std::size_t, 3>{1U, 2U, 0U}
            };

            // Twelve chamfers interpolate between face normals for soft highlights.
            for (const auto &axes : edgeAxes) {
                const std::size_t firstAxis = axes[0];
                const std::size_t secondAxis = axes[1];
                const std::size_t edgeAxis = axes[2];

                for (std::size_t firstSign = 0U; firstSign < 2U; ++firstSign) {
                    for (std::size_t secondSign = 0U; secondSign < 2U; ++secondSign) {
                        std::array<std::size_t, 3> negativeSigns{};
                        negativeSigns[firstAxis] = firstSign;
                        negativeSigns[secondAxis] = secondSign;
                        negativeSigns[edgeAxis] = 0U;

                        std::array<std::size_t, 3> positiveSigns =
                                negativeSigns;

                        positiveSigns[edgeAxis] = 1U;

                        const std::array<std::uint32_t, 4> edge{
                            vertexIndex(firstAxis, negativeSigns),
                            vertexIndex(firstAxis, positiveSigns),
                            vertexIndex(secondAxis, positiveSigns),
                            vertexIndex(secondAxis, negativeSigns)
                        };

                        Vector3 outward{};
                        setComponent(
                            outward,
                            firstAxis,
                            firstSign == 0U ? -1.0F : 1.0F
                        );
                        setComponent(
                            outward,
                            secondAxis,
                            secondSign == 0U ? -1.0F : 1.0F
                        );

                        appendOrientedQuad(mesh, edge, outward);
                    }
                }
            }

            // Eight corner triangles complete the bevel without duplicate vertices.
            for (std::size_t xSign = 0U; xSign < 2U; ++xSign) {
                for (std::size_t ySign = 0U; ySign < 2U; ++ySign) {
                    for (std::size_t zSign = 0U; zSign < 2U; ++zSign) {
                        const std::array<std::size_t, 3> signs{
                            xSign,
                            ySign,
                            zSign
                        };

                        const std::array<std::uint32_t, 3> corner{
                            vertexIndex(0U, signs),
                            vertexIndex(1U, signs),
                            vertexIndex(2U, signs)
                        };

                        const Vector3 outward{
                            xSign == 0U ? -1.0F : 1.0F,
                            ySign == 0U ? -1.0F : 1.0F,
                            zSign == 0U ? -1.0F : 1.0F
                        };

                        appendOrientedTriangle(
                            mesh,
                            corner[0],
                            corner[1],
                            corner[2],
                            outward
                        );
                    }
                }
            }
        }
    }

    Mesh MeshBuilder::build(const SceneLayout &layout) {
        Mesh mesh;
        mesh.vertices.reserve(layout.voxels.size() * verticesPerVoxel);
        mesh.indices.reserve(
            layout.voxels.size() * indicesPerBeveledVoxel
        );
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

            appendBeveledCuboid(
                mesh,
                voxel,
                static_cast<std::uint32_t>(mesh.pickRecords.size())
            );
        }

        return mesh;
    }
}

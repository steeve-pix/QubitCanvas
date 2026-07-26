#include "quantum_sim/gui/rendering/DensityVolumeScene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace quantum_sim::gui::density_volume {
    namespace {
        constexpr float cellPitch = 0.72F;
        constexpr float cubeSide = cellPitch * 0.79F;
        constexpr float roundedRadius = 0.14F;
        constexpr std::size_t roundedSegments = 2U;

        [[nodiscard]] float clamp01(const float value) noexcept {
            return std::clamp(value, 0.0F, 1.0F);
        }

        [[nodiscard]] float smoothStep(
            const float low,
            const float high,
            const float value
        ) noexcept {
            const float normalized =
                    clamp01((value - low) / (high - low));

            return normalized * normalized * (3.0F - 2.0F * normalized);
        }

        [[nodiscard]] Color mixColor(
            const Color &left,
            const Color &right,
            const float amount
        ) noexcept {
            const float safeAmount =
                    clamp01(amount);

            return Color{
                left.red + (right.red - left.red) * safeAmount,
                left.green + (right.green - left.green) * safeAmount,
                left.blue + (right.blue - left.blue) * safeAmount
            };
        }

        [[nodiscard]] Color voxelColor(
            const DensityCell &cell,
            const std::size_t layer
        ) noexcept {
            constexpr Color dormant{0.028F, 0.020F, 0.078F};
            constexpr Color ember{0.66F, 0.060F, 0.105F};
            constexpr Color orange{1.00F, 0.285F, 0.055F};
            constexpr Color gold{1.00F, 0.760F, 0.235F};
            constexpr Color whiteHot{1.00F, 0.965F, 0.720F};

            const float magnitude =
                    clamp01(static_cast<float>(cell.magnitude));

            const float response =
                    std::pow(magnitude, 0.38F);

            Color color{};

            if (response < 0.34F) {
                color = mixColor(dormant, ember, response / 0.34F);
            } else if (response < 0.68F) {
                color = mixColor(
                    ember,
                    orange,
                    (response - 0.34F) / 0.34F
                );
            } else if (response < 0.92F) {
                color = mixColor(
                    orange,
                    gold,
                    (response - 0.68F) / 0.24F
                );
            } else {
                color = mixColor(
                    gold,
                    whiteHot,
                    (response - 0.92F) / 0.08F
                );
            }

            // Phase remains visible as a restrained warm/cool variation while
            // magnitude owns the dominant inferno-style brightness ramp.
            const float phaseSin =
                    static_cast<float>(std::sin(cell.phaseRadians));

            const float phaseCos =
                    static_cast<float>(std::cos(cell.phaseRadians));

            color.red += phaseCos * 0.030F * response;
            color.green += phaseSin * 0.018F * response;
            color.blue += -phaseCos * 0.024F * response;

            const float layerVariation =
                    0.965F +
                    0.035F *
                    std::sin(static_cast<float>(layer) * 0.71F);

            return Color{
                clamp01(color.red * layerVariation),
                clamp01(color.green * layerVariation),
                clamp01(color.blue * layerVariation)
            };
        }

        [[nodiscard]] float voxelEmissive(
            const DensityCell &cell
        ) noexcept {
            const float magnitude =
                    clamp01(static_cast<float>(cell.magnitude));

            return
                    smoothStep(0.025F, 0.82F, magnitude) *
                    (0.42F + 1.35F * std::sqrt(magnitude));
        }

        [[nodiscard]] float matrixSpan(
            const std::size_t dimension
        ) noexcept {
            if (dimension == 0U) {
                return cubeSide;
            }

            return
                    static_cast<float>(dimension - 1U) *
                    cellPitch +
                    cubeSide;
        }

        [[nodiscard]] constexpr float stackLayerGap() noexcept {
            // Fixed separation is architectural: even very long histories
            // must never compress neighboring matrix slabs into each other.
            return cubeSide * 1.23F;
        }

        void appendVoxel(
            InstanceScene &scene,
            const DensityCell &cell,
            const std::size_t layer,
            const Vector3 center,
            const Vector3 size
        ) {
            if (
                scene.pickRecords.size() >=
                static_cast<std::size_t>(1U << 24U)
            ) {
                throw std::runtime_error{
                    "Density Volume exceeds exact floating-point picking IDs."
                };
            }

            scene.pickRecords.push_back(
                Selection{
                    .layer = layer,
                    .row = cell.row,
                    .column = cell.column,
                    .magnitudeVoxel = true
                }
            );

            scene.voxels.push_back(
                VoxelInstance{
                    .center = center,
                    .size = size,
                    .color = voxelColor(cell, layer),
                    .emissive = voxelEmissive(cell),
                    .magnitude =
                        clamp01(static_cast<float>(cell.magnitude)),
                    .layer = static_cast<float>(layer),
                    .pickId =
                        static_cast<float>(scene.pickRecords.size())
                }
            );
        }

        [[nodiscard]] InstanceScene buildLayerStack(
            const DensityStack &stack
        ) {
            InstanceScene scene;

            if (stack.layers.empty()) {
                return scene;
            }

            std::size_t voxelCount{};

            for (const DensityLayer &layer : stack.layers) {
                if (
                    layer.cells.size() >
                    std::numeric_limits<std::size_t>::max() - voxelCount
                ) {
                    throw std::runtime_error{
                        "Density Volume instance count overflow."
                    };
                }

                voxelCount += layer.cells.size();
            }

            scene.voxels.reserve(voxelCount);
            scene.pickRecords.reserve(voxelCount);
            scene.layerEndInstanceCounts.reserve(stack.layers.size());

            const float span =
                    matrixSpan(stack.layers.back().dimension);

            const float layerGap =
                    stackLayerGap();

            const float halfMatrix =
                    static_cast<float>(
                        stack.layers.back().dimension - 1U
                    ) *
                    cellPitch *
                    0.5F;

            for (const DensityLayer &layer : stack.layers) {
                const float layerX =
                        static_cast<float>(layer.index) *
                        layerGap;

                for (const DensityCell &cell : layer.cells) {
                    const float magnitude =
                            clamp01(
                                static_cast<float>(cell.magnitude)
                            );

                    const float response =
                            std::pow(magnitude, 0.38F);

                    const float crossSection =
                            cubeSide * (0.82F + response * 0.18F);

                    const float depth =
                            cubeSide * (0.72F + response * 0.26F);

                    appendVoxel(
                        scene,
                        cell,
                        layer.index,
                        Vector3{
                            layerX,
                            halfMatrix -
                                static_cast<float>(cell.row) *
                                cellPitch,
                            static_cast<float>(cell.column) *
                                cellPitch -
                                halfMatrix
                        },
                        Vector3{
                            depth,
                            crossSection,
                            crossSection
                        }
                    );
                }

                scene.layerEndInstanceCounts.push_back(
                    scene.voxels.size()
                );
            }

            const float historyLength =
                    static_cast<float>(stack.layers.size() - 1U) *
                    layerGap +
                    cubeSide;

            scene.center = Vector3{
                (historyLength - cubeSide) * 0.5F,
                0.0F,
                0.0F
            };

            scene.radius =
                    std::sqrt(
                        std::pow(historyLength * 0.5F, 2.0F) +
                        std::pow(span * 0.5F, 2.0F) * 2.0F
                    ) * 1.08F;

            scene.groundCenter = Vector3{
                scene.center.x,
                -span * 0.5F - cellPitch * 0.92F,
                0.0F
            };

            scene.groundHalfExtentX =
                    historyLength * 0.64F + span * 0.34F;

            scene.groundHalfExtentZ =
                    span * 0.78F;

            return scene;
        }

        [[nodiscard]] InstanceScene buildFloorField(
            const DensityLayer &layer
        ) {
            InstanceScene scene;
            scene.voxels.reserve(layer.cells.size());
            scene.pickRecords.reserve(layer.cells.size());
            scene.layerEndInstanceCounts.assign(
                layer.index + 1U,
                0U
            );

            const float span =
                    matrixSpan(layer.dimension);

            const float halfMatrix =
                    static_cast<float>(layer.dimension - 1U) *
                    cellPitch *
                    0.5F;

            float maximumHeight =
                    0.08F;

            for (const DensityCell &cell : layer.cells) {
                const float magnitude =
                        clamp01(
                            static_cast<float>(cell.magnitude)
                        );

                const float height =
                        0.065F +
                        std::pow(magnitude, 0.52F) *
                        3.25F;

                maximumHeight =
                        std::max(maximumHeight, height);

                appendVoxel(
                    scene,
                    cell,
                    layer.index,
                    Vector3{
                        static_cast<float>(cell.column) *
                            cellPitch -
                            halfMatrix,
                        height * 0.5F,
                        static_cast<float>(cell.row) *
                            cellPitch -
                            halfMatrix
                    },
                    Vector3{
                        cubeSide,
                        height,
                        cubeSide
                    }
                );
            }

            scene.layerEndInstanceCounts[layer.index] =
                    scene.voxels.size();

            scene.center = Vector3{
                0.0F,
                maximumHeight * 0.32F,
                0.0F
            };

            scene.radius =
                    std::sqrt(
                        std::pow(span * 0.5F, 2.0F) * 2.0F +
                        std::pow(maximumHeight * 0.5F, 2.0F)
                    ) * 1.16F;

            scene.groundCenter = Vector3{
                0.0F,
                -0.035F,
                0.0F
            };

            scene.groundHalfExtentX =
                    span * 0.78F;

            scene.groundHalfExtentZ =
                    span * 0.78F;

            return scene;
        }

        [[nodiscard]] Vector3 roundedPoint(
            const Vector3 raw
        ) noexcept {
            constexpr float half = 0.5F;
            constexpr float inner = half - roundedRadius;

            const Vector3 nearest{
                std::clamp(raw.x, -inner, inner),
                std::clamp(raw.y, -inner, inner),
                std::clamp(raw.z, -inner, inner)
            };

            const Vector3 delta =
                    raw - nearest;

            return nearest + normalize(delta) * roundedRadius;
        }

        [[nodiscard]] Vector3 roundedNormal(
            const Vector3 raw
        ) noexcept {
            constexpr float half = 0.5F;
            constexpr float inner = half - roundedRadius;

            return normalize(
                raw -
                Vector3{
                    std::clamp(raw.x, -inner, inner),
                    std::clamp(raw.y, -inner, inner),
                    std::clamp(raw.z, -inner, inner)
                }
            );
        }
    }

    VoxelGeometry VoxelGeometryBuilder::buildRoundedCube() {
        struct Face {
            Vector3 normal;
            Vector3 tangent;
            Vector3 bitangent;
        };

        constexpr std::array<Face, 6U> faces{
            Face{
                Vector3{1.0F, 0.0F, 0.0F},
                Vector3{0.0F, 0.0F, -1.0F},
                Vector3{0.0F, 1.0F, 0.0F}
            },
            Face{
                Vector3{-1.0F, 0.0F, 0.0F},
                Vector3{0.0F, 0.0F, 1.0F},
                Vector3{0.0F, 1.0F, 0.0F}
            },
            Face{
                Vector3{0.0F, 1.0F, 0.0F},
                Vector3{1.0F, 0.0F, 0.0F},
                Vector3{0.0F, 0.0F, -1.0F}
            },
            Face{
                Vector3{0.0F, -1.0F, 0.0F},
                Vector3{1.0F, 0.0F, 0.0F},
                Vector3{0.0F, 0.0F, 1.0F}
            },
            Face{
                Vector3{0.0F, 0.0F, 1.0F},
                Vector3{1.0F, 0.0F, 0.0F},
                Vector3{0.0F, 1.0F, 0.0F}
            },
            Face{
                Vector3{0.0F, 0.0F, -1.0F},
                Vector3{-1.0F, 0.0F, 0.0F},
                Vector3{0.0F, 1.0F, 0.0F}
            }
        };

        VoxelGeometry geometry;

        constexpr std::size_t verticesPerSide =
                roundedSegments + 1U;

        geometry.vertices.reserve(
            faces.size() *
            verticesPerSide *
            verticesPerSide
        );

        geometry.indices.reserve(
            faces.size() *
            roundedSegments *
            roundedSegments *
            6U
        );

        for (const Face &face : faces) {
            const std::uint32_t faceStart =
                    static_cast<std::uint32_t>(
                        geometry.vertices.size()
                    );

            for (std::size_t row = 0U; row <= roundedSegments; ++row) {
                const float vertical =
                        -1.0F +
                        2.0F *
                        static_cast<float>(row) /
                        static_cast<float>(roundedSegments);

                for (std::size_t column = 0U; column <= roundedSegments; ++column) {
                    const float horizontal =
                            -1.0F +
                            2.0F *
                            static_cast<float>(column) /
                            static_cast<float>(roundedSegments);

                    const Vector3 raw =
                            face.normal * 0.5F +
                            face.tangent * (horizontal * 0.5F) +
                            face.bitangent * (vertical * 0.5F);

                    const Vector3 position =
                            roundedPoint(raw);

                    const Vector3 normal =
                            roundedNormal(raw);

                    geometry.vertices.push_back(
                        VoxelVertex{
                            .position = {
                                position.x,
                                position.y,
                                position.z
                            },
                            .normal = {
                                normal.x,
                                normal.y,
                                normal.z
                            }
                        }
                    );
                }
            }

            for (std::size_t row = 0U; row < roundedSegments; ++row) {
                for (std::size_t column = 0U; column < roundedSegments; ++column) {
                    const std::uint32_t lowerLeft =
                            faceStart +
                            static_cast<std::uint32_t>(
                                row * verticesPerSide + column
                            );

                    const std::uint32_t lowerRight =
                            lowerLeft + 1U;

                    const std::uint32_t upperLeft =
                            lowerLeft +
                            static_cast<std::uint32_t>(
                                verticesPerSide
                            );

                    const std::uint32_t upperRight =
                            upperLeft + 1U;

                    geometry.indices.push_back(lowerLeft);
                    geometry.indices.push_back(lowerRight);
                    geometry.indices.push_back(upperRight);
                    geometry.indices.push_back(lowerLeft);
                    geometry.indices.push_back(upperRight);
                    geometry.indices.push_back(upperLeft);
                }
            }
        }

        return geometry;
    }

    InstanceScene SceneBuilder::build(
        const DensityStack &stack,
        const std::size_t selectedLayer,
        const VisualizationMode mode
    ) {
        if (stack.layers.empty()) {
            return InstanceScene{};
        }

        const std::size_t safeLayer =
                std::min(
                    selectedLayer,
                    stack.layers.size() - 1U
                );

        return mode == VisualizationMode::LayerStack
                   ? buildLayerStack(stack)
                   : buildFloorField(stack.layers[safeLayer]);
    }
}

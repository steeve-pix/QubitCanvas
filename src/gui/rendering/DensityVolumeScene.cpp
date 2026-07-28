#include "quantum_sim/gui/rendering/DensityVolumeScene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace quantum_sim::gui::density_volume {
    namespace {
        constexpr float cellPitch = 1.0F;
        constexpr float cubeSide = 0.88F;
        constexpr float maximumFloorHeight = 4.4F;
        constexpr float solidVisibilityThreshold = 1.0e-4F;
        constexpr float roundedCubeRadius = 0.12F;
        constexpr std::size_t roundedCubeSegments = 4U;
        constexpr float roundedTopRadius = 0.22F;
        constexpr std::size_t roundedTopSegments = 8U;

        [[nodiscard]] float clamp01(const float value) noexcept {
            return std::clamp(value, 0.0F, 1.0F);
        }

        [[nodiscard]] float srgbToLinear(
            const float component
        ) noexcept {
            const float safeComponent =
                    std::max(component, 0.0F);

            if (safeComponent <= 0.04045F) {
                return safeComponent / 12.92F;
            }

            return std::pow(
                (safeComponent + 0.055F) / 1.055F,
                2.4F
            );
        }

        [[nodiscard]] Color linearColor(
            const Color &srgb
        ) noexcept {
            return Color{
                srgbToLinear(srgb.red),
                srgbToLinear(srgb.green),
                srgbToLinear(srgb.blue)
            };
        }

        [[nodiscard]] Color voxelColor(
            const float normalizedMagnitude
        ) noexcept {
            const float magnitude =
                    clamp01(
                        normalizedMagnitude
                    );

            const Color base =
                    linearColor(
                        magnitudeColor(magnitude)
                    );

            // The reference applies this gain after sRGB-to-linear conversion,
            // preserving the Inferno hue while giving bright cells HDR energy.
            const float glowGain =
                    1.0F + 1.9F * magnitude;

            return Color{
                base.red * glowGain,
                base.green * glowGain,
                base.blue * glowGain
            };
        }

        [[nodiscard]] Color ghostColor() noexcept {
            const Color base =
                    linearColor(
                        magnitudeColor(0.001)
                    );

            return Color{
                base.red * 0.82F,
                base.green * 0.82F,
                base.blue * 0.82F
            };
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
            return 1.15F;
        }

        void appendInstance(
            InstanceScene &scene,
            std::vector<VoxelInstance> &instances,
            const DensityCell &cell,
            const std::size_t layer,
            const Vector3 center,
            const Vector3 size,
            const Color color,
            const float emissive,
            const float visualMagnitude,
            const bool magnitudeVoxel
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
                    .magnitudeVoxel = magnitudeVoxel
                }
            );

            instances.push_back(
                VoxelInstance{
                    .center = center,
                    .size = size,
                    .color = color,
                    .emissive = emissive,
                    .magnitude = clamp01(visualMagnitude),
                    .layer = static_cast<float>(layer),
                    .pickId =
                        static_cast<float>(scene.pickRecords.size())
                }
            );
        }

        void appendSolidVoxel(
            InstanceScene &scene,
            const DensityCell &cell,
            const std::size_t layer,
            const Vector3 center,
            const Vector3 size,
            const float visualMagnitude
        ) {
            appendInstance(
                scene,
                scene.voxels,
                cell,
                layer,
                center,
                size,
                voxelColor(visualMagnitude),
                clamp01(visualMagnitude),
                visualMagnitude,
                true
            );
        }

        void appendGhostVoxel(
            InstanceScene &scene,
            const DensityCell &cell,
            const std::size_t layer,
            const Vector3 center,
            const Vector3 size
        ) {
            appendInstance(
                scene,
                scene.ghostVoxels,
                cell,
                layer,
                center,
                size,
                ghostColor(),
                0.0F,
                0.0F,
                false
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
            scene.ghostVoxels.reserve(voxelCount);
            scene.pickRecords.reserve(voxelCount);
            scene.layerEndInstanceCounts.reserve(stack.layers.size());
            scene.layerEndGhostCounts.reserve(stack.layers.size());
            scene.layerCenters.reserve(stack.layers.size());

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

            const double displayReferenceMagnitude =
                    stack.maximumMagnitude;

            for (const DensityLayer &layer : stack.layers) {
                const float layerX =
                        static_cast<float>(layer.index) *
                        layerGap;

                const Vector3 layerCenter{
                    layerX,
                    0.0F,
                    0.0F
                };

                scene.layerCenters.push_back(
                    layerCenter
                );

                const bool showGhosts =
                        !layer.bucketed &&
                        layer.dimension <= 16U;

                for (const DensityCell &cell : layer.cells) {
                    const float rawMagnitude =
                            clamp01(
                                static_cast<float>(cell.magnitude)
                            );

                    const float displayMagnitude =
                            static_cast<float>(
                                normalizeMagnitude(
                                    cell.magnitude,
                                    displayReferenceMagnitude
                                )
                            );

                    const Vector3 center{
                        layerX,
                        halfMatrix -
                            static_cast<float>(cell.row) *
                            cellPitch,
                        static_cast<float>(cell.column) *
                            cellPitch -
                            halfMatrix
                    };

                    constexpr Vector3 size{
                        cubeSide,
                        cubeSide,
                        cubeSide
                    };

                    if (rawMagnitude >= solidVisibilityThreshold) {
                        appendSolidVoxel(
                            scene,
                            cell,
                            layer.index,
                            center,
                            size,
                            displayMagnitude
                        );
                    } else if (showGhosts) {
                        appendGhostVoxel(
                            scene,
                            cell,
                            layer.index,
                            center,
                            size
                        );
                    }
                }

                scene.layerEndInstanceCounts.push_back(
                    scene.voxels.size()
                );

                scene.layerEndGhostCounts.push_back(
                    scene.ghostVoxels.size()
                );
            }

            const float historyWidth =
                    static_cast<float>(stack.layers.size() - 1U) *
                    layerGap +
                    cubeSide;

            scene.center = Vector3{
                (historyWidth - cubeSide) * 0.5F,
                0.0F,
                0.0F
            };

            scene.framingMinimum = Vector3{
                -cubeSide * 0.5F,
                -span * 0.5F,
                -span * 0.5F
            };

            scene.framingMaximum = Vector3{
                historyWidth - cubeSide * 0.5F,
                span * 0.5F,
                span * 0.5F
            };

            scene.matrixSpan = span;
            scene.layerSpacing = layerGap;
            scene.voxelSide = cubeSide;

            scene.radius =
                    std::sqrt(
                        std::pow(historyWidth * 0.5F, 2.0F) +
                        std::pow(span * 0.5F, 2.0F) * 2.0F
                    ) * 1.08F;

            scene.groundCenter = Vector3{
                (historyWidth - cubeSide) * 0.5F,
                -span * 0.5F - cubeSide * 0.5F - cellPitch * 0.22F,
                0.0F
            };

            scene.groundHalfExtentX =
                    std::max(
                        historyWidth * 0.58F,
                        span * 0.78F
                    );

            scene.groundHalfExtentZ =
                    span * 0.78F;

            return scene;
        }

        [[nodiscard]] InstanceScene buildIsolatedLayer(
            const DensityLayer &layer,
            const double displayReferenceMagnitude
        ) {
            InstanceScene scene;
            scene.voxels.reserve(layer.cells.size());
            scene.ghostVoxels.reserve(layer.cells.size());
            scene.pickRecords.reserve(layer.cells.size());
            scene.layerEndInstanceCounts.assign(
                layer.index + 1U,
                0U
            );
            scene.layerEndGhostCounts.assign(
                layer.index + 1U,
                0U
            );
            scene.layerCenters.assign(
                layer.index + 1U,
                Vector3{}
            );

            const float span =
                    matrixSpan(layer.dimension);

            const float halfMatrix =
                    static_cast<float>(
                        layer.dimension - 1U
                    ) *
                    cellPitch *
                    0.5F;

            const bool showGhosts =
                    !layer.bucketed &&
                    layer.dimension <= 16U;

            for (const DensityCell &cell : layer.cells) {
                const float rawMagnitude =
                        clamp01(
                            static_cast<float>(
                                cell.magnitude
                            )
                        );

                const float displayMagnitude =
                        static_cast<float>(
                            normalizeMagnitude(
                                cell.magnitude,
                                displayReferenceMagnitude
                            )
                        );

                const Vector3 center{
                    0.0F,
                    halfMatrix -
                        static_cast<float>(cell.row) *
                        cellPitch,
                    static_cast<float>(cell.column) *
                        cellPitch -
                        halfMatrix
                };

                constexpr Vector3 size{
                    cubeSide,
                    cubeSide,
                    cubeSide
                };

                if (rawMagnitude >= solidVisibilityThreshold) {
                    appendSolidVoxel(
                        scene,
                        cell,
                        layer.index,
                        center,
                        size,
                        displayMagnitude
                    );
                } else if (showGhosts) {
                    appendGhostVoxel(
                        scene,
                        cell,
                        layer.index,
                        center,
                        size
                    );
                }
            }

            scene.layerEndInstanceCounts[layer.index] =
                    scene.voxels.size();

            scene.layerEndGhostCounts[layer.index] =
                    scene.ghostVoxels.size();

            scene.center = Vector3{};
            scene.framingMinimum = Vector3{
                -cubeSide * 0.5F,
                -span * 0.5F,
                -span * 0.5F
            };
            scene.framingMaximum = Vector3{
                cubeSide * 0.5F,
                span * 0.5F,
                span * 0.5F
            };
            scene.matrixSpan = span;
            scene.layerSpacing = stackLayerGap();
            scene.voxelSide = cubeSide;
            scene.radius =
                    std::sqrt(
                        std::pow(span * 0.5F, 2.0F) *
                        2.0F +
                        std::pow(cubeSide * 0.5F, 2.0F)
                    ) *
                    1.08F;
            scene.groundCenter = Vector3{
                0.0F,
                -span * 0.5F -
                    cubeSide * 0.5F -
                    cellPitch * 0.22F,
                0.0F
            };
            scene.groundHalfExtentX =
                    std::max(span * 0.78F, 2.5F);
            scene.groundHalfExtentZ =
                    std::max(span * 0.78F, 2.5F);

            return scene;
        }

        [[nodiscard]] InstanceScene buildFloorField(
            const DensityLayer &layer,
            const double displayReferenceMagnitude
        ) {
            InstanceScene scene;
            scene.voxels.reserve(layer.cells.size());
            scene.ghostVoxels.reserve(layer.cells.size());
            scene.pickRecords.reserve(layer.cells.size());
            scene.layerEndInstanceCounts.assign(
                layer.index + 1U,
                0U
            );
            scene.layerEndGhostCounts.assign(
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

            const bool showGhosts =
                    !layer.bucketed &&
                    layer.dimension <= 16U;

            const double layerMaximumMagnitude =
                    layer.maximumCellMagnitude();

            for (const DensityCell &cell : layer.cells) {
                const float rawMagnitude =
                        clamp01(
                            static_cast<float>(cell.magnitude)
                        );

                const float normalizedMagnitude =
                        static_cast<float>(
                            normalizeMagnitude(
                                cell.magnitude,
                                layerMaximumMagnitude
                            )
                        );

                const float colorMagnitude =
                        static_cast<float>(
                            normalizeMagnitude(
                                cell.magnitude,
                                displayReferenceMagnitude
                            )
                        );

                const float height =
                        std::max(
                            0.012F,
                            normalizedMagnitude *
                            maximumFloorHeight
                        );

                maximumHeight =
                        std::max(maximumHeight, height);

                const Vector3 center{
                    static_cast<float>(cell.column) *
                        cellPitch -
                        halfMatrix,
                    height * 0.5F,
                    static_cast<float>(cell.row) *
                        cellPitch -
                        halfMatrix
                };

                if (rawMagnitude >= solidVisibilityThreshold) {
                    appendSolidVoxel(
                        scene,
                        cell,
                        layer.index,
                        center,
                        Vector3{
                            cubeSide,
                            height,
                            cubeSide
                        },
                        colorMagnitude
                    );
                } else if (showGhosts) {
                    appendGhostVoxel(
                        scene,
                        cell,
                        layer.index,
                        Vector3{
                            center.x,
                            0.03F,
                            center.z
                        },
                        Vector3{
                            cubeSide,
                            0.06F,
                            cubeSide
                        }
                    );
                }
            }

            scene.layerEndInstanceCounts[layer.index] =
                    scene.voxels.size();

            scene.layerEndGhostCounts[layer.index] =
                    scene.ghostVoxels.size();

            scene.center = Vector3{
                0.0F,
                maximumHeight * 0.32F,
                0.0F
            };

            scene.framingMinimum = Vector3{
                -span * 0.5F,
                0.0F,
                -span * 0.5F
            };

            scene.framingMaximum = Vector3{
                span * 0.5F,
                maximumHeight,
                span * 0.5F
            };

            scene.matrixSpan = span;
            scene.layerSpacing = stackLayerGap();
            scene.voxelSide = cubeSide;

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
            const Vector3 raw,
            const float radius,
            const bool topOnly
        ) noexcept {
            constexpr float half = 0.5F;
            const float inner = half - radius;

            if (topOnly && raw.y <= inner) {
                return raw;
            }

            const Vector3 nearest{
                std::clamp(raw.x, -inner, inner),
                topOnly
                    ? std::min(raw.y, inner)
                    : std::clamp(raw.y, -inner, inner),
                std::clamp(raw.z, -inner, inner)
            };

            const Vector3 delta =
                    raw - nearest;

            return nearest + normalize(delta) * radius;
        }

        [[nodiscard]] Vector3 roundedNormal(
            const Vector3 raw,
            const Vector3 fallbackNormal,
            const float radius,
            const bool topOnly
        ) noexcept {
            constexpr float half = 0.5F;
            const float inner = half - radius;

            if (topOnly && raw.y <= inner) {
                return fallbackNormal;
            }

            return normalize(
                raw -
                Vector3{
                    std::clamp(raw.x, -inner, inner),
                    topOnly
                        ? std::min(raw.y, inner)
                        : std::clamp(raw.y, -inner, inner),
                    std::clamp(raw.z, -inner, inner)
                }
            );
        }

        [[nodiscard]] VoxelGeometry buildRoundedGeometry(
            const float radius,
            const std::size_t segmentCount,
            const bool topOnly
        ) {
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

            const std::size_t verticesPerSide =
                    segmentCount + 1U;

            geometry.vertices.reserve(
                faces.size() *
                verticesPerSide *
                verticesPerSide
            );

            geometry.indices.reserve(
                faces.size() *
                segmentCount *
                segmentCount *
                6U
            );

            for (const Face &face : faces) {
                const std::uint32_t faceStart =
                        static_cast<std::uint32_t>(
                            geometry.vertices.size()
                        );

                for (std::size_t row = 0U; row <= segmentCount; ++row) {
                    const float vertical =
                            -1.0F +
                            2.0F *
                            static_cast<float>(row) /
                            static_cast<float>(segmentCount);

                    for (std::size_t column = 0U; column <= segmentCount; ++column) {
                        const float horizontal =
                                -1.0F +
                                2.0F *
                                static_cast<float>(column) /
                                static_cast<float>(segmentCount);

                        const Vector3 raw =
                                face.normal * 0.5F +
                                face.tangent * (horizontal * 0.5F) +
                                face.bitangent * (vertical * 0.5F);

                        const Vector3 position =
                                roundedPoint(
                                    raw,
                                    radius,
                                    topOnly
                                );

                        const Vector3 normal =
                                roundedNormal(
                                    raw,
                                    face.normal,
                                    radius,
                                    topOnly
                                );

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

                for (std::size_t row = 0U; row < segmentCount; ++row) {
                    for (std::size_t column = 0U; column < segmentCount; ++column) {
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
    }

    VoxelGeometry VoxelGeometryBuilder::buildRoundedCube() {
        return buildRoundedGeometry(
            roundedCubeRadius,
            roundedCubeSegments,
            false
        );
    }

    VoxelGeometry VoxelGeometryBuilder::buildRoundedTopColumn() {
        return buildRoundedGeometry(
            roundedTopRadius,
            roundedTopSegments,
            true
        );
    }

    InstanceScene SceneBuilder::build(
        const DensityStack &stack,
        const std::size_t selectedLayer,
        const VisualizationMode mode,
        const SceneViewOptions &options
    ) {
        if (stack.layers.empty()) {
            return InstanceScene{};
        }

        const std::size_t safeLayer =
                std::min(
                    selectedLayer,
                    stack.layers.size() - 1U
                );

        if (options.comparisonLayer.has_value()) {
            const std::size_t safeReferenceLayer =
                    std::min(
                        options.comparisonLayer.value(),
                        stack.layers.size() - 1U
                    );

            const DensityLayer differenceLayer =
                    DensityModel::difference(
                        stack.layers[safeLayer],
                        stack.layers[safeReferenceLayer]
                    );

            return mode == VisualizationMode::LayerStack
                       ? buildIsolatedLayer(
                           differenceLayer,
                           stack.maximumMagnitude
                       )
                       : buildFloorField(
                           differenceLayer,
                           stack.maximumMagnitude
                       );
        }

        if (
            mode == VisualizationMode::LayerStack &&
            options.isolateSelectedLayer
        ) {
            return buildIsolatedLayer(
                stack.layers[safeLayer],
                stack.maximumMagnitude
            );
        }

        return mode == VisualizationMode::LayerStack
                   ? buildLayerStack(stack)
                   : buildFloorField(
                       stack.layers[safeLayer],
                       stack.maximumMagnitude
                   );
    }
}

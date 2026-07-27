#include "quantum_sim/gui/rendering/DensityVolumeRenderer.hpp"

#include <glad/gl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace quantum_sim::gui::density_volume {
    namespace {
        /**
         * Axis-aligned bounds for the visible solid portion of an instance scene.
         */
        struct VisibleVoxelBounds {
            Vector3 minimum;
            Vector3 maximum;
            Vector3 center;
            float radius{1.0F};
            bool valid{false};
        };

        /**
         * Measures only instances currently submitted to the solid voxel pass.
         *
         * @param voxels Complete ordered instance list.
         * @param visibleCount Prefix length rendered for the selected layer.
         * @return Bounds around visible solids, or an invalid result when empty.
         */
        [[nodiscard]] VisibleVoxelBounds calculateVisibleVoxelBounds(
            const std::vector<VoxelInstance> &voxels,
            const std::size_t visibleCount
        ) noexcept {
            const std::size_t safeCount =
                    std::min(
                        visibleCount,
                        voxels.size()
                    );

            if (safeCount == 0U) {
                return VisibleVoxelBounds{};
            }

            VisibleVoxelBounds bounds;
            bounds.minimum = Vector3{
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()
            };
            bounds.maximum = Vector3{
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest()
            };

            for (std::size_t index = 0U; index < safeCount; ++index) {
                const VoxelInstance &voxel =
                        voxels[index];

                const Vector3 halfSize{
                    voxel.size.x * 0.5F,
                    voxel.size.y * 0.5F,
                    voxel.size.z * 0.5F
                };

                bounds.minimum.x =
                        std::min(
                            bounds.minimum.x,
                            voxel.center.x - halfSize.x
                        );
                bounds.minimum.y =
                        std::min(
                            bounds.minimum.y,
                            voxel.center.y - halfSize.y
                        );
                bounds.minimum.z =
                        std::min(
                            bounds.minimum.z,
                            voxel.center.z - halfSize.z
                        );
                bounds.maximum.x =
                        std::max(
                            bounds.maximum.x,
                            voxel.center.x + halfSize.x
                        );
                bounds.maximum.y =
                        std::max(
                            bounds.maximum.y,
                            voxel.center.y + halfSize.y
                        );
                bounds.maximum.z =
                        std::max(
                            bounds.maximum.z,
                            voxel.center.z + halfSize.z
                        );
            }

            bounds.center = Vector3{
                (bounds.minimum.x + bounds.maximum.x) * 0.5F,
                (bounds.minimum.y + bounds.maximum.y) * 0.5F,
                (bounds.minimum.z + bounds.maximum.z) * 0.5F
            };

            const Vector3 halfExtent{
                (bounds.maximum.x - bounds.minimum.x) * 0.5F,
                (bounds.maximum.y - bounds.minimum.y) * 0.5F,
                (bounds.maximum.z - bounds.minimum.z) * 0.5F
            };

            bounds.radius =
                    std::max(
                        length(halfExtent),
                        0.5F
                    );
            bounds.valid = true;
            return bounds;
        }

        [[nodiscard]] unsigned int compileShader(
            const unsigned int shaderType,
            const char *source,
            const char *label
        ) {
            const unsigned int shader =
                    glCreateShader(shaderType);

            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);

            int compiled{};
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

            if (compiled == GL_TRUE) {
                return shader;
            }

            int logLength{};
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

            std::string log(
                static_cast<std::size_t>(std::max(logLength, 1)),
                '\0'
            );

            glGetShaderInfoLog(
                shader,
                logLength,
                nullptr,
                log.data()
            );

            glDeleteShader(shader);

            throw std::runtime_error{
                std::string{"Failed to compile "} + label + ": " + log
            };
        }

        [[nodiscard]] unsigned int createProgram(
            const char *vertexSource,
            const char *fragmentSource,
            const char *label
        ) {
            const unsigned int vertexShader =
                    compileShader(
                        GL_VERTEX_SHADER,
                        vertexSource,
                        label
                    );

            unsigned int fragmentShader{};
            unsigned int program{};

            try {
                fragmentShader =
                        compileShader(
                            GL_FRAGMENT_SHADER,
                            fragmentSource,
                            label
                        );

                program =
                        glCreateProgram();

                glAttachShader(program, vertexShader);
                glAttachShader(program, fragmentShader);
                glLinkProgram(program);

                int linked{};
                glGetProgramiv(program, GL_LINK_STATUS, &linked);

                if (linked != GL_TRUE) {
                    int logLength{};
                    glGetProgramiv(
                        program,
                        GL_INFO_LOG_LENGTH,
                        &logLength
                    );

                    std::string log(
                        static_cast<std::size_t>(
                            std::max(logLength, 1)
                        ),
                        '\0'
                    );

                    glGetProgramInfoLog(
                        program,
                        logLength,
                        nullptr,
                        log.data()
                    );

                    throw std::runtime_error{
                        std::string{"Failed to link "} +
                        label +
                        ": " +
                        log
                    };
                }
            } catch (...) {
                if (program != 0U) {
                    glDeleteProgram(program);
                }

                glDeleteShader(vertexShader);

                if (fragmentShader != 0U) {
                    glDeleteShader(fragmentShader);
                }

                throw;
            }

            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            return program;
        }

        void requireUniform(
            const int location,
            const char *name
        ) {
            if (location < 0) {
                throw std::runtime_error{
                    std::string{
                        "Density Volume shader uniform is unavailable: "
                    } +
                    name
                };
            }
        }
    }

    void Renderer::initialize() {
        if (initialized_) {
            return;
        }

        try {
            voxelGeometry_ =
                    VoxelGeometryBuilder::buildRoundedCube();

            createScenePrograms();
            createPostProcessPrograms();
            createVoxelBuffers();
            createGhostBuffers();
            createGridBuffers();
            createPostProcessVertexArray();

            glGenFramebuffers(1, &framebuffer_);
            glGenFramebuffers(1, &compositeFramebuffer_);
            glGenFramebuffers(2, blurFramebuffers_);
            glGenTextures(1, &colorTexture_);
            glGenTextures(1, &sceneColorTexture_);
            glGenTextures(1, &brightTexture_);
            glGenTextures(1, &pickTexture_);
            glGenTextures(2, blurTextures_);
            glGenRenderbuffers(1, &depthStencilBuffer_);
            initialized_ = true;
        } catch (...) {
            shutdown();
            throw;
        }
    }

    bool Renderer::updateScene(
        const DensityStack &stack,
        const std::size_t selectedLayer,
        const VisualizationMode mode
    ) {
        if (!initialized_) {
            throw std::runtime_error{
                "Density Volume renderer is not initialized."
            };
        }

        const bool historyChanged =
                sceneFingerprint_ != stack.fingerprint ||
                !sceneMode_.has_value() ||
                sceneMode_.value() != mode;

        if (stack.layers.empty()) {
            const bool changed =
                    historyChanged ||
                    !scene_.voxels.empty() ||
                    sceneSelectedLayer_.has_value();

            if (!changed) {
                return false;
            }

            scene_ = InstanceScene{};
            uploadInstances();
            visibleInstanceCount_ = 0;
            visibleGhostCount_ = 0;
            sceneFingerprint_ = stack.fingerprint;
            sceneSelectedLayer_.reset();
            sceneMode_ = mode;
            sceneCenter_ = Vector3{};
            sceneFocusRadius_ = 1.0F;
            sceneRadius_ = 1.0F;
            return true;
        }

        const std::size_t safeSelectedLayer =
                std::min(
                    selectedLayer,
                    stack.layers.size() - 1U
                );

        const bool selectionChanged =
                !sceneSelectedLayer_.has_value() ||
                sceneSelectedLayer_.value() != safeSelectedLayer;

        if (
            mode == VisualizationMode::LayerStack &&
            historyChanged
        ) {
            scene_ =
                    SceneBuilder::build(
                        stack,
                        safeSelectedLayer,
                        mode
                    );

            uploadInstances();
        } else if (
            mode == VisualizationMode::FloorField &&
            (historyChanged || selectionChanged)
        ) {
            scene_ =
                    SceneBuilder::build(
                        stack,
                        safeSelectedLayer,
                        mode
                    );

            uploadInstances();
        } else if (!historyChanged && !selectionChanged) {
            return false;
        }

        if (mode == VisualizationMode::LayerStack) {
            const std::size_t visibleSolidCount =
                    scene_.layerEndInstanceCounts.at(
                        safeSelectedLayer
                    );

            const std::size_t visibleGhostCount =
                    scene_.layerEndGhostCounts.at(
                        safeSelectedLayer
                    );

            if (
                visibleSolidCount >
                    static_cast<std::size_t>(
                        std::numeric_limits<int>::max()
                    ) ||
                visibleGhostCount >
                static_cast<std::size_t>(
                    std::numeric_limits<int>::max()
                )
            ) {
                throw std::runtime_error{
                    "Density Volume instance count exceeds OpenGL limits."
                };
            }

            visibleInstanceCount_ =
                    static_cast<int>(visibleSolidCount);

            visibleGhostCount_ =
                    static_cast<int>(visibleGhostCount);

            const VisibleVoxelBounds visibleBounds =
                    calculateVisibleVoxelBounds(
                        scene_.voxels,
                        visibleSolidCount
                    );

            if (visibleBounds.valid) {
                const float matrixHalfSpan =
                        scene_.matrixSpan * 0.5F;

                sceneCenter_ =
                        safeSelectedLayer <
                            scene_.layerCenters.size()
                            ? scene_.layerCenters[
                                safeSelectedLayer
                            ]
                            : visibleBounds.center;

                sceneFocusRadius_ =
                        std::max(
                            std::sqrt(
                                matrixHalfSpan *
                                    matrixHalfSpan *
                                    2.0F +
                                scene_.voxelSide *
                                    scene_.voxelSide *
                                    0.25F
                            ) *
                            1.12F,
                            1.4F
                        );

                sceneRadius_ =
                        std::max(
                            visibleBounds.radius * 1.18F,
                            sceneFocusRadius_
                        );

                scene_.groundCenter = Vector3{
                    0.0F,
                    visibleBounds.minimum.y -
                        scene_.voxelSide * 1.15F,
                    0.0F
                };

                scene_.groundHalfExtentX =
                        std::max(
                            scene_.matrixSpan * 0.78F,
                            2.5F
                        );

                scene_.groundHalfExtentZ =
                        scene_.groundHalfExtentX;
            } else {
                sceneCenter_ =
                        safeSelectedLayer <
                            scene_.layerCenters.size()
                            ? scene_.layerCenters[
                                safeSelectedLayer
                            ]
                            : scene_.center;
                sceneFocusRadius_ =
                        std::max(
                            scene_.matrixSpan * 0.8F,
                            1.4F
                        );
                sceneRadius_ =
                        std::max(
                            scene_.radius,
                            sceneFocusRadius_
                        );
            }
        } else {
            visibleInstanceCount_ =
                    static_cast<int>(scene_.voxels.size());
            visibleGhostCount_ =
                    static_cast<int>(scene_.ghostVoxels.size());
            sceneCenter_ = scene_.center;
            sceneFocusRadius_ = scene_.radius;
            sceneRadius_ = scene_.radius;
        }

        sceneFingerprint_ = stack.fingerprint;
        sceneSelectedLayer_ = safeSelectedLayer;
        sceneMode_ = mode;
        return true;
    }

    void Renderer::render(
        const int width,
        const int height,
        const std::size_t selectedLayer,
        const float heatAmount,
        const CameraController &camera
    ) {
        if (!initialized_) {
            throw std::runtime_error{
                "Density Volume renderer is not initialized."
            };
        }

        const int safeWidth =
                std::max(width, 1);

        const int safeHeight =
                std::max(height, 1);

        if (
            safeWidth != framebufferWidth_ ||
            safeHeight != framebufferHeight_
        ) {
            resizeFramebuffer(safeWidth, safeHeight);
        }

        int previousFramebuffer{};
        int previousViewport[4]{};

        glGetIntegerv(
            GL_FRAMEBUFFER_BINDING,
            &previousFramebuffer
        );

        glGetIntegerv(
            GL_VIEWPORT,
            previousViewport
        );

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
        glViewport(0, 0, safeWidth, safeHeight);

        constexpr std::array<float, 4U> clearScene{
            0.0018F,
            0.0045F,
            0.0110F,
            1.0F
        };

        constexpr std::array<float, 4U> clearBright{
            0.0F,
            0.0F,
            0.0F,
            1.0F
        };

        constexpr std::array<unsigned int, 4U> clearPick{
            0U,
            0U,
            0U,
            0U
        };

        glClearBufferfv(
            GL_COLOR,
            0,
            clearScene.data()
        );

        glClearBufferuiv(
            GL_COLOR,
            1,
            clearPick.data()
        );

        glClearBufferfv(
            GL_COLOR,
            2,
            clearBright.data()
        );

        glClear(
            GL_DEPTH_BUFFER_BIT |
            GL_STENCIL_BUFFER_BIT
        );

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        const Matrix4 view =
                camera.viewMatrix();

        const Matrix4 projection =
                camera.projectionMatrix(
                    safeWidth,
                    safeHeight
                );

        glDisable(GL_CULL_FACE);
        glUseProgram(gridShaderProgram_);
        glUniformMatrix4fv(
            gridViewUniform_,
            1,
            GL_FALSE,
            view.data()
        );

        glUniformMatrix4fv(
            gridProjectionUniform_,
            1,
            GL_FALSE,
            projection.data()
        );

        glUniform3f(
            gridCenterUniform_,
            scene_.groundCenter.x,
            scene_.groundCenter.y,
            scene_.groundCenter.z
        );

        glUniform2f(
            gridExtentUniform_,
            scene_.groundHalfExtentX,
            scene_.groundHalfExtentZ
        );

        glBindVertexArray(gridVertexArray_);
        glDrawElements(
            GL_TRIANGLES,
            6,
            GL_UNSIGNED_INT,
            nullptr
        );

        if (visibleInstanceCount_ > 0) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);
            glUseProgram(voxelShaderProgram_);

            glUniformMatrix4fv(
                voxelViewUniform_,
                1,
                GL_FALSE,
                view.data()
            );

            glUniformMatrix4fv(
                voxelProjectionUniform_,
                1,
                GL_FALSE,
                projection.data()
            );

            glUniform1f(
                voxelSelectedLayerUniform_,
                static_cast<float>(selectedLayer)
            );

            glUniform1f(
                voxelHeatUniform_,
                std::clamp(
                    heatAmount / 0.78F,
                    0.45F,
                    1.70F
                )
            );

            glBindVertexArray(voxelVertexArray_);
            glDrawElementsInstanced(
                GL_TRIANGLES,
                cubeIndexCount_,
                GL_UNSIGNED_INT,
                nullptr,
                visibleInstanceCount_
            );
        }

        if (visibleGhostCount_ > 0) {
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            glUseProgram(ghostShaderProgram_);

            glUniformMatrix4fv(
                ghostViewUniform_,
                1,
                GL_FALSE,
                view.data()
            );

            glUniformMatrix4fv(
                ghostProjectionUniform_,
                1,
                GL_FALSE,
                projection.data()
            );

            glUniform1f(
                ghostSelectedLayerUniform_,
                static_cast<float>(selectedLayer)
            );

            glBindVertexArray(ghostVertexArray_);
            glDrawElementsInstanced(
                GL_LINES,
                24,
                GL_UNSIGNED_INT,
                nullptr,
                visibleGhostCount_
            );

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }

        renderPostProcess(
            safeWidth,
            safeHeight
        );

        glBindVertexArray(0);
        glUseProgram(0);
        glBindFramebuffer(
            GL_FRAMEBUFFER,
            static_cast<unsigned int>(previousFramebuffer)
        );

        glViewport(
            previousViewport[0],
            previousViewport[1],
            previousViewport[2],
            previousViewport[3]
        );
    }

    std::optional<Selection> Renderer::pick(
        const int x,
        const int y
    ) const {
        if (
            !initialized_ ||
            x < 0 ||
            y < 0 ||
            x >= framebufferWidth_ ||
            y >= framebufferHeight_
        ) {
            return std::nullopt;
        }

        int previousReadFramebuffer{};
        glGetIntegerv(
            GL_READ_FRAMEBUFFER_BINDING,
            &previousReadFramebuffer
        );

        glBindFramebuffer(
            GL_READ_FRAMEBUFFER,
            framebuffer_
        );

        glReadBuffer(GL_COLOR_ATTACHMENT1);

        unsigned int pickId{};
        glReadPixels(
            x,
            y,
            1,
            1,
            GL_RED_INTEGER,
            GL_UNSIGNED_INT,
            &pickId
        );

        glBindFramebuffer(
            GL_READ_FRAMEBUFFER,
            static_cast<unsigned int>(
                previousReadFramebuffer
            )
        );

        if (
            pickId == 0U ||
            pickId > scene_.pickRecords.size()
        ) {
            return std::nullopt;
        }

        return scene_.pickRecords.at(pickId - 1U);
    }

    void Renderer::shutdown() noexcept {
        if (depthStencilBuffer_ != 0U) {
            glDeleteRenderbuffers(
                1,
                &depthStencilBuffer_
            );
        }

        glDeleteTextures(1, &colorTexture_);
        glDeleteTextures(1, &sceneColorTexture_);
        glDeleteTextures(1, &brightTexture_);
        glDeleteTextures(1, &pickTexture_);
        glDeleteTextures(2, blurTextures_);
        glDeleteFramebuffers(1, &framebuffer_);
        glDeleteFramebuffers(1, &compositeFramebuffer_);
        glDeleteFramebuffers(2, blurFramebuffers_);
        glDeleteVertexArrays(1, &postProcessVertexArray_);
        glDeleteVertexArrays(1, &gridVertexArray_);
        glDeleteBuffers(1, &gridVertexBuffer_);
        glDeleteBuffers(1, &gridIndexBuffer_);
        glDeleteVertexArrays(1, &voxelVertexArray_);
        glDeleteBuffers(1, &voxelVertexBuffer_);
        glDeleteBuffers(1, &voxelIndexBuffer_);
        glDeleteBuffers(1, &instanceBuffer_);
        glDeleteVertexArrays(1, &ghostVertexArray_);
        glDeleteBuffers(1, &ghostVertexBuffer_);
        glDeleteBuffers(1, &ghostIndexBuffer_);
        glDeleteBuffers(1, &ghostInstanceBuffer_);
        glDeleteProgram(voxelShaderProgram_);
        glDeleteProgram(ghostShaderProgram_);
        glDeleteProgram(gridShaderProgram_);
        glDeleteProgram(blurShaderProgram_);
        glDeleteProgram(compositeShaderProgram_);

        voxelVertexArray_ = 0U;
        voxelVertexBuffer_ = 0U;
        voxelIndexBuffer_ = 0U;
        instanceBuffer_ = 0U;
        ghostVertexArray_ = 0U;
        ghostVertexBuffer_ = 0U;
        ghostIndexBuffer_ = 0U;
        ghostInstanceBuffer_ = 0U;
        gridVertexArray_ = 0U;
        gridVertexBuffer_ = 0U;
        gridIndexBuffer_ = 0U;
        voxelShaderProgram_ = 0U;
        ghostShaderProgram_ = 0U;
        gridShaderProgram_ = 0U;
        blurShaderProgram_ = 0U;
        compositeShaderProgram_ = 0U;
        postProcessVertexArray_ = 0U;
        framebuffer_ = 0U;
        colorTexture_ = 0U;
        sceneColorTexture_ = 0U;
        brightTexture_ = 0U;
        pickTexture_ = 0U;
        depthStencilBuffer_ = 0U;
        blurFramebuffers_[0] = 0U;
        blurFramebuffers_[1] = 0U;
        blurTextures_[0] = 0U;
        blurTextures_[1] = 0U;
        compositeFramebuffer_ = 0U;
        framebufferWidth_ = 0;
        framebufferHeight_ = 0;
        cubeIndexCount_ = 0;
        visibleInstanceCount_ = 0;
        visibleGhostCount_ = 0;
        instanceCapacity_ = 0U;
        ghostInstanceCapacity_ = 0U;
        sceneFingerprint_ = 0U;
        sceneSelectedLayer_.reset();
        sceneMode_.reset();
        scene_ = InstanceScene{};
        voxelGeometry_ = VoxelGeometry{};
        sceneCenter_ = Vector3{};
        sceneRadius_ = 1.0F;
        initialized_ = false;
    }

    unsigned int Renderer::colorTexture() const noexcept {
        return colorTexture_;
    }

    Vector3 Renderer::sceneCenter() const noexcept {
        return sceneCenter_;
    }

    float Renderer::sceneFocusRadius() const noexcept {
        return sceneFocusRadius_;
    }

    float Renderer::sceneRadius() const noexcept {
        return sceneRadius_;
    }

    bool Renderer::isInitialized() const noexcept {
        return initialized_;
    }

    void Renderer::createScenePrograms() {
        constexpr const char *voxelVertexShader = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aInstanceCenter;
layout(location = 3) in vec3 aInstanceSize;
layout(location = 4) in vec3 aInstanceColor;
layout(location = 5) in vec4 aInstanceMaterial;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPosition;
out vec3 vViewPosition;
out vec3 vNormal;
out vec3 vLocalPosition;
out vec3 vColor;
flat out vec4 vMaterial;

void main() {
    vec3 safeSize = max(aInstanceSize, vec3(0.001));
    vec3 roundedNormal = normalize(aNormal);

    // Reconstruct the rounded box in world units so a tall Floor Field column
    // keeps the same corner radius as a regular voxel. Directly scaling the
    // shared mesh would stretch its top and bottom curves into a capsule.
    const float unitRadius = 0.22;
    const float unitInnerExtent = 0.50 - unitRadius;
    float worldRadius =
        min(min(safeSize.x, safeSize.y), safeSize.z) *
        unitRadius;
    vec3 unitNearest =
        aPosition -
        roundedNormal * unitRadius;
    vec3 normalizedNearest =
        unitNearest /
        unitInnerExtent;
    vec3 worldInnerExtent =
        max(
            safeSize * 0.5 - vec3(worldRadius),
            vec3(0.0005)
        );
    vec3 worldPosition =
        aInstanceCenter +
        normalizedNearest * worldInnerExtent +
        roundedNormal * worldRadius;
    vec4 viewPosition = uView * vec4(worldPosition, 1.0);

    vWorldPosition = worldPosition;
    vViewPosition = viewPosition.xyz;
    vNormal = roundedNormal;
    vLocalPosition = aPosition;
    vColor = aInstanceColor;
    vMaterial = aInstanceMaterial;
    gl_Position = uProjection * viewPosition;
}
)glsl";

        constexpr const char *voxelFragmentShader = R"glsl(
#version 330 core
in vec3 vWorldPosition;
in vec3 vViewPosition;
in vec3 vNormal;
in vec3 vLocalPosition;
in vec3 vColor;
flat in vec4 vMaterial;

uniform float uSelectedLayer;
uniform float uHeat;

layout(location = 0) out vec4 sceneColor;
layout(location = 1) out uint pickColor;
layout(location = 2) out vec4 brightColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 viewDirection = normalize(-vViewPosition);
    vec3 keyDirection = normalize(vec3(-0.38, 0.84, 0.40));
    vec3 fillDirection = normalize(vec3(0.58, 0.30, -0.76));

    float wrappedKey = clamp(
        (dot(normal, keyDirection) + 0.42) / 1.42,
        0.0,
        1.0
    );

    float fill = max(dot(normal, fillDirection), 0.0);
    float hemisphere = normal.y * 0.5 + 0.5;
    float viewFacing = max(dot(normal, viewDirection), 0.0);

    vec3 halfVector = normalize(keyDirection + viewDirection);
    float specular = pow(
        max(dot(normal, halfVector), 0.0),
        18.0
    );

    float selected = abs(vMaterial.z - uSelectedLayer) < 0.45
        ? 1.04
        : 0.94;

    float faceLight =
        0.82 +
        wrappedKey * 0.10 +
        fill * 0.025 +
        hemisphere * 0.045;

    vec3 base = max(vColor * uHeat, vec3(0.0005));

    // Preserve the bright density core without hiding the voxel's square top
    // and side faces. Only the physically rounded corner receives a soft falloff.
    float surfaceRadius = length(vLocalPosition);
    float densityCore =
        1.0 -
        smoothstep(0.68, 0.94, surfaceRadius);
    float fresnelRim = pow(1.0 - viewFacing, 1.65);
    float softBody =
        mix(0.88, 1.04, densityCore) *
        mix(1.0, 0.92, fresnelRim);

    vec3 highlight =
        mix(base, vec3(1.0, 0.72, 0.38), 0.30) *
        specular *
        (0.025 + vMaterial.x * 0.075);

    vec3 lit =
        base *
        faceLight *
        selected *
        softBody +
        base * (0.025 + vMaterial.x * 0.075) +
        highlight;

    sceneColor = vec4(lit, 1.0);
    pickColor = uint(vMaterial.w + 0.5);

    float luminance = dot(
        lit,
        vec3(0.2126, 0.7152, 0.0722)
    );
    float bloomMask = smoothstep(
        0.12,
        0.48,
        luminance
    );

    float bloomEnergy =
        (0.32 + vMaterial.x * 0.68) *
        mix(0.82, 1.0, densityCore);

    brightColor = vec4(
        lit * bloomMask * bloomEnergy,
        1.0
    );
}
)glsl";

        constexpr const char *ghostVertexShader = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec3 aInstanceCenter;
layout(location = 3) in vec3 aInstanceSize;
layout(location = 4) in vec3 aInstanceColor;
layout(location = 5) in vec4 aInstanceMaterial;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vColor;
flat out vec4 vMaterial;

void main() {
    vec3 worldPosition =
        aInstanceCenter +
        aPosition * max(aInstanceSize, vec3(0.001));

    vColor = aInstanceColor;
    vMaterial = aInstanceMaterial;
    gl_Position =
        uProjection *
        uView *
        vec4(worldPosition, 1.0);
}
)glsl";

        constexpr const char *ghostFragmentShader = R"glsl(
#version 330 core
in vec3 vColor;
flat in vec4 vMaterial;

uniform float uSelectedLayer;

layout(location = 0) out vec4 sceneColor;
layout(location = 1) out uint pickColor;
layout(location = 2) out vec4 brightColor;

void main() {
    float selected =
        abs(vMaterial.z - uSelectedLayer) < 0.45
            ? 1.0
            : 0.0;

    float alpha =
        mix(0.18, 0.42, selected);

    vec3 color =
        vColor *
        mix(0.82, 1.45, selected);

    sceneColor = vec4(color, alpha);
    pickColor = uint(vMaterial.w + 0.5);
    brightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
)glsl";

        constexpr const char *gridVertexShader = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPosition;

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uGridCenter;
uniform vec2 uGridExtent;

out vec3 vWorldPosition;
out vec2 vNormalizedPosition;

void main() {
    vNormalizedPosition = aPosition;
    vWorldPosition = uGridCenter + vec3(
        aPosition.x * uGridExtent.x,
        0.0,
        aPosition.y * uGridExtent.y
    );
    gl_Position =
        uProjection *
        uView *
        vec4(vWorldPosition, 1.0);
}
)glsl";

        constexpr const char *gridFragmentShader = R"glsl(
#version 330 core
in vec3 vWorldPosition;
in vec2 vNormalizedPosition;

layout(location = 0) out vec4 sceneColor;
layout(location = 1) out uint pickColor;
layout(location = 2) out vec4 brightColor;

float gridLine(vec2 coordinate) {
    vec2 derivative = max(fwidth(coordinate), vec2(0.0001));
    vec2 distanceToLine =
        abs(fract(coordinate - 0.5) - 0.5) /
        derivative;
    return 1.0 - min(min(distanceToLine.x, distanceToLine.y), 1.0);
}

void main() {
    float minor = gridLine(vWorldPosition.xz / 0.72);
    float major = gridLine(vWorldPosition.xz / 3.60);
    float radial = length(vNormalizedPosition);
    float fade = 1.0 - smoothstep(0.36, 1.18, radial);

    vec3 floorColor = vec3(0.0022, 0.0065, 0.0145);
    vec3 minorColor = vec3(0.020, 0.072, 0.100);
    vec3 majorColor = vec3(0.038, 0.125, 0.160);
    vec3 color = floorColor;
    color += minorColor * minor * fade * 0.48;
    color += majorColor * major * fade * 0.56;

    sceneColor = vec4(color, 1.0);
    pickColor = 0U;
    brightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
)glsl";

        voxelShaderProgram_ =
                createProgram(
                    voxelVertexShader,
                    voxelFragmentShader,
                    "Density Volume instanced voxel shader"
                );

        ghostShaderProgram_ =
                createProgram(
                    ghostVertexShader,
                    ghostFragmentShader,
                    "Density Volume edge-ghost shader"
                );

        gridShaderProgram_ =
                createProgram(
                    gridVertexShader,
                    gridFragmentShader,
                    "Density Volume ground-grid shader"
                );

        voxelViewUniform_ =
                glGetUniformLocation(
                    voxelShaderProgram_,
                    "uView"
                );

        voxelProjectionUniform_ =
                glGetUniformLocation(
                    voxelShaderProgram_,
                    "uProjection"
                );

        voxelSelectedLayerUniform_ =
                glGetUniformLocation(
                    voxelShaderProgram_,
                    "uSelectedLayer"
                );

        voxelHeatUniform_ =
                glGetUniformLocation(
                    voxelShaderProgram_,
                    "uHeat"
                );

        ghostViewUniform_ =
                glGetUniformLocation(
                    ghostShaderProgram_,
                    "uView"
                );

        ghostProjectionUniform_ =
                glGetUniformLocation(
                    ghostShaderProgram_,
                    "uProjection"
                );

        ghostSelectedLayerUniform_ =
                glGetUniformLocation(
                    ghostShaderProgram_,
                    "uSelectedLayer"
                );

        gridViewUniform_ =
                glGetUniformLocation(
                    gridShaderProgram_,
                    "uView"
                );

        gridProjectionUniform_ =
                glGetUniformLocation(
                    gridShaderProgram_,
                    "uProjection"
                );

        gridCenterUniform_ =
                glGetUniformLocation(
                    gridShaderProgram_,
                    "uGridCenter"
                );

        gridExtentUniform_ =
                glGetUniformLocation(
                    gridShaderProgram_,
                    "uGridExtent"
                );

        requireUniform(voxelViewUniform_, "uView");
        requireUniform(voxelProjectionUniform_, "uProjection");
        requireUniform(
            voxelSelectedLayerUniform_,
            "uSelectedLayer"
        );
        requireUniform(voxelHeatUniform_, "uHeat");
        requireUniform(ghostViewUniform_, "ghost uView");
        requireUniform(
            ghostProjectionUniform_,
            "ghost uProjection"
        );
        requireUniform(
            ghostSelectedLayerUniform_,
            "ghost uSelectedLayer"
        );
        requireUniform(gridViewUniform_, "uView");
        requireUniform(gridProjectionUniform_, "uProjection");
        requireUniform(gridCenterUniform_, "uGridCenter");
        requireUniform(gridExtentUniform_, "uGridExtent");
    }

    void Renderer::createPostProcessPrograms() {
        constexpr const char *fullScreenVertexShader = R"glsl(
#version 330 core
out vec2 vUv;

void main() {
    const vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    const vec2 coordinates[3] = vec2[](
        vec2(0.0, 0.0),
        vec2(2.0, 0.0),
        vec2(0.0, 2.0)
    );

    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    vUv = coordinates[gl_VertexID];
}
)glsl";

        constexpr const char *blurFragmentShader = R"glsl(
#version 330 core
in vec2 vUv;
uniform sampler2D uInput;
uniform bool uHorizontal;
out vec4 fragmentColor;

void main() {
    const float weights[6] = float[](
        0.2096,
        0.1873,
        0.1328,
        0.0753,
        0.0341,
        0.0123
    );

    vec2 texel = 1.0 / vec2(textureSize(uInput, 0));
    vec3 result = texture(uInput, vUv).rgb * weights[0];

    for (int index = 1; index < 6; ++index) {
        vec2 offset = uHorizontal
            ? vec2(texel.x * float(index), 0.0)
            : vec2(0.0, texel.y * float(index));

        result += texture(uInput, vUv + offset).rgb * weights[index];
        result += texture(uInput, vUv - offset).rgb * weights[index];
    }

    fragmentColor = vec4(result, 1.0);
}
)glsl";

        constexpr const char *compositeFragmentShader = R"glsl(
#version 330 core
in vec2 vUv;
uniform sampler2D uScene;
uniform sampler2D uBloom;
out vec4 fragmentColor;

vec3 linearToSrgb(vec3 linearColor) {
    vec3 low =
        linearColor * 12.92;

    vec3 high =
        1.055 *
        pow(max(linearColor, vec3(0.0)), vec3(1.0 / 2.4)) -
        0.055;

    return mix(
        high,
        low,
        lessThanEqual(linearColor, vec3(0.0031308))
    );
}

void main() {
    vec3 scene = texture(uScene, vUv).rgb;
    vec3 bloom = texture(uBloom, vUv).rgb;
    vec3 linearColor =
        scene +
        bloom * 0.40;

    vec2 centered = vUv * 2.0 - 1.0;
    float vignette =
        1.0 -
        smoothstep(0.58, 1.52, dot(centered, centered));

    linearColor *=
        mix(0.90, 1.0, vignette);

    fragmentColor = vec4(
        linearToSrgb(linearColor),
        1.0
    );
}
)glsl";

        blurShaderProgram_ =
                createProgram(
                    fullScreenVertexShader,
                    blurFragmentShader,
                    "Density Volume bloom blur shader"
                );

        compositeShaderProgram_ =
                createProgram(
                    fullScreenVertexShader,
                    compositeFragmentShader,
                    "Density Volume HDR composite shader"
                );

        blurInputUniform_ =
                glGetUniformLocation(
                    blurShaderProgram_,
                    "uInput"
                );

        blurHorizontalUniform_ =
                glGetUniformLocation(
                    blurShaderProgram_,
                    "uHorizontal"
                );

        compositeSceneUniform_ =
                glGetUniformLocation(
                    compositeShaderProgram_,
                    "uScene"
                );

        compositeBloomUniform_ =
                glGetUniformLocation(
                    compositeShaderProgram_,
                    "uBloom"
                );

        requireUniform(blurInputUniform_, "uInput");
        requireUniform(blurHorizontalUniform_, "uHorizontal");
        requireUniform(compositeSceneUniform_, "uScene");
        requireUniform(compositeBloomUniform_, "uBloom");
    }

    void Renderer::createVoxelBuffers() {
        static_assert(
            std::is_standard_layout_v<VoxelInstance>
        );

        if (
            voxelGeometry_.indices.size() >
            static_cast<std::size_t>(
                std::numeric_limits<int>::max()
            )
        ) {
            throw std::runtime_error{
                "Density Volume rounded cube exceeds OpenGL index limits."
            };
        }

        cubeIndexCount_ =
                static_cast<int>(
                    voxelGeometry_.indices.size()
                );

        glGenVertexArrays(1, &voxelVertexArray_);
        glGenBuffers(1, &voxelVertexBuffer_);
        glGenBuffers(1, &voxelIndexBuffer_);
        glGenBuffers(1, &instanceBuffer_);
        glBindVertexArray(voxelVertexArray_);

        glBindBuffer(
            GL_ARRAY_BUFFER,
            voxelVertexBuffer_
        );

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                voxelGeometry_.vertices.size() *
                sizeof(VoxelVertex)
            ),
            voxelGeometry_.vertices.data(),
            GL_STATIC_DRAW
        );

        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            static_cast<int>(sizeof(VoxelVertex)),
            reinterpret_cast<const void *>(
                offsetof(VoxelVertex, position)
            )
        );

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            static_cast<int>(sizeof(VoxelVertex)),
            reinterpret_cast<const void *>(
                offsetof(VoxelVertex, normal)
            )
        );

        glEnableVertexAttribArray(1);

        glBindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            voxelIndexBuffer_
        );

        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                voxelGeometry_.indices.size() *
                sizeof(std::uint32_t)
            ),
            voxelGeometry_.indices.data(),
            GL_STATIC_DRAW
        );

        glBindBuffer(
            GL_ARRAY_BUFFER,
            instanceBuffer_
        );

        const auto defineInstanceAttribute =
                [](
                    const unsigned int location,
                    const int components,
                    const std::size_t offset
                ) {
                    glVertexAttribPointer(
                        location,
                        components,
                        GL_FLOAT,
                        GL_FALSE,
                        static_cast<int>(
                            sizeof(VoxelInstance)
                        ),
                        reinterpret_cast<const void *>(offset)
                    );

                    glEnableVertexAttribArray(location);
                    glVertexAttribDivisor(location, 1);
                };

        defineInstanceAttribute(
            2,
            3,
            offsetof(VoxelInstance, center)
        );

        defineInstanceAttribute(
            3,
            3,
            offsetof(VoxelInstance, size)
        );

        defineInstanceAttribute(
            4,
            3,
            offsetof(VoxelInstance, color)
        );

        defineInstanceAttribute(
            5,
            4,
            offsetof(VoxelInstance, emissive)
        );

        glBindVertexArray(0);
    }

    void Renderer::createGhostBuffers() {
        constexpr std::array<float, 24U> vertices{
            -0.5F, -0.5F, -0.5F,
             0.5F, -0.5F, -0.5F,
             0.5F, -0.5F,  0.5F,
            -0.5F, -0.5F,  0.5F,
            -0.5F,  0.5F, -0.5F,
             0.5F,  0.5F, -0.5F,
             0.5F,  0.5F,  0.5F,
            -0.5F,  0.5F,  0.5F
        };

        constexpr std::array<std::uint32_t, 24U> indices{
            0U, 1U,
            1U, 2U,
            2U, 3U,
            3U, 0U,
            4U, 5U,
            5U, 6U,
            6U, 7U,
            7U, 4U,
            0U, 4U,
            1U, 5U,
            2U, 6U,
            3U, 7U
        };

        glGenVertexArrays(1, &ghostVertexArray_);
        glGenBuffers(1, &ghostVertexBuffer_);
        glGenBuffers(1, &ghostIndexBuffer_);
        glGenBuffers(1, &ghostInstanceBuffer_);
        glBindVertexArray(ghostVertexArray_);

        glBindBuffer(
            GL_ARRAY_BUFFER,
            ghostVertexBuffer_
        );

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                vertices.size() * sizeof(float)
            ),
            vertices.data(),
            GL_STATIC_DRAW
        );

        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            3 * static_cast<int>(sizeof(float)),
            nullptr
        );

        glEnableVertexAttribArray(0);

        glBindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            ghostIndexBuffer_
        );

        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                indices.size() *
                sizeof(std::uint32_t)
            ),
            indices.data(),
            GL_STATIC_DRAW
        );

        glBindBuffer(
            GL_ARRAY_BUFFER,
            ghostInstanceBuffer_
        );

        const auto defineInstanceAttribute =
                [](
                    const unsigned int location,
                    const int components,
                    const std::size_t offset
                ) {
                    glVertexAttribPointer(
                        location,
                        components,
                        GL_FLOAT,
                        GL_FALSE,
                        static_cast<int>(
                            sizeof(VoxelInstance)
                        ),
                        reinterpret_cast<const void *>(offset)
                    );

                    glEnableVertexAttribArray(location);
                    glVertexAttribDivisor(location, 1);
                };

        defineInstanceAttribute(
            2,
            3,
            offsetof(VoxelInstance, center)
        );

        defineInstanceAttribute(
            3,
            3,
            offsetof(VoxelInstance, size)
        );

        defineInstanceAttribute(
            4,
            3,
            offsetof(VoxelInstance, color)
        );

        defineInstanceAttribute(
            5,
            4,
            offsetof(VoxelInstance, emissive)
        );

        glBindVertexArray(0);
    }

    void Renderer::createGridBuffers() {
        constexpr std::array<float, 8U> vertices{
            -1.0F, -1.0F,
             1.0F, -1.0F,
             1.0F,  1.0F,
            -1.0F,  1.0F
        };

        constexpr std::array<std::uint32_t, 6U> indices{
            0U, 1U, 2U,
            0U, 2U, 3U
        };

        glGenVertexArrays(1, &gridVertexArray_);
        glGenBuffers(1, &gridVertexBuffer_);
        glGenBuffers(1, &gridIndexBuffer_);
        glBindVertexArray(gridVertexArray_);

        glBindBuffer(
            GL_ARRAY_BUFFER,
            gridVertexBuffer_
        );

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                vertices.size() * sizeof(float)
            ),
            vertices.data(),
            GL_STATIC_DRAW
        );

        glVertexAttribPointer(
            0,
            2,
            GL_FLOAT,
            GL_FALSE,
            2 * static_cast<int>(sizeof(float)),
            nullptr
        );

        glEnableVertexAttribArray(0);

        glBindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            gridIndexBuffer_
        );

        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                indices.size() *
                sizeof(std::uint32_t)
            ),
            indices.data(),
            GL_STATIC_DRAW
        );

        glBindVertexArray(0);
    }

    void Renderer::createPostProcessVertexArray() {
        glGenVertexArrays(1, &postProcessVertexArray_);
    }

    void Renderer::uploadInstances() {
        if (
            scene_.voxels.size() >
                static_cast<std::size_t>(
                    std::numeric_limits<int>::max()
                ) ||
            scene_.ghostVoxels.size() >
            static_cast<std::size_t>(
                std::numeric_limits<int>::max()
            )
        ) {
            throw std::runtime_error{
                "Density Volume instance count exceeds OpenGL limits."
            };
        }

        const auto upload =
                [](
                    const unsigned int buffer,
                    const std::vector<VoxelInstance> &instances,
                    std::size_t &capacity
                ) {
                    glBindBuffer(
                        GL_ARRAY_BUFFER,
                        buffer
                    );

                    if (instances.size() > capacity) {
                        const std::size_t grownCapacity =
                                std::max(
                                    instances.size(),
                                    capacity +
                                    capacity / 2U +
                                    256U
                                );

                        glBufferData(
                            GL_ARRAY_BUFFER,
                            static_cast<GLsizeiptr>(
                                grownCapacity *
                                sizeof(VoxelInstance)
                            ),
                            nullptr,
                            GL_DYNAMIC_DRAW
                        );

                        capacity =
                                grownCapacity;
                    }

                    if (!instances.empty()) {
                        glBufferSubData(
                            GL_ARRAY_BUFFER,
                            0,
                            static_cast<GLsizeiptr>(
                                instances.size() *
                                sizeof(VoxelInstance)
                            ),
                            instances.data()
                        );
                    }
                };

        upload(
            instanceBuffer_,
            scene_.voxels,
            instanceCapacity_
        );

        upload(
            ghostInstanceBuffer_,
            scene_.ghostVoxels,
            ghostInstanceCapacity_
        );
    }

    void Renderer::resizeFramebuffer(
        const int width,
        const int height
    ) {
        int previousFramebuffer{};
        int previousTexture{};
        int previousRenderbuffer{};

        glGetIntegerv(
            GL_FRAMEBUFFER_BINDING,
            &previousFramebuffer
        );

        glGetIntegerv(
            GL_TEXTURE_BINDING_2D,
            &previousTexture
        );

        glGetIntegerv(
            GL_RENDERBUFFER_BINDING,
            &previousRenderbuffer
        );

        const int bloomWidth =
                std::max(width / 2, 1);

        const int bloomHeight =
                std::max(height / 2, 1);

        const auto allocateColorTexture =
                [](
                    const unsigned int texture,
                    const int internalFormat,
                    const unsigned int dataType,
                    const int textureWidth,
                    const int textureHeight
                ) {
                    glBindTexture(GL_TEXTURE_2D, texture);
                    glTexImage2D(
                        GL_TEXTURE_2D,
                        0,
                        internalFormat,
                        textureWidth,
                        textureHeight,
                        0,
                        GL_RGBA,
                        dataType,
                        nullptr
                    );

                    glTexParameteri(
                        GL_TEXTURE_2D,
                        GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR
                    );

                    glTexParameteri(
                        GL_TEXTURE_2D,
                        GL_TEXTURE_MAG_FILTER,
                        GL_LINEAR
                    );

                    glTexParameteri(
                        GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_S,
                        GL_CLAMP_TO_EDGE
                    );

                    glTexParameteri(
                        GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_T,
                        GL_CLAMP_TO_EDGE
                    );
                };

        allocateColorTexture(
            sceneColorTexture_,
            GL_RGBA16F,
            GL_FLOAT,
            width,
            height
        );

        allocateColorTexture(
            brightTexture_,
            GL_RGBA16F,
            GL_FLOAT,
            width,
            height
        );

        allocateColorTexture(
            blurTextures_[0],
            GL_RGBA16F,
            GL_FLOAT,
            bloomWidth,
            bloomHeight
        );

        allocateColorTexture(
            blurTextures_[1],
            GL_RGBA16F,
            GL_FLOAT,
            bloomWidth,
            bloomHeight
        );

        allocateColorTexture(
            colorTexture_,
            GL_RGBA8,
            GL_UNSIGNED_BYTE,
            width,
            height
        );

        glBindTexture(
            GL_TEXTURE_2D,
            pickTexture_
        );

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_R32UI,
            width,
            height,
            0,
            GL_RED_INTEGER,
            GL_UNSIGNED_INT,
            nullptr
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_NEAREST
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_NEAREST
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE
        );

        glBindRenderbuffer(
            GL_RENDERBUFFER,
            depthStencilBuffer_
        );

        glRenderbufferStorage(
            GL_RENDERBUFFER,
            GL_DEPTH24_STENCIL8,
            width,
            height
        );

        glBindFramebuffer(
            GL_FRAMEBUFFER,
            framebuffer_
        );

        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            sceneColorTexture_,
            0
        );

        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT1,
            GL_TEXTURE_2D,
            pickTexture_,
            0
        );

        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT2,
            GL_TEXTURE_2D,
            brightTexture_,
            0
        );

        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER,
            depthStencilBuffer_
        );

        constexpr std::array<unsigned int, 3U> drawBuffers{
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1,
            GL_COLOR_ATTACHMENT2
        };

        glDrawBuffers(
            static_cast<int>(drawBuffers.size()),
            drawBuffers.data()
        );

        bool complete =
                glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
                GL_FRAMEBUFFER_COMPLETE;

        for (std::size_t index = 0U; index < 2U; ++index) {
            glBindFramebuffer(
                GL_FRAMEBUFFER,
                blurFramebuffers_[index]
            );

            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                blurTextures_[index],
                0
            );

            glDrawBuffer(GL_COLOR_ATTACHMENT0);

            complete =
                    complete &&
                    glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
                    GL_FRAMEBUFFER_COMPLETE;
        }

        glBindFramebuffer(
            GL_FRAMEBUFFER,
            compositeFramebuffer_
        );

        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            colorTexture_,
            0
        );

        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        complete =
                complete &&
                glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
                GL_FRAMEBUFFER_COMPLETE;

        glBindFramebuffer(
            GL_FRAMEBUFFER,
            static_cast<unsigned int>(previousFramebuffer)
        );

        glBindRenderbuffer(
            GL_RENDERBUFFER,
            static_cast<unsigned int>(previousRenderbuffer)
        );

        glBindTexture(
            GL_TEXTURE_2D,
            static_cast<unsigned int>(previousTexture)
        );

        if (!complete) {
            throw std::runtime_error{
                "Density Volume scene framebuffer is incomplete."
            };
        }

        framebufferWidth_ = width;
        framebufferHeight_ = height;
    }

    void Renderer::renderPostProcess(
        const int width,
        const int height
    ) {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glBindVertexArray(postProcessVertexArray_);
        glUseProgram(blurShaderProgram_);
        glUniform1i(blurInputUniform_, 0);

        constexpr int blurPassCount = 4;

        const int bloomWidth =
                std::max(width / 2, 1);

        const int bloomHeight =
                std::max(height / 2, 1);

        for (int pass = 0; pass < blurPassCount; ++pass) {
            const std::size_t outputIndex =
                    static_cast<std::size_t>(pass % 2);

            const unsigned int inputTexture =
                    pass == 0
                        ? brightTexture_
                        : blurTextures_[1U - outputIndex];

            glBindFramebuffer(
                GL_FRAMEBUFFER,
                blurFramebuffers_[outputIndex]
            );

            glViewport(
                0,
                0,
                bloomWidth,
                bloomHeight
            );
            glUniform1i(
                blurHorizontalUniform_,
                pass % 2 == 0 ? 1 : 0
            );

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(
                GL_TEXTURE_2D,
                inputTexture
            );

            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        const unsigned int blurredTexture =
                blurTextures_[
                    static_cast<std::size_t>(
                        (blurPassCount - 1) % 2
                    )
                ];

        glBindFramebuffer(
            GL_FRAMEBUFFER,
            compositeFramebuffer_
        );

        glViewport(0, 0, width, height);
        glUseProgram(compositeShaderProgram_);
        glUniform1i(compositeSceneUniform_, 0);
        glUniform1i(compositeBloomUniform_, 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(
            GL_TEXTURE_2D,
            sceneColorTexture_
        );

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(
            GL_TEXTURE_2D,
            blurredTexture
        );

        glDrawArrays(GL_TRIANGLES, 0, 3);
        glActiveTexture(GL_TEXTURE0);
    }
}

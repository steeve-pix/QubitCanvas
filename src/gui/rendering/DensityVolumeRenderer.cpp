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
            sceneFingerprint_ = stack.fingerprint;
            sceneSelectedLayer_.reset();
            sceneMode_ = mode;
            sceneCenter_ = Vector3{};
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
            const std::size_t visibleCount =
                    scene_.layerEndInstanceCounts.at(
                        safeSelectedLayer
                    );

            if (
                visibleCount >
                static_cast<std::size_t>(
                    std::numeric_limits<int>::max()
                )
            ) {
                throw std::runtime_error{
                    "Density Volume instance count exceeds OpenGL limits."
                };
            }

            visibleInstanceCount_ =
                    static_cast<int>(visibleCount);

            Vector3 minimum{
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()
            };

            Vector3 maximum{
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest()
            };

            for (
                std::size_t index = 0U;
                index < visibleCount;
                ++index
            ) {
                const VoxelInstance &voxel =
                        scene_.voxels[index];

                const Vector3 halfSize =
                        voxel.size * 0.5F;

                minimum.x =
                        std::min(
                            minimum.x,
                            voxel.center.x - halfSize.x
                        );

                minimum.y =
                        std::min(
                            minimum.y,
                            voxel.center.y - halfSize.y
                        );

                minimum.z =
                        std::min(
                            minimum.z,
                            voxel.center.z - halfSize.z
                        );

                maximum.x =
                        std::max(
                            maximum.x,
                            voxel.center.x + halfSize.x
                        );

                maximum.y =
                        std::max(
                            maximum.y,
                            voxel.center.y + halfSize.y
                        );

                maximum.z =
                        std::max(
                            maximum.z,
                            voxel.center.z + halfSize.z
                        );
            }

            sceneCenter_ =
                    (minimum + maximum) * 0.5F;

            const float visibleLength =
                    maximum.x - minimum.x;

            const float visibleHeight =
                    maximum.y - minimum.y;

            const float visibleWidth =
                    maximum.z - minimum.z;

            // The viewport is deliberately wide. Treating a long history as
            // a sphere wastes most of that horizontal field of view and makes
            // its voxels unreadably small. This framing measure reserves the
            // full vertical matrix while fitting X against a typical 2:1+
            // panel aspect; clip planes still retain generous scene margins.
            sceneRadius_ =
                    std::max(
                        {
                            visibleLength / 3.15F,
                            visibleHeight * 0.88F,
                            visibleWidth * 0.82F,
                            1.0F
                        }
                    ) *
                    1.03F;

            scene_.groundCenter.x =
                    sceneCenter_.x;

            scene_.groundHalfExtentX =
                    std::max(
                        visibleLength * 0.68F +
                        visibleWidth * 0.34F,
                        visibleWidth * 0.78F
                    );
        } else {
            visibleInstanceCount_ =
                    static_cast<int>(scene_.voxels.size());
            sceneCenter_ = scene_.center;
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

            glBindVertexArray(voxelVertexArray_);
            glDrawElementsInstanced(
                GL_TRIANGLES,
                cubeIndexCount_,
                GL_UNSIGNED_INT,
                nullptr,
                visibleInstanceCount_
            );
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
        glDeleteProgram(voxelShaderProgram_);
        glDeleteProgram(gridShaderProgram_);
        glDeleteProgram(blurShaderProgram_);
        glDeleteProgram(compositeShaderProgram_);

        voxelVertexArray_ = 0U;
        voxelVertexBuffer_ = 0U;
        voxelIndexBuffer_ = 0U;
        instanceBuffer_ = 0U;
        gridVertexArray_ = 0U;
        gridVertexBuffer_ = 0U;
        gridIndexBuffer_ = 0U;
        voxelShaderProgram_ = 0U;
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
        instanceCapacity_ = 0U;
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
    vec3 worldPosition = aInstanceCenter + aPosition * safeSize;
    vec4 viewPosition = uView * vec4(worldPosition, 1.0);

    vWorldPosition = worldPosition;
    vViewPosition = viewPosition.xyz;
    vNormal = normalize(aNormal / safeSize);
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

layout(location = 0) out vec4 sceneColor;
layout(location = 1) out uint pickColor;
layout(location = 2) out vec4 brightColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 viewDirection = normalize(-vViewPosition);
    vec3 keyDirection = normalize(vec3(-0.42, 0.78, 0.46));
    vec3 fillDirection = normalize(vec3(0.62, 0.26, -0.74));

    float keyDiffuse = max(dot(normal, keyDirection), 0.0);
    float wrappedKey = clamp(
        (dot(normal, keyDirection) + 0.34) / 1.34,
        0.0,
        1.0
    );
    float fillDiffuse = max(dot(normal, fillDirection), 0.0);
    float hemisphere = normal.y * 0.5 + 0.5;

    vec3 halfVector = normalize(keyDirection + viewDirection);
    float specular = pow(
        max(dot(normal, halfVector), 0.0),
        28.0
    );
    float fresnel = pow(
        1.0 - max(dot(normal, viewDirection), 0.0),
        3.0
    );

    float lowerFace = smoothstep(-0.50, -0.12, vLocalPosition.y);
    float sideContact = smoothstep(
        0.46,
        0.18,
        max(abs(vLocalPosition.x), abs(vLocalPosition.z))
    );
    float contactOcclusion = mix(0.66, 1.0, lowerFace);
    contactOcclusion *= mix(0.88, 1.0, sideContact);

    float selected = abs(vMaterial.z - uSelectedLayer) < 0.45
        ? 1.08
        : 0.94;

    vec3 base = max(vColor, vec3(0.008, 0.006, 0.018));
    vec3 ambient = base * (
        vec3(0.080, 0.100, 0.155) +
        vec3(0.095, 0.072, 0.038) * hemisphere
    );
    vec3 diffuse = base * (
        wrappedKey * vec3(1.08, 0.76, 0.40) +
        keyDiffuse * vec3(0.36, 0.24, 0.11) +
        fillDiffuse * vec3(0.075, 0.105, 0.180)
    );
    vec3 highlights =
        specular * vec3(1.00, 0.84, 0.56) * 0.62 +
        fresnel * base * 0.22;

    float emission = vMaterial.x;
    vec3 emissive =
        base * emission * 1.42 +
        vec3(1.00, 0.34, 0.035) * emission * emission * 0.12;

    vec3 lit =
        ((ambient + diffuse) * contactOcclusion + highlights) *
        selected +
        emissive;

    float distanceFade = clamp(
        exp(-length(vViewPosition) * 0.0018),
        0.74,
        1.0
    );
    vec3 fog = vec3(0.002, 0.006, 0.015);
    lit = mix(fog, lit, distanceFade);

    sceneColor = vec4(lit, 1.0);
    pickColor = uint(vMaterial.w + 0.5);

    float luminance = dot(
        lit,
        vec3(0.2126, 0.7152, 0.0722)
    );
    float bloomMask = smoothstep(
        0.64,
        1.48,
        luminance + emission * 0.42
    );
    brightColor = vec4(
        lit * bloomMask * (0.60 + emission * 0.42),
        1.0
    );
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

vec3 acesToneMap(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp(
        (color * (a * color + b)) /
        (color * (c * color + d) + e),
        0.0,
        1.0
    );
}

void main() {
    vec3 scene = texture(uScene, vUv).rgb;
    vec3 bloom = texture(uBloom, vUv).rgb;
    vec3 hdr = scene + bloom * 0.92;

    vec3 mapped = acesToneMap(hdr * 1.08);
    mapped = pow(mapped, vec3(1.0 / 2.2));

    float horizon = smoothstep(0.0, 0.88, 1.0 - vUv.y);
    mapped += vec3(0.002, 0.007, 0.014) * horizon;

    vec2 centered = vUv * 2.0 - 1.0;
    float vignette =
        1.0 -
        smoothstep(0.52, 1.48, dot(centered, centered));
    mapped *= mix(0.84, 1.0, vignette);

    fragmentColor = vec4(mapped, 1.0);
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
            )
        ) {
            throw std::runtime_error{
                "Density Volume instance count exceeds OpenGL limits."
            };
        }

        glBindBuffer(
            GL_ARRAY_BUFFER,
            instanceBuffer_
        );

        if (scene_.voxels.size() > instanceCapacity_) {
            const std::size_t grownCapacity =
                    std::max(
                        scene_.voxels.size(),
                        instanceCapacity_ +
                        instanceCapacity_ / 2U +
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

            instanceCapacity_ =
                    grownCapacity;
        }

        if (!scene_.voxels.empty()) {
            glBufferSubData(
                GL_ARRAY_BUFFER,
                0,
                static_cast<GLsizeiptr>(
                    scene_.voxels.size() *
                    sizeof(VoxelInstance)
                ),
                scene_.voxels.data()
            );
        }
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

        const auto allocateColorTexture =
                [width, height](
                    const unsigned int texture,
                    const int internalFormat,
                    const unsigned int dataType
                ) {
                    glBindTexture(GL_TEXTURE_2D, texture);
                    glTexImage2D(
                        GL_TEXTURE_2D,
                        0,
                        internalFormat,
                        width,
                        height,
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
            GL_FLOAT
        );

        allocateColorTexture(
            brightTexture_,
            GL_RGBA16F,
            GL_FLOAT
        );

        allocateColorTexture(
            blurTextures_[0],
            GL_RGBA16F,
            GL_FLOAT
        );

        allocateColorTexture(
            blurTextures_[1],
            GL_RGBA16F,
            GL_FLOAT
        );

        allocateColorTexture(
            colorTexture_,
            GL_RGBA8,
            GL_UNSIGNED_BYTE
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

        constexpr int blurPassCount = 6;

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

            glViewport(0, 0, width, height);
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

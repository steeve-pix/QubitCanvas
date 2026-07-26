#include "quantum_sim/gui/rendering/DensityVolumeRenderer.hpp"

#include "quantum_sim/gui/rendering/DensityVolumeLayout.hpp"

#include <glad/gl.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>

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

            glGetShaderInfoLog(shader, logLength, nullptr, log.data());
            glDeleteShader(shader);
            throw std::runtime_error{
                std::string{"Failed to compile "} + label + ": " + log
            };
        }
    }

    void Renderer::initialize() {
        if (initialized_) {
            return;
        }

        try {
            createShaderProgram();
            createMeshBuffers();

            glGenFramebuffers(1, &framebuffer_);
            glGenTextures(1, &colorTexture_);
            glGenTextures(1, &pickTexture_);
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
            throw std::runtime_error{"Density Volume renderer is not initialized."};
        }

        if (
            sceneFingerprint_ == stack.fingerprint &&
            sceneVisibleThroughLayer_.has_value() &&
            sceneVisibleThroughLayer_.value() == selectedLayer &&
            sceneMode_.has_value() &&
            sceneMode_.value() == mode &&
            indexCount_ > 0U
        ) {
            return false;
        }

        const SceneLayout layout =
                LayerStackLayout::build(
                    stack,
                    selectedLayer,
                    mode
                );

        const Mesh mesh =
                MeshBuilder::build(layout);

        uploadMesh(mesh);
        pickRecords_ = mesh.pickRecords;
        sceneCenter_ = layout.center;
        sceneRadius_ = layout.radius;
        sceneFingerprint_ = stack.fingerprint;
        sceneVisibleThroughLayer_ = selectedLayer;
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
            throw std::runtime_error{"Density Volume renderer is not initialized."};
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
        int previousProgram{};
        int previousVertexArray{};
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
        glGetIntegerv(GL_VIEWPORT, previousViewport);
        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);

        const bool depthTestWasEnabled =
                glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;

        const bool cullFaceWasEnabled =
                glIsEnabled(GL_CULL_FACE) == GL_TRUE;

        const bool blendingWasEnabled =
                glIsEnabled(GL_BLEND) == GL_TRUE;

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
        glViewport(0, 0, safeWidth, safeHeight);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glDisable(GL_BLEND);

        constexpr float clearColor[4]{
            0.006F,
            0.010F,
            0.020F,
            1.0F
        };

        constexpr unsigned int clearPick[4]{0U, 0U, 0U, 0U};

        glClearBufferfv(GL_COLOR, 0, clearColor);
        glClearBufferuiv(GL_COLOR, 1, clearPick);
        glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0F, 0);

        const Matrix4 model =
                identityMatrix();

        const Matrix4 view =
                camera.viewMatrix();

        const Matrix4 projection =
                camera.projectionMatrix(safeWidth, safeHeight);

        glUseProgram(shaderProgram_);
        glUniformMatrix4fv(modelUniform_, 1, GL_FALSE, model.data());
        glUniformMatrix4fv(viewUniform_, 1, GL_FALSE, view.data());
        glUniformMatrix4fv(projectionUniform_, 1, GL_FALSE, projection.data());
        glUniform1i(
            selectedLayerUniform_,
            static_cast<int>(
                std::min<std::size_t>(
                    selectedLayer,
                    static_cast<std::size_t>(std::numeric_limits<int>::max())
                )
            )
        );

        glBindVertexArray(vertexArray_);

        if (indexCount_ > 0U) {
            glDrawElements(
                GL_TRIANGLES,
                static_cast<int>(indexCount_),
                GL_UNSIGNED_INT,
                nullptr
            );
        }

        glBindVertexArray(static_cast<unsigned int>(previousVertexArray));
        glUseProgram(static_cast<unsigned int>(previousProgram));

        if (!depthTestWasEnabled) {
            glDisable(GL_DEPTH_TEST);
        }

        if (!cullFaceWasEnabled) {
            glDisable(GL_CULL_FACE);
        }

        if (blendingWasEnabled) {
            glEnable(GL_BLEND);
        }

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
        int previousReadBuffer{};
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
        glGetIntegerv(GL_READ_BUFFER, &previousReadBuffer);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_);
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
            static_cast<unsigned int>(previousReadFramebuffer)
        );
        glReadBuffer(static_cast<unsigned int>(previousReadBuffer));

        if (
            pickId == 0U ||
            pickId > pickRecords_.size()
        ) {
            return std::nullopt;
        }

        return pickRecords_[pickId - 1U];
    }

    void Renderer::shutdown() noexcept {
        if (depthStencilBuffer_ != 0U) {
            glDeleteRenderbuffers(1, &depthStencilBuffer_);
            depthStencilBuffer_ = 0U;
        }

        if (pickTexture_ != 0U) {
            glDeleteTextures(1, &pickTexture_);
            pickTexture_ = 0U;
        }

        if (colorTexture_ != 0U) {
            glDeleteTextures(1, &colorTexture_);
            colorTexture_ = 0U;
        }

        if (framebuffer_ != 0U) {
            glDeleteFramebuffers(1, &framebuffer_);
            framebuffer_ = 0U;
        }

        if (indexBuffer_ != 0U) {
            glDeleteBuffers(1, &indexBuffer_);
            indexBuffer_ = 0U;
        }

        if (vertexBuffer_ != 0U) {
            glDeleteBuffers(1, &vertexBuffer_);
            vertexBuffer_ = 0U;
        }

        if (vertexArray_ != 0U) {
            glDeleteVertexArrays(1, &vertexArray_);
            vertexArray_ = 0U;
        }

        if (shaderProgram_ != 0U) {
            glDeleteProgram(shaderProgram_);
            shaderProgram_ = 0U;
        }

        framebufferWidth_ = 0;
        framebufferHeight_ = 0;
        modelUniform_ = -1;
        viewUniform_ = -1;
        projectionUniform_ = -1;
        selectedLayerUniform_ = -1;
        indexCount_ = 0U;
        sceneFingerprint_ = 0U;
        sceneVisibleThroughLayer_.reset();
        sceneMode_.reset();
        sceneCenter_ = Vector3{};
        sceneRadius_ = 1.0F;
        pickRecords_.clear();
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

    void Renderer::createShaderProgram() {
        constexpr const char *vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in float aLayer;
layout (location = 4) in float aPickId;
layout (location = 5) in float aMagnitudeVoxel;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vNormal;
out vec3 vColor;
flat out float vLayer;
flat out float vPickId;
flat out float vMagnitudeVoxel;

void main() {
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vNormal = normalize(mat3(uModel) * aNormal);
    vColor = aColor;
    vLayer = aLayer;
    vPickId = aPickId;
    vMagnitudeVoxel = aMagnitudeVoxel;
    gl_Position = uProjection * uView * worldPosition;
}
)glsl";

        constexpr const char *fragmentShaderSource = R"glsl(
#version 330 core
in vec3 vNormal;
in vec3 vColor;
flat in float vLayer;
flat in float vPickId;
flat in float vMagnitudeVoxel;

uniform int uSelectedLayer;

layout (location = 0) out vec4 fragmentColor;
layout (location = 1) out uint pickOutput;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDirection = normalize(vec3(-0.55, 0.90, 0.62));
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float topFace = max(normal.y, 0.0);
    float sideContrast = 0.68 + 0.32 * max(normal.z, 0.0);
    float selected = abs(vLayer - float(uSelectedLayer)) < 0.25 ? 1.0 : 0.0;
    float selectedValue = selected * vMagnitudeVoxel;
    float emissive = pow(max(max(vColor.r, vColor.g), vColor.b), 2.0);

    vec3 color = vColor * (0.30 + 0.70 * diffuse) * sideContrast;
    color += vColor * topFace * 0.18;
    color += vColor * emissive * 0.35 * vMagnitudeVoxel;
    color = mix(
        color,
        color * 1.10 + vec3(0.10, 0.045, 0.008),
        selectedValue
    );

    fragmentColor = vec4(color, 1.0);
    pickOutput = uint(vPickId + 0.5);
}
)glsl";

        const unsigned int vertexShader =
                compileShader(
                    GL_VERTEX_SHADER,
                    vertexShaderSource,
                    "Density Volume vertex shader"
                );

        unsigned int fragmentShader{};

        try {
            fragmentShader =
                    compileShader(
                        GL_FRAGMENT_SHADER,
                        fragmentShaderSource,
                        "Density Volume fragment shader"
                    );

            shaderProgram_ =
                    glCreateProgram();

            glAttachShader(shaderProgram_, vertexShader);
            glAttachShader(shaderProgram_, fragmentShader);
            glLinkProgram(shaderProgram_);

            int linked{};
            glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &linked);

            if (linked != GL_TRUE) {
                int logLength{};
                glGetProgramiv(shaderProgram_, GL_INFO_LOG_LENGTH, &logLength);

                std::string log(
                    static_cast<std::size_t>(std::max(logLength, 1)),
                    '\0'
                );

                glGetProgramInfoLog(
                    shaderProgram_,
                    logLength,
                    nullptr,
                    log.data()
                );

                throw std::runtime_error{
                    std::string{"Failed to link Density Volume shader program: "} + log
                };
            }
        } catch (...) {
            glDeleteShader(vertexShader);

            if (fragmentShader != 0U) {
                glDeleteShader(fragmentShader);
            }

            throw;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        modelUniform_ =
                glGetUniformLocation(shaderProgram_, "uModel");

        viewUniform_ =
                glGetUniformLocation(shaderProgram_, "uView");

        projectionUniform_ =
                glGetUniformLocation(shaderProgram_, "uProjection");

        selectedLayerUniform_ =
                glGetUniformLocation(shaderProgram_, "uSelectedLayer");

        if (
            modelUniform_ < 0 ||
            viewUniform_ < 0 ||
            projectionUniform_ < 0 ||
            selectedLayerUniform_ < 0
        ) {
            throw std::runtime_error{"Density Volume shader uniforms are unavailable."};
        }
    }

    void Renderer::createMeshBuffers() {
        glGenVertexArrays(1, &vertexArray_);
        glGenBuffers(1, &vertexBuffer_);
        glGenBuffers(1, &indexBuffer_);

        glBindVertexArray(vertexArray_);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);

        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            static_cast<int>(sizeof(MeshVertex)),
            reinterpret_cast<const void *>(offsetof(MeshVertex, position))
        );
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            static_cast<int>(sizeof(MeshVertex)),
            reinterpret_cast<const void *>(offsetof(MeshVertex, normal))
        );
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(
            2,
            3,
            GL_FLOAT,
            GL_FALSE,
            static_cast<int>(sizeof(MeshVertex)),
            reinterpret_cast<const void *>(offsetof(MeshVertex, color))
        );
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(
            3,
            1,
            GL_FLOAT,
            GL_FALSE,
            static_cast<int>(sizeof(MeshVertex)),
            reinterpret_cast<const void *>(offsetof(MeshVertex, layer))
        );
        glEnableVertexAttribArray(3);

        glVertexAttribPointer(
            4,
            1,
            GL_FLOAT,
            GL_FALSE,
            static_cast<int>(sizeof(MeshVertex)),
            reinterpret_cast<const void *>(offsetof(MeshVertex, pickId))
        );
        glEnableVertexAttribArray(4);

        glVertexAttribPointer(
            5,
            1,
            GL_FLOAT,
            GL_FALSE,
            static_cast<int>(sizeof(MeshVertex)),
            reinterpret_cast<const void *>(
                offsetof(MeshVertex, magnitudeVoxel)
            )
        );
        glEnableVertexAttribArray(5);

        glBindVertexArray(0);
    }

    void Renderer::uploadMesh(const Mesh &mesh) {
        if (mesh.indices.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw std::runtime_error{"Density Volume mesh exceeds OpenGL draw-index limits."};
        }

        glBindVertexArray(vertexArray_);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                mesh.vertices.size() * sizeof(MeshVertex)
            ),
            mesh.vertices.data(),
            GL_STATIC_DRAW
        );

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                mesh.indices.size() * sizeof(std::uint32_t)
            ),
            mesh.indices.data(),
            GL_STATIC_DRAW
        );

        glBindVertexArray(0);
        indexCount_ = mesh.indices.size();
    }

    void Renderer::resizeFramebuffer(
        const int width,
        const int height
    ) {
        int previousFramebuffer{};
        int previousTexture{};
        int previousRenderbuffer{};
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &previousRenderbuffer);

        glBindTexture(GL_TEXTURE_2D, colorTexture_);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            nullptr
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, pickTexture_);
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
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindRenderbuffer(GL_RENDERBUFFER, depthStencilBuffer_);
        glRenderbufferStorage(
            GL_RENDERBUFFER,
            GL_DEPTH24_STENCIL8,
            width,
            height
        );

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            colorTexture_,
            0
        );

        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT1,
            GL_TEXTURE_2D,
            pickTexture_,
            0
        );

        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER,
            depthStencilBuffer_
        );

        constexpr unsigned int drawBuffers[2]{
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1
        };

        glDrawBuffers(2, drawBuffers);

        const unsigned int framebufferStatus =
                glCheckFramebufferStatus(GL_FRAMEBUFFER);

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

        if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error{"Density Volume framebuffer is incomplete."};
        }

        framebufferWidth_ = width;
        framebufferHeight_ = height;
    }
}

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

        const bool historyChanged =
                sceneFingerprint_ != stack.fingerprint ||
                !sceneMode_.has_value() ||
                sceneMode_.value() != mode;

        if (stack.layers.empty()) {
            const bool sceneChanged =
                    historyChanged ||
                    sceneVisibleThroughLayer_.has_value() ||
                    indexCount_ > 0U;

            if (!sceneChanged) {
                return false;
            }

            sceneMesh_ = Mesh{};
            uploadMesh(sceneMesh_);
            layerIndexCounts_.clear();
            layerSceneCenters_.clear();
            layerSceneRadii_.clear();
            builtThroughLayer_.reset();
            sceneFingerprint_ = stack.fingerprint;
            sceneVisibleThroughLayer_.reset();
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

        if (mode == VisualizationMode::FloorField) {
            if (
                !historyChanged &&
                sceneVisibleThroughLayer_.has_value() &&
                sceneVisibleThroughLayer_.value() == safeSelectedLayer &&
                indexCount_ > 0U
            ) {
                return false;
            }

            const SceneLayout layout =
                    LayerStackLayout::buildLayer(
                        stack.layers[safeSelectedLayer],
                        mode
                    );

            sceneMesh_ =
                    MeshBuilder::build(layout);

            uploadMesh(sceneMesh_);
            layerIndexCounts_.clear();
            layerSceneCenters_.clear();
            layerSceneRadii_.clear();
            builtThroughLayer_.reset();
            sceneCenter_ = layout.center;
            sceneRadius_ = layout.radius;
            sceneFingerprint_ = stack.fingerprint;
            sceneVisibleThroughLayer_ = safeSelectedLayer;
            sceneMode_ = mode;
            return true;
        }

        if (historyChanged) {
            std::size_t maximumVoxelCount{};

            for (const DensityLayer &layer : stack.layers) {
                if (
                    layer.cells.size() >
                    (
                        std::numeric_limits<std::size_t>::max() -
                        maximumVoxelCount
                    ) / 2U
                ) {
                    throw std::runtime_error{
                        "Density Volume layer stack exceeds mesh capacity limits."
                    };
                }

                maximumVoxelCount +=
                        layer.cells.size() * 2U;
            }

            if (
                maximumVoxelCount >
                std::numeric_limits<std::size_t>::max() /
                MeshBuilder::verticesPerVoxel ||
                maximumVoxelCount >
                std::numeric_limits<std::size_t>::max() /
                MeshBuilder::indicesPerBeveledVoxel
            ) {
                throw std::runtime_error{
                    "Density Volume layer stack exceeds mesh capacity limits."
                };
            }

            const std::size_t maximumVertexCount =
                    maximumVoxelCount *
                    MeshBuilder::verticesPerVoxel;

            const std::size_t maximumIndexCount =
                    maximumVoxelCount *
                    MeshBuilder::indicesPerBeveledVoxel;

            if (
                maximumIndexCount >
                static_cast<std::size_t>(
                    std::numeric_limits<int>::max()
                )
            ) {
                throw std::runtime_error{
                    "Density Volume mesh exceeds OpenGL draw-index limits."
                };
            }

            sceneMesh_ = Mesh{};
            sceneMesh_.vertices.reserve(maximumVertexCount);
            sceneMesh_.indices.reserve(maximumIndexCount);
            sceneMesh_.pickRecords.reserve(maximumVoxelCount);

            allocateMeshStorage(
                maximumVertexCount,
                maximumIndexCount
            );

            layerIndexCounts_.assign(stack.layers.size(), 0U);
            layerSceneCenters_.assign(
                stack.layers.size(),
                Vector3{}
            );
            layerSceneRadii_.assign(stack.layers.size(), 1.0F);
            builtThroughLayer_.reset();
            sceneFingerprint_ = stack.fingerprint;
            sceneVisibleThroughLayer_.reset();
            sceneMode_ = mode;
        } else if (
            sceneVisibleThroughLayer_.has_value() &&
            sceneVisibleThroughLayer_.value() == safeSelectedLayer &&
            indexCount_ > 0U
        ) {
            return false;
        }

        if (
            !builtThroughLayer_.has_value() ||
            safeSelectedLayer > builtThroughLayer_.value()
        ) {
            const std::size_t firstNewLayer =
                    builtThroughLayer_.has_value()
                        ? builtThroughLayer_.value() + 1U
                        : 0U;

            const std::size_t firstNewVertex =
                    sceneMesh_.vertices.size();

            const std::size_t firstNewIndex =
                    sceneMesh_.indices.size();

            for (
                std::size_t layerIndex = firstNewLayer;
                layerIndex <= safeSelectedLayer;
                ++layerIndex
            ) {
                const SceneLayout layerLayout =
                        LayerStackLayout::buildLayer(
                            stack.layers[layerIndex],
                            VisualizationMode::LayerStack
                        );

                MeshBuilder::append(sceneMesh_, layerLayout);
                layerIndexCounts_[layerIndex] =
                        sceneMesh_.indices.size();
                layerSceneCenters_[layerIndex] =
                        layerLayout.center;
                layerSceneRadii_[layerIndex] =
                        layerLayout.radius;
            }

            uploadMeshSuffix(
                sceneMesh_,
                firstNewVertex,
                firstNewIndex
            );

            builtThroughLayer_ =
                    safeSelectedLayer;
        }

        indexCount_ =
                layerIndexCounts_[safeSelectedLayer];

        sceneCenter_ =
                layerSceneCenters_[safeSelectedLayer];

        sceneRadius_ =
                layerSceneRadii_[safeSelectedLayer];

        sceneVisibleThroughLayer_ =
                safeSelectedLayer;

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
            pickId > sceneMesh_.pickRecords.size()
        ) {
            return std::nullopt;
        }

        return sceneMesh_.pickRecords[pickId - 1U];
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
        sceneMesh_ = Mesh{};
        layerIndexCounts_.clear();
        layerSceneCenters_.clear();
        layerSceneRadii_.clear();
        builtThroughLayer_.reset();
        vertexCapacity_ = 0U;
        indexCapacity_ = 0U;
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

out vec3 vViewPosition;
out vec3 vViewNormal;
out vec3 vColor;
flat out float vLayer;
flat out float vPickId;
flat out float vMagnitudeVoxel;

void main() {
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vec4 viewPosition = uView * worldPosition;
    vViewPosition = viewPosition.xyz;
    vViewNormal = normalize(mat3(uView * uModel) * aNormal);
    vColor = aColor;
    vLayer = aLayer;
    vPickId = aPickId;
    vMagnitudeVoxel = aMagnitudeVoxel;
    gl_Position = uProjection * viewPosition;
}
)glsl";

        constexpr const char *fragmentShaderSource = R"glsl(
#version 330 core
in vec3 vViewPosition;
in vec3 vViewNormal;
in vec3 vColor;
flat in float vLayer;
flat in float vPickId;
flat in float vMagnitudeVoxel;

uniform int uSelectedLayer;

layout (location = 0) out vec4 fragmentColor;
layout (location = 1) out uint pickOutput;

void main() {
    vec3 normal = normalize(vViewNormal);
    vec3 viewDirection = normalize(-vViewPosition);
    vec3 keyDirection = normalize(vec3(-0.48, 0.76, 0.54));
    vec3 fillDirection = normalize(vec3(0.72, 0.28, 0.46));
    vec3 halfDirection = normalize(keyDirection + viewDirection);

    float keyDiffuse = max(dot(normal, keyDirection), 0.0);
    float wrappedDiffuse = clamp(
        (dot(normal, keyDirection) + 0.22) / 1.22,
        0.0,
        1.0
    );
    float fillDiffuse = max(dot(normal, fillDirection), 0.0);
    float hemisphere = normal.y * 0.5 + 0.5;
    float specular = pow(max(dot(normal, halfDirection), 0.0), 34.0);
    float rim = pow(
        1.0 - max(dot(normal, viewDirection), 0.0),
        3.0
    );

    float selected = abs(vLayer - float(uSelectedLayer)) < 0.25 ? 1.0 : 0.0;
    float selectedValue = selected * vMagnitudeVoxel;
    float emissive = pow(max(max(vColor.r, vColor.g), vColor.b), 2.0);

    float ambient = mix(0.16, 0.30, hemisphere);
    vec3 color = vColor * (
        ambient +
        0.58 * wrappedDiffuse +
        0.24 * keyDiffuse +
        0.12 * fillDiffuse
    );

    color += mix(
        vec3(0.014, 0.020, 0.040),
        vColor,
        0.42
    ) * rim * (0.10 + 0.16 * vMagnitudeVoxel);

    color += vec3(1.00, 0.82, 0.58) *
             specular *
             (0.08 + 0.30 * vMagnitudeVoxel);

    color += vColor * emissive * 0.24 * vMagnitudeVoxel;
    color = mix(
        color,
        color * 1.08 + vec3(0.085, 0.040, 0.010),
        selectedValue
    );
    color = pow(max(color, vec3(0.0)), vec3(0.92));

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
            GL_DYNAMIC_DRAW
        );

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                mesh.indices.size() * sizeof(std::uint32_t)
            ),
            mesh.indices.data(),
            GL_DYNAMIC_DRAW
        );

        glBindVertexArray(0);
        vertexCapacity_ = mesh.vertices.size();
        indexCapacity_ = mesh.indices.size();
        indexCount_ = mesh.indices.size();
    }

    void Renderer::allocateMeshStorage(
        const std::size_t vertexCapacity,
        const std::size_t indexCapacity
    ) {
        glBindVertexArray(vertexArray_);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                vertexCapacity * sizeof(MeshVertex)
            ),
            nullptr,
            GL_DYNAMIC_DRAW
        );

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                indexCapacity * sizeof(std::uint32_t)
            ),
            nullptr,
            GL_DYNAMIC_DRAW
        );

        glBindVertexArray(0);
        vertexCapacity_ = vertexCapacity;
        indexCapacity_ = indexCapacity;
        indexCount_ = 0U;
    }

    void Renderer::uploadMeshSuffix(
        const Mesh &mesh,
        const std::size_t firstVertex,
        const std::size_t firstIndex
    ) {
        if (
            firstVertex > mesh.vertices.size() ||
            firstIndex > mesh.indices.size() ||
            mesh.vertices.size() > vertexCapacity_ ||
            mesh.indices.size() > indexCapacity_
        ) {
            throw std::runtime_error{
                "Density Volume incremental mesh exceeds allocated GPU storage."
            };
        }

        glBindVertexArray(vertexArray_);

        if (firstVertex < mesh.vertices.size()) {
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
            glBufferSubData(
                GL_ARRAY_BUFFER,
                static_cast<GLintptr>(
                    firstVertex * sizeof(MeshVertex)
                ),
                static_cast<GLsizeiptr>(
                    (
                        mesh.vertices.size() -
                        firstVertex
                    ) * sizeof(MeshVertex)
                ),
                mesh.vertices.data() + firstVertex
            );
        }

        if (firstIndex < mesh.indices.size()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
            glBufferSubData(
                GL_ELEMENT_ARRAY_BUFFER,
                static_cast<GLintptr>(
                    firstIndex * sizeof(std::uint32_t)
                ),
                static_cast<GLsizeiptr>(
                    (
                        mesh.indices.size() -
                        firstIndex
                    ) * sizeof(std::uint32_t)
                ),
                mesh.indices.data() + firstIndex
            );
        }

        glBindVertexArray(0);
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

#include "quantum_sim/gui/rendering/QaveCubeRenderer.hpp"

#include <glad/gl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace quantum_sim::gui {
    namespace {
        constexpr float pi = 3.14159265358979323846F;

        struct Vector3 {
            float x{};
            float y{};
            float z{};
        };

        using Matrix4 = std::array<float, 16>;

        [[nodiscard]] Vector3 subtract(const Vector3 &left, const Vector3 &right) {
            return Vector3{
                left.x - right.x,
                left.y - right.y,
                left.z - right.z
            };
        }

        [[nodiscard]] float dot(const Vector3 &left, const Vector3 &right) {
            return left.x * right.x + left.y * right.y + left.z * right.z;
        }

        [[nodiscard]] Vector3 cross(const Vector3 &left, const Vector3 &right) {
            return Vector3{
                left.y * right.z - left.z * right.y,
                left.z * right.x - left.x * right.z,
                left.x * right.y - left.y * right.x
            };
        }

        [[nodiscard]] Vector3 normalize(const Vector3 &value) {
            const float length =
                    std::sqrt(dot(value, value));

            if (length <= 0.000001F) {
                throw std::runtime_error{"Cannot normalize a zero-length camera vector."};
            }

            return Vector3{
                value.x / length,
                value.y / length,
                value.z / length
            };
        }

        [[nodiscard]] Matrix4 identityMatrix() {
            return Matrix4{
                1.0F, 0.0F, 0.0F, 0.0F,
                0.0F, 1.0F, 0.0F, 0.0F,
                0.0F, 0.0F, 1.0F, 0.0F,
                0.0F, 0.0F, 0.0F, 1.0F
            };
        }

        [[nodiscard]] Matrix4 multiply(const Matrix4 &left, const Matrix4 &right) {
            Matrix4 result{};

            // Matrices are column-major because glUniformMatrix4fv receives
            // them without transposition.
            for (int column = 0; column < 4; ++column) {
                for (int row = 0; row < 4; ++row) {
                    for (int component = 0; component < 4; ++component) {
                        result[static_cast<std::size_t>(column * 4 + row)] +=
                                left[static_cast<std::size_t>(component * 4 + row)] *
                                right[static_cast<std::size_t>(column * 4 + component)];
                    }
                }
            }

            return result;
        }

        [[nodiscard]] Matrix4 rotationX(const float angle) {
            Matrix4 result =
                    identityMatrix();

            const float cosine =
                    std::cos(angle);

            const float sine =
                    std::sin(angle);

            result[5] = cosine;
            result[6] = sine;
            result[9] = -sine;
            result[10] = cosine;
            return result;
        }

        [[nodiscard]] Matrix4 rotationY(const float angle) {
            Matrix4 result =
                    identityMatrix();

            const float cosine =
                    std::cos(angle);

            const float sine =
                    std::sin(angle);

            result[0] = cosine;
            result[2] = -sine;
            result[8] = sine;
            result[10] = cosine;
            return result;
        }

        [[nodiscard]] Matrix4 lookAt(
            const Vector3 &eye,
            const Vector3 &target,
            const Vector3 &worldUp
        ) {
            const Vector3 forward =
                    normalize(subtract(target, eye));

            const Vector3 right =
                    normalize(cross(forward, worldUp));

            const Vector3 up =
                    cross(right, forward);

            Matrix4 result =
                    identityMatrix();

            result[0] = right.x;
            result[4] = right.y;
            result[8] = right.z;
            result[1] = up.x;
            result[5] = up.y;
            result[9] = up.z;
            result[2] = -forward.x;
            result[6] = -forward.y;
            result[10] = -forward.z;
            result[12] = -dot(right, eye);
            result[13] = -dot(up, eye);
            result[14] = dot(forward, eye);
            return result;
        }

        [[nodiscard]] Matrix4 perspective(
            const float verticalFieldOfViewRadians,
            const float aspectRatio,
            const float nearPlane,
            const float farPlane
        ) {
            Matrix4 result{};

            const float focalLength =
                    1.0F / std::tan(verticalFieldOfViewRadians * 0.5F);

            result[0] = focalLength / aspectRatio;
            result[5] = focalLength;
            result[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
            result[11] = -1.0F;
            result[14] = (2.0F * farPlane * nearPlane) / (nearPlane - farPlane);
            return result;
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

            glGetShaderInfoLog(shader, logLength, nullptr, log.data());
            glDeleteShader(shader);
            throw std::runtime_error{
                std::string{"Failed to compile "} + label + ": " + log
            };
        }
    }

    void QaveCubeRenderer::initialize() {
        if (initialized_) {
            return;
        }

        try {
            createShaderProgram();
            createCubeMesh();

            glGenFramebuffers(1, &framebuffer_);
            glGenTextures(1, &colorTexture_);
            glGenRenderbuffers(1, &depthStencilBuffer_);
            initialized_ = true;
        } catch (...) {
            shutdown();
            throw;
        }
    }

    void QaveCubeRenderer::render(const int width, const int height) {
        if (!initialized_) {
            throw std::runtime_error{"QAVE cube renderer is not initialized."};
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
        glClearColor(0.012F, 0.020F, 0.032F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const Matrix4 model =
                multiply(
                    rotationY(-0.52F),
                    rotationX(0.31F)
                );

        const Matrix4 view =
                lookAt(
                    Vector3{2.7F, 2.1F, 3.4F},
                    Vector3{0.0F, 0.0F, 0.0F},
                    Vector3{0.0F, 1.0F, 0.0F}
                );

        const Matrix4 projection =
                perspective(
                    46.0F * pi / 180.0F,
                    static_cast<float>(safeWidth) / static_cast<float>(safeHeight),
                    0.1F,
                    100.0F
                );

        glUseProgram(shaderProgram_);
        glUniformMatrix4fv(modelUniform_, 1, GL_FALSE, model.data());
        glUniformMatrix4fv(viewUniform_, 1, GL_FALSE, view.data());
        glUniformMatrix4fv(projectionUniform_, 1, GL_FALSE, projection.data());
        glBindVertexArray(vertexArray_);

        // One indexed draw call is the complete first QAVE geometry pass.
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);

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

    void QaveCubeRenderer::shutdown() noexcept {
        if (depthStencilBuffer_ != 0U) {
            glDeleteRenderbuffers(1, &depthStencilBuffer_);
            depthStencilBuffer_ = 0U;
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
        initialized_ = false;
    }

    unsigned int QaveCubeRenderer::colorTexture() const noexcept {
        return colorTexture_;
    }

    bool QaveCubeRenderer::isInitialized() const noexcept {
        return initialized_;
    }

    void QaveCubeRenderer::createCubeMesh() {
        // Each face has distinct vertices so its normal remains flat and crisp.
        constexpr std::array<float, 144> vertices{
            // Front
            -0.7F, -0.7F,  0.7F,  0.0F,  0.0F,  1.0F,
             0.7F, -0.7F,  0.7F,  0.0F,  0.0F,  1.0F,
             0.7F,  0.7F,  0.7F,  0.0F,  0.0F,  1.0F,
            -0.7F,  0.7F,  0.7F,  0.0F,  0.0F,  1.0F,
            // Back
             0.7F, -0.7F, -0.7F,  0.0F,  0.0F, -1.0F,
            -0.7F, -0.7F, -0.7F,  0.0F,  0.0F, -1.0F,
            -0.7F,  0.7F, -0.7F,  0.0F,  0.0F, -1.0F,
             0.7F,  0.7F, -0.7F,  0.0F,  0.0F, -1.0F,
            // Left
            -0.7F, -0.7F, -0.7F, -1.0F,  0.0F,  0.0F,
            -0.7F, -0.7F,  0.7F, -1.0F,  0.0F,  0.0F,
            -0.7F,  0.7F,  0.7F, -1.0F,  0.0F,  0.0F,
            -0.7F,  0.7F, -0.7F, -1.0F,  0.0F,  0.0F,
            // Right
             0.7F, -0.7F,  0.7F,  1.0F,  0.0F,  0.0F,
             0.7F, -0.7F, -0.7F,  1.0F,  0.0F,  0.0F,
             0.7F,  0.7F, -0.7F,  1.0F,  0.0F,  0.0F,
             0.7F,  0.7F,  0.7F,  1.0F,  0.0F,  0.0F,
            // Top
            -0.7F,  0.7F,  0.7F,  0.0F,  1.0F,  0.0F,
             0.7F,  0.7F,  0.7F,  0.0F,  1.0F,  0.0F,
             0.7F,  0.7F, -0.7F,  0.0F,  1.0F,  0.0F,
            -0.7F,  0.7F, -0.7F,  0.0F,  1.0F,  0.0F,
            // Bottom
            -0.7F, -0.7F, -0.7F,  0.0F, -1.0F,  0.0F,
             0.7F, -0.7F, -0.7F,  0.0F, -1.0F,  0.0F,
             0.7F, -0.7F,  0.7F,  0.0F, -1.0F,  0.0F,
            -0.7F, -0.7F,  0.7F,  0.0F, -1.0F,  0.0F
        };

        constexpr std::array<unsigned int, 36> indices{
             0U,  1U,  2U,  2U,  3U,  0U,
             4U,  5U,  6U,  6U,  7U,  4U,
             8U,  9U, 10U, 10U, 11U,  8U,
            12U, 13U, 14U, 14U, 15U, 12U,
            16U, 17U, 18U, 18U, 19U, 16U,
            20U, 21U, 22U, 22U, 23U, 20U
        };

        glGenVertexArrays(1, &vertexArray_);
        glGenBuffers(1, &vertexBuffer_);
        glGenBuffers(1, &indexBuffer_);

        glBindVertexArray(vertexArray_);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(sizeof(vertices)),
            vertices.data(),
            GL_STATIC_DRAW
        );

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(sizeof(indices)),
            indices.data(),
            GL_STATIC_DRAW
        );

        constexpr int componentsPerVertex = 6;
        constexpr int positionComponents = 3;

        glVertexAttribPointer(
            0,
            positionComponents,
            GL_FLOAT,
            GL_FALSE,
            componentsPerVertex * static_cast<int>(sizeof(float)),
            nullptr
        );

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            1,
            positionComponents,
            GL_FLOAT,
            GL_FALSE,
            componentsPerVertex * static_cast<int>(sizeof(float)),
            reinterpret_cast<const void *>(positionComponents * sizeof(float))
        );

        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    void QaveCubeRenderer::createShaderProgram() {
        constexpr const char *vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vNormal;

void main() {
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vNormal = normalize(mat3(uModel) * aNormal);
    gl_Position = uProjection * uView * worldPosition;
}
)glsl";

        constexpr const char *fragmentShaderSource = R"glsl(
#version 330 core
in vec3 vNormal;

out vec4 fragmentColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDirection = normalize(vec3(-0.55, 0.85, 0.70));
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float topFace = max(normal.y, 0.0);
    float sideContrast = 0.82 + 0.18 * max(normal.z, 0.0);

    vec3 baseColor = vec3(0.08, 0.57, 0.92);
    vec3 accentColor = vec3(0.94, 0.27, 0.55);
    vec3 color = baseColor * (0.24 + 0.68 * diffuse) * sideContrast;
    color += accentColor * (0.08 + 0.18 * topFace);

    fragmentColor = vec4(color, 1.0);
}
)glsl";

        const unsigned int vertexShader =
                compileShader(GL_VERTEX_SHADER, vertexShaderSource, "QAVE vertex shader");

        unsigned int fragmentShader{};

        try {
            fragmentShader =
                    compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource, "QAVE fragment shader");

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
                    std::string{"Failed to link QAVE shader program: "} + log
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

        if (
            modelUniform_ < 0 ||
            viewUniform_ < 0 ||
            projectionUniform_ < 0
        ) {
            throw std::runtime_error{"QAVE shader matrix uniforms are unavailable."};
        }
    }

    void QaveCubeRenderer::resizeFramebuffer(const int width, const int height) {
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

        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER,
            depthStencilBuffer_
        );

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
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

            throw std::runtime_error{"QAVE framebuffer is incomplete."};
        }

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

        framebufferWidth_ = width;
        framebufferHeight_ = height;
    }
}

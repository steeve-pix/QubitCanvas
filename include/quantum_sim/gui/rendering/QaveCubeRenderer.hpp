#pragma once

namespace quantum_sim::gui {
    /**
     * Owns the first native OpenGL QAVE render pass.
     *
     * The renderer draws one indexed opaque cube into an off-screen framebuffer.
     * Dear ImGui only receives the resulting color texture.
     */
    class QaveCubeRenderer {
    public:
        QaveCubeRenderer() = default;
        ~QaveCubeRenderer() = default;

        QaveCubeRenderer(const QaveCubeRenderer &) = delete;
        QaveCubeRenderer &operator=(const QaveCubeRenderer &) = delete;
        QaveCubeRenderer(QaveCubeRenderer &&) = delete;
        QaveCubeRenderer &operator=(QaveCubeRenderer &&) = delete;

        /**
         * Creates the cube mesh, shader program, and framebuffer objects.
         *
         * A current OpenGL context and a successfully initialized GLAD loader
         * are required before calling this function.
         *
         * @throws std::runtime_error if shader compilation, program linking, or
         * framebuffer creation fails.
         */
        void initialize();

        /**
         * Draws one indexed cube into the off-screen framebuffer.
         *
         * @param width Framebuffer width in physical pixels.
         * @param height Framebuffer height in physical pixels.
         * @throws std::runtime_error if the framebuffer cannot be resized.
         */
        void render(int width, int height);

        /**
         * Releases every OpenGL object owned by the renderer.
         *
         * This must run while the application's OpenGL context is still current.
         */
        void shutdown() noexcept;

        /**
         * Returns the OpenGL color texture containing the most recent frame.
         *
         * @return OpenGL texture object name, or zero before initialization.
         */
        [[nodiscard]] unsigned int colorTexture() const noexcept;

        /**
         * Reports whether the renderer has completed OpenGL initialization.
         *
         * @return true after initialize() succeeds and before shutdown().
         */
        [[nodiscard]] bool isInitialized() const noexcept;

    private:
        unsigned int vertexArray_{};
        unsigned int vertexBuffer_{};
        unsigned int indexBuffer_{};
        unsigned int shaderProgram_{};
        unsigned int framebuffer_{};
        unsigned int colorTexture_{};
        unsigned int depthStencilBuffer_{};
        int framebufferWidth_{};
        int framebufferHeight_{};
        int modelUniform_{-1};
        int viewUniform_{-1};
        int projectionUniform_{-1};
        bool initialized_{false};

        /**
         * Uploads the cube's face vertices and triangle indices.
         */
        void createCubeMesh();

        /**
         * Compiles and links the OpenGL 3.3 cube shaders.
         */
        void createShaderProgram();

        /**
         * Allocates framebuffer color and depth storage for a new viewport size.
         *
         * @param width Framebuffer width in physical pixels.
         * @param height Framebuffer height in physical pixels.
         * @throws std::runtime_error if OpenGL reports an incomplete framebuffer.
         */
        void resizeFramebuffer(int width, int height);
    };
}

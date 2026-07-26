#pragma once

#include "quantum_sim/gui/rendering/DensityVolumeCameraController.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeModel.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeScene.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace quantum_sim::gui::density_volume {
    /**
     * Instanced OpenGL renderer for the interactive density-matrix history.
     *
     * A shared rounded cube is drawn once per visible density cell. A separate
     * instanced edge pass preserves the shape of exact small matrices without
     * filling near-zero cells. The scene is rendered into linear HDR,
     * integer-picking, and emissive attachments before restrained bloom and
     * linear-to-sRGB conversion produce the Dear ImGui texture.
     */
    class Renderer {
    public:
        Renderer() = default;
        ~Renderer() = default;

        Renderer(const Renderer &) = delete;
        Renderer &operator=(const Renderer &) = delete;
        Renderer(Renderer &&) = delete;
        Renderer &operator=(Renderer &&) = delete;

        /**
         * Creates shaders, shared geometry, instance buffers, and framebuffers.
         *
         * A current OpenGL 3.3 Core context and initialized GLAD loader are
         * required.
         *
         * @throws std::runtime_error when an OpenGL resource cannot be created.
         */
        void initialize();

        /**
         * Converts changed density data and uploads compact voxel instances.
         *
         * Layer-stack playback uploads the complete history only when its
         * fingerprint changes. Moving between debugger steps then changes only
         * the number of instances submitted to glDrawElementsInstanced().
         *
         * @param stack Shared density history.
         * @param selectedLayer Debugger layer visible to the user.
         * @param mode Spatial presentation to render.
         * @return true when scene bounds or visible contents changed.
         */
        bool updateScene(
            const DensityStack &stack,
            std::size_t selectedLayer,
            VisualizationMode mode
        );

        /**
         * Renders the current instance range into the final display texture.
         *
         * @param width Framebuffer width in physical pixels.
         * @param height Framebuffer height in physical pixels.
         * @param selectedLayer Layer emphasized by the synchronized views.
         * @param heatAmount User-controlled glow multiplier.
         * @param camera Camera supplying view and projection matrices.
         */
        void render(
            int width,
            int height,
            std::size_t selectedLayer,
            float heatAmount,
            const CameraController &camera
        );

        /**
         * Reads one stable cell ID from the integer picking attachment.
         *
         * @param x Pixel coordinate from the framebuffer's left edge.
         * @param y Pixel coordinate from the framebuffer's bottom edge.
         * @return Picked matrix cell, or std::nullopt for the background.
         */
        [[nodiscard]] std::optional<Selection> pick(int x, int y) const;

        /**
         * Releases every owned OpenGL object while the context is current.
         */
        void shutdown() noexcept;

        /**
         * Returns the bloom-composited sRGB texture displayed by ImGui::Image().
         */
        [[nodiscard]] unsigned int colorTexture() const noexcept;

        /**
         * Returns the stable full-scene camera target.
         */
        [[nodiscard]] Vector3 sceneCenter() const noexcept;

        /**
         * Returns the stable full-scene bounding-sphere radius.
         */
        [[nodiscard]] float sceneRadius() const noexcept;

        /**
         * Reports whether OpenGL initialization completed.
         */
        [[nodiscard]] bool isInitialized() const noexcept;

    private:
        unsigned int voxelVertexArray_{};
        unsigned int voxelVertexBuffer_{};
        unsigned int voxelIndexBuffer_{};
        unsigned int instanceBuffer_{};
        unsigned int ghostVertexArray_{};
        unsigned int ghostVertexBuffer_{};
        unsigned int ghostIndexBuffer_{};
        unsigned int ghostInstanceBuffer_{};
        unsigned int gridVertexArray_{};
        unsigned int gridVertexBuffer_{};
        unsigned int gridIndexBuffer_{};
        unsigned int voxelShaderProgram_{};
        unsigned int ghostShaderProgram_{};
        unsigned int gridShaderProgram_{};
        unsigned int blurShaderProgram_{};
        unsigned int compositeShaderProgram_{};
        unsigned int postProcessVertexArray_{};
        unsigned int framebuffer_{};
        unsigned int colorTexture_{};
        unsigned int sceneColorTexture_{};
        unsigned int brightTexture_{};
        unsigned int pickTexture_{};
        unsigned int depthStencilBuffer_{};
        unsigned int blurFramebuffers_[2]{};
        unsigned int blurTextures_[2]{};
        unsigned int compositeFramebuffer_{};
        int framebufferWidth_{};
        int framebufferHeight_{};
        int voxelViewUniform_{-1};
        int voxelProjectionUniform_{-1};
        int voxelSelectedLayerUniform_{-1};
        int voxelHeatUniform_{-1};
        int ghostViewUniform_{-1};
        int ghostProjectionUniform_{-1};
        int ghostSelectedLayerUniform_{-1};
        int gridViewUniform_{-1};
        int gridProjectionUniform_{-1};
        int gridCenterUniform_{-1};
        int gridExtentUniform_{-1};
        int blurInputUniform_{-1};
        int blurHorizontalUniform_{-1};
        int compositeSceneUniform_{-1};
        int compositeBloomUniform_{-1};
        int cubeIndexCount_{};
        int visibleInstanceCount_{};
        int visibleGhostCount_{};
        std::size_t instanceCapacity_{};
        std::size_t ghostInstanceCapacity_{};
        std::uint64_t sceneFingerprint_{};
        std::optional<std::size_t> sceneSelectedLayer_;
        std::optional<VisualizationMode> sceneMode_;
        InstanceScene scene_;
        VoxelGeometry voxelGeometry_;
        Vector3 sceneCenter_{};
        float sceneRadius_{1.0F};
        bool initialized_{false};

        /**
         * Compiles the solid, edge-ghost, and procedural-grid programs.
         */
        void createScenePrograms();

        /**
         * Compiles bloom blur and HDR tone-mapping programs.
         */
        void createPostProcessPrograms();

        /**
         * Uploads the shared rounded cube and defines per-instance attributes.
         */
        void createVoxelBuffers();

        /**
         * Uploads the indexed unit-cube edges used for near-zero matrix cells.
         */
        void createGhostBuffers();

        /**
         * Creates the normalized quad used by the procedural ground grid.
         */
        void createGridBuffers();

        /**
         * Creates the empty core-profile VAO for full-screen triangle passes.
         */
        void createPostProcessVertexArray();

        /**
         * Uploads all compact records while retaining reusable GPU capacity.
         */
        void uploadInstances();

        /**
         * Allocates HDR, bloom, picking, final-color, and depth attachments.
         */
        void resizeFramebuffer(int width, int height);

        /**
         * Blurs emissive highlights and converts the final linear scene to sRGB.
         */
        void renderPostProcess(int width, int height);
    };
}

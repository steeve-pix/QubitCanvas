#pragma once

#include "quantum_sim/gui/rendering/DensityVolumeCameraController.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeModel.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeMeshBuilder.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace quantum_sim::gui::density_volume {
    /**
     * Raw OpenGL renderer for the interactive Density Volume density-matrix history.
     *
     * The renderer owns the scene VAO/VBO/EBO, shader program, color texture,
     * integer picking texture, depth buffer, and framebuffer.
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
         * Creates shader, mesh-buffer, framebuffer, and texture objects.
         *
         * A current OpenGL 3.3 Core context and initialized GLAD loader are required.
         *
         * @throws std::runtime_error when shader or OpenGL object creation fails.
         */
        void initialize();

        /**
         * Rebuilds and uploads geometry when the density history changes.
         *
         * @param stack Shared density history.
         * @param selectedLayer Last historical layer or selected floor.
         * @param mode Spatial presentation to upload.
         * @return true when a new mesh was uploaded.
         */
        bool updateScene(
            const DensityStack &stack,
            std::size_t selectedLayer,
            VisualizationMode mode
        );

        /**
         * Renders the uploaded stack into the off-screen framebuffer.
         *
         * @param width Framebuffer width in physical pixels.
         * @param height Framebuffer height in physical pixels.
         * @param selectedLayer Layer highlighted by both Density Volume views.
         * @param camera Camera supplying view and projection matrices.
         */
        void render(
            int width,
            int height,
            std::size_t selectedLayer,
            const CameraController &camera
        );

        /**
         * Reads one integer pick ID from the picking attachment.
         *
         * @param x Pixel coordinate from the framebuffer's left edge.
         * @param y Pixel coordinate from the framebuffer's bottom edge.
         * @return Selected density cell, or std::nullopt for empty background.
         */
        [[nodiscard]] std::optional<Selection> pick(int x, int y) const;

        /**
         * Releases every owned OpenGL object while the context is current.
         */
        void shutdown() noexcept;

        /**
         * Returns the OpenGL color texture displayed by ImGui::Image().
         */
        [[nodiscard]] unsigned int colorTexture() const noexcept;

        /**
         * Returns the uploaded scene's camera target.
         */
        [[nodiscard]] Vector3 sceneCenter() const noexcept;

        /**
         * Returns the uploaded scene's bounding-sphere radius.
         */
        [[nodiscard]] float sceneRadius() const noexcept;

        /**
         * Reports whether OpenGL initialization completed.
         */
        [[nodiscard]] bool isInitialized() const noexcept;

    private:
        unsigned int vertexArray_{};
        unsigned int vertexBuffer_{};
        unsigned int indexBuffer_{};
        unsigned int shaderProgram_{};
        unsigned int framebuffer_{};
        unsigned int colorTexture_{};
        unsigned int pickTexture_{};
        unsigned int depthStencilBuffer_{};
        int framebufferWidth_{};
        int framebufferHeight_{};
        int modelUniform_{-1};
        int viewUniform_{-1};
        int projectionUniform_{-1};
        int selectedLayerUniform_{-1};
        std::size_t indexCount_{};
        std::uint64_t sceneFingerprint_{};
        std::optional<std::size_t> sceneVisibleThroughLayer_;
        std::optional<VisualizationMode> sceneMode_;
        Vector3 sceneCenter_{};
        float sceneRadius_{1.0F};
        Mesh sceneMesh_;
        std::vector<std::size_t> layerIndexCounts_;
        std::vector<Vector3> layerSceneCenters_;
        std::vector<float> layerSceneRadii_;
        std::optional<std::size_t> builtThroughLayer_;
        std::size_t vertexCapacity_{};
        std::size_t indexCapacity_{};
        bool initialized_{false};

        /**
         * Compiles and links the Density Volume color/picking shader program.
         */
        void createShaderProgram();

        /**
         * Creates the VAO, VBO, EBO, and interleaved vertex attributes.
         */
        void createMeshBuffers();

        /**
         * Uploads one generated scene mesh to the existing VBO and EBO.
         */
        void uploadMesh(const Mesh &mesh);

        /**
         * Preallocates GPU storage for an incrementally generated layer stack.
         *
         * @param vertexCapacity Maximum number of MeshVertex elements.
         * @param indexCapacity Maximum number of uint32 indices.
         */
        void allocateMeshStorage(
            std::size_t vertexCapacity,
            std::size_t indexCapacity
        );

        /**
         * Uploads only the suffix appended since the previous playback step.
         *
         * @param mesh Complete CPU-side scene mesh.
         * @param firstVertex First newly appended vertex.
         * @param firstIndex First newly appended index.
         */
        void uploadMeshSuffix(
            const Mesh &mesh,
            std::size_t firstVertex,
            std::size_t firstIndex
        );

        /**
         * Allocates color, picking, and depth attachments for a viewport size.
         */
        void resizeFramebuffer(int width, int height);
    };
}

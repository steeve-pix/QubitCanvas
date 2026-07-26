#pragma once

#include "quantum_sim/gui/rendering/DensityVolumeMath.hpp"

namespace quantum_sim::gui::density_volume {
    /**
     * Smoothed orbit camera matching QuantumAtom's mouse behavior.
     */
    class CameraController {
    public:
        /**
         * Frames a new scene and stores that composition as the reset view.
         *
         * @param center Stable look-at target at the scene bounds center.
         * @param radius Bounding-sphere radius used for distance and clip planes.
         */
        void frameScene(const Vector3 &center, float radius);

        /**
         * Refreshes scene-aware reset and clipping bounds without moving the camera.
         *
         * Playback calls this when the visible layer range changes. The current
         * orbit, pan target, and zoom remain untouched, while a later reset uses
         * the newest complete-scene composition.
         *
         * @param center Stable look-at target at the latest scene bounds center.
         * @param radius Bounding-sphere radius used for reset distance and clip planes.
         */
        void updateSceneBounds(const Vector3 &center, float radius) noexcept;

        /**
         * Advances current camera values toward their input targets.
         *
         * @param deltaTime Frame duration in seconds.
         */
        void update(float deltaTime) noexcept;

        /**
         * Applies QuantumAtom-style left-drag orbit input.
         *
         * @param deltaX Horizontal mouse delta in pixels.
         * @param deltaY Vertical mouse delta in pixels.
         */
        void orbit(float deltaX, float deltaY) noexcept;

        /**
         * Applies QuantumAtom-style right-drag or Shift+left-drag panning.
         *
         * @param deltaX Horizontal mouse delta in pixels.
         * @param deltaY Vertical mouse delta in pixels.
         */
        void pan(float deltaX, float deltaY);

        /**
         * Applies mouse-wheel dolly input.
         *
         * @param wheelDelta Positive values move toward the scene.
         */
        void zoom(float wheelDelta) noexcept;

        /**
         * Restores the most recent frameScene() composition.
         */
        void reset() noexcept;

        /**
         * Computes the current right-handed view matrix.
         */
        [[nodiscard]] Matrix4 viewMatrix() const;

        /**
         * Computes a perspective projection with scene-aware clip planes.
         *
         * @param width Viewport width in physical pixels.
         * @param height Viewport height in physical pixels.
         */
        [[nodiscard]] Matrix4 projectionMatrix(int width, int height) const;

        /**
         * Reports whether frameScene() has initialized the controller.
         */
        [[nodiscard]] bool isFramed() const noexcept;

    private:
        float yawDegrees_{-40.0F};
        float pitchDegrees_{25.0F};
        float distance_{20.0F};
        Vector3 target_{};

        float targetYawDegrees_{-40.0F};
        float targetPitchDegrees_{25.0F};
        float targetDistance_{20.0F};
        Vector3 destinationTarget_{};

        float launchYawDegrees_{-40.0F};
        float launchPitchDegrees_{25.0F};
        float launchDistance_{20.0F};
        Vector3 launchTarget_{};
        float sceneRadius_{4.0F};
        float smoothness_{2.5F};
        bool framed_{false};
    };
}

#include "quantum_sim/gui/rendering/DensityVolumeCameraController.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace quantum_sim::gui::density_volume {
    namespace {
        [[nodiscard]] float radians(const float degrees) noexcept {
            return degrees *
                   std::numbers::pi_v<float> /
                   180.0F;
        }
    }

    void CameraController::frameScene(
        const Vector3 &center,
        const float radius,
        const bool floorField
    ) {
        updateSceneBounds(
            center,
            radius,
            floorField
        );
        reset();
        framed_ = true;
    }

    void CameraController::updateSceneBounds(
        const Vector3 &center,
        const float radius,
        const bool floorField
    ) noexcept {
        sceneRadius_ =
                std::max(radius, 1.0F);

        launchYawDegrees_ =
                floorField
                    ? 42.0F
                    : 110.0F;

        launchPitchDegrees_ =
                floorField
                    ? 46.0F
                    : 25.0F;

        launchDistance_ =
                std::max(
                    sceneRadius_ *
                    (floorField ? 2.05F : 1.90F),
                    3.2F
                );

        // Playback only refreshes clipping and the reset composition. The live
        // camera follows untouched playback, but never overwrites a pose the
        // user has deliberately orbit/panned/zoomed.
        launchTarget_ =
                center;

        if (framed_ && !userControlled_) {
            targetYawDegrees_ =
                    launchYawDegrees_;

            targetPitchDegrees_ =
                    launchPitchDegrees_;

            targetDistance_ =
                    launchDistance_;

            destinationTarget_ =
                    launchTarget_;
        }
    }

    void CameraController::update(const float deltaTime) noexcept {
        const float safeDeltaTime =
                std::clamp(deltaTime, 0.0F, 0.1F);

        const float blend =
                1.0F -
                std::exp(-smoothness_ * safeDeltaTime);

        yawDegrees_ +=
                (targetYawDegrees_ - yawDegrees_) * blend;

        pitchDegrees_ +=
                (targetPitchDegrees_ - pitchDegrees_) * blend;

        distance_ +=
                (targetDistance_ - distance_) * blend;

        target_.x +=
                (destinationTarget_.x - target_.x) * blend;

        target_.y +=
                (destinationTarget_.y - target_.y) * blend;

        target_.z +=
                (destinationTarget_.z - target_.z) * blend;
    }

    void CameraController::orbit(
        const float deltaX,
        const float deltaY
    ) noexcept {
        userControlled_ = true;

        targetYawDegrees_ +=
                deltaX * 0.18F;

        targetPitchDegrees_ =
                std::clamp(
                    targetPitchDegrees_ - deltaY * 0.18F,
                    -89.0F,
                    89.0F
                );
    }

    void CameraController::pan(
        const float deltaX,
        const float deltaY
    ) {
        userControlled_ = true;

        const float yaw =
                radians(yawDegrees_);

        const float pitch =
                radians(pitchDegrees_);

        Vector3 forward{
            -std::cos(yaw) * std::cos(pitch),
            -std::sin(pitch),
            -std::sin(yaw) * std::cos(pitch)
        };

        forward =
                normalize(forward);

        const Vector3 right =
                normalize(
                    cross(
                        forward,
                        Vector3{0.0F, 1.0F, 0.0F}
                    )
                );

        const Vector3 up =
                cross(right, forward);

        const float factor =
                distance_ * 0.0012F;

        destinationTarget_ =
                destinationTarget_ +
                right * (-deltaX * factor) +
                up * (deltaY * factor);
    }

    void CameraController::zoom(const float wheelDelta) noexcept {
        userControlled_ = true;

        const float zoomStep =
                std::max(
                    targetDistance_ * 0.12F,
                    0.8F
                );

        targetDistance_ =
                std::clamp(
                    targetDistance_ - wheelDelta * zoomStep,
                    std::max(sceneRadius_ * 0.42F, 2.0F),
                    std::max(sceneRadius_ * 8.0F, 30.0F)
                );
    }

    void CameraController::reset() noexcept {
        yawDegrees_ = launchYawDegrees_;
        pitchDegrees_ = launchPitchDegrees_;
        distance_ = launchDistance_;
        target_ = launchTarget_;

        targetYawDegrees_ = launchYawDegrees_;
        targetPitchDegrees_ = launchPitchDegrees_;
        targetDistance_ = launchDistance_;
        destinationTarget_ = launchTarget_;
        userControlled_ = false;
    }

    Matrix4 CameraController::viewMatrix() const {
        const float yaw =
                radians(yawDegrees_);

        const float pitch =
                radians(pitchDegrees_);

        const Vector3 offset{
            distance_ * std::cos(yaw) * std::cos(pitch),
            distance_ * std::sin(pitch),
            distance_ * std::sin(yaw) * std::cos(pitch)
        };

        return lookAtMatrix(
            target_ + offset,
            target_,
            Vector3{0.0F, 1.0F, 0.0F}
        );
    }

    Matrix4 CameraController::projectionMatrix(
        const int width,
        const int height
    ) const {
        const float aspect =
                height > 0
                    ? static_cast<float>(width) /
                      static_cast<float>(height)
                    : 1.0F;

        const float farPlane =
                std::max(
                    100.0F,
                    distance_ + sceneRadius_ * 8.0F
                );

        return perspectiveMatrix(
            radians(46.0F),
            aspect,
            0.05F,
            farPlane
        );
    }

    bool CameraController::isFramed() const noexcept {
        return framed_;
    }
}

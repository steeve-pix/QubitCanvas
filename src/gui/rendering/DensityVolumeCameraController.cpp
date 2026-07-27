#include "quantum_sim/gui/rendering/DensityVolumeCameraController.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace quantum_sim::gui::density_volume {
    namespace {
        constexpr float verticalFieldOfViewDegrees = 46.0F;

        [[nodiscard]] float radians(const float degrees) noexcept {
            return degrees *
                   std::numbers::pi_v<float> /
                   180.0F;
        }

        [[nodiscard]] Vector3 boundsCenter(
            const Vector3 &minimum,
            const Vector3 &maximum
        ) noexcept {
            return (minimum + maximum) * 0.5F;
        }

        [[nodiscard]] float fittedOrbitDistance(
            const Vector3 &minimum,
            const Vector3 &maximum,
            const Vector3 &center,
            const float yawDegrees,
            const float pitchDegrees,
            const float viewportAspect
        ) noexcept {
            const float yaw =
                    radians(yawDegrees);

            const float pitch =
                    radians(pitchDegrees);

            const float cosineYaw =
                    std::cos(yaw);

            const float sineYaw =
                    std::sin(yaw);

            const float cosinePitch =
                    std::cos(pitch);

            const float sinePitch =
                    std::sin(pitch);

            const Vector3 forward{
                -cosineYaw * cosinePitch,
                -sinePitch,
                -sineYaw * cosinePitch
            };

            const Vector3 right{
                sineYaw,
                0.0F,
                -cosineYaw
            };

            const Vector3 up{
                -cosineYaw * sinePitch,
                cosinePitch,
                -sineYaw * sinePitch
            };

            const float tangentHalfVertical =
                    std::tan(
                        radians(verticalFieldOfViewDegrees) *
                        0.5F
                    );

            const float tangentHalfHorizontal =
                    tangentHalfVertical *
                    std::max(viewportAspect, 0.1F);

            const std::array<Vector3, 8U> corners{
                Vector3{minimum.x, minimum.y, minimum.z},
                Vector3{minimum.x, minimum.y, maximum.z},
                Vector3{minimum.x, maximum.y, minimum.z},
                Vector3{minimum.x, maximum.y, maximum.z},
                Vector3{maximum.x, minimum.y, minimum.z},
                Vector3{maximum.x, minimum.y, maximum.z},
                Vector3{maximum.x, maximum.y, minimum.z},
                Vector3{maximum.x, maximum.y, maximum.z}
            };

            float requiredDistance{};

            for (const Vector3 &corner : corners) {
                const Vector3 relative =
                        corner - center;

                const float depthOffset =
                        dot(relative, forward);

                requiredDistance =
                        std::max({
                            requiredDistance,
                            std::abs(dot(relative, right)) /
                                tangentHalfHorizontal -
                                depthOffset,
                            std::abs(dot(relative, up)) /
                                tangentHalfVertical -
                                depthOffset
                        });
            }

            const float radius =
                    length(maximum - minimum) * 0.5F;

            return std::max(
                requiredDistance +
                    radius * 0.08F +
                    0.2F,
                3.2F
            );
        }
    }

    void CameraController::frameScene(
        const Vector3 &minimum,
        const Vector3 &maximum,
        const float viewportAspect,
        const bool floorField
    ) {
        updateSceneBounds(
            minimum,
            maximum,
            viewportAspect,
            floorField
        );
        reset();
        framed_ = true;
    }

    void CameraController::updateSceneBounds(
        const Vector3 &minimum,
        const Vector3 &maximum,
        const float viewportAspect,
        const bool floorField
    ) noexcept {
        const Vector3 safeMinimum{
            std::min(minimum.x, maximum.x),
            std::min(minimum.y, maximum.y),
            std::min(minimum.z, maximum.z)
        };

        const Vector3 safeMaximum{
            std::max(minimum.x, maximum.x),
            std::max(minimum.y, maximum.y),
            std::max(minimum.z, maximum.z)
        };

        const Vector3 halfExtent =
                (safeMaximum - safeMinimum) * 0.5F;

        focusRadius_ =
                std::max(
                    std::sqrt(
                        halfExtent.x * halfExtent.x +
                        halfExtent.z * halfExtent.z
                    ),
                    1.0F
                );

        sceneRadius_ =
                std::max(
                    length(halfExtent),
                    1.0F
                );

        launchYawDegrees_ =
                floorField
                    ? 42.0F
                    : 120.0F;

        launchPitchDegrees_ =
                floorField
                    ? 46.0F
                    : 25.0F;

        launchTarget_ =
                boundsCenter(
                    safeMinimum,
                    safeMaximum
                );

        launchDistance_ =
                fittedOrbitDistance(
                    safeMinimum,
                    safeMaximum,
                    launchTarget_,
                    launchYawDegrees_,
                    launchPitchDegrees_,
                    viewportAspect
                );

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
                    std::max(focusRadius_ * 0.42F, 2.0F),
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
            radians(verticalFieldOfViewDegrees),
            aspect,
            0.05F,
            farPlane
        );
    }

    bool CameraController::isFramed() const noexcept {
        return framed_;
    }

    float CameraController::orbitDistance() const noexcept {
        return distance_;
    }
}

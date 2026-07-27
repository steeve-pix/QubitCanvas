#include "quantum_sim/gui/rendering/DensityVolumeMath.hpp"

#include <cmath>
#include <stdexcept>

namespace quantum_sim::gui::density_volume {
    Vector3 operator+(const Vector3 &left, const Vector3 &right) noexcept {
        return Vector3{
            left.x + right.x,
            left.y + right.y,
            left.z + right.z
        };
    }

    Vector3 operator-(const Vector3 &left, const Vector3 &right) noexcept {
        return Vector3{
            left.x - right.x,
            left.y - right.y,
            left.z - right.z
        };
    }

    Vector3 operator*(const Vector3 &value, const float scalar) noexcept {
        return Vector3{
            value.x * scalar,
            value.y * scalar,
            value.z * scalar
        };
    }

    float dot(const Vector3 &left, const Vector3 &right) noexcept {
        return left.x * right.x + left.y * right.y + left.z * right.z;
    }

    Vector3 cross(const Vector3 &left, const Vector3 &right) noexcept {
        return Vector3{
            left.y * right.z - left.z * right.y,
            left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x
        };
    }

    float length(const Vector3 &value) noexcept {
        return std::sqrt(dot(value, value));
    }

    Vector3 normalize(const Vector3 &value) {
        const float vectorLength =
                length(value);

        if (vectorLength <= 0.000001F) {
            throw std::runtime_error{"Cannot normalize a zero-length Density Volume vector."};
        }

        return Vector3{
            value.x / vectorLength,
            value.y / vectorLength,
            value.z / vectorLength
        };
    }

    Matrix4 identityMatrix() noexcept {
        return Matrix4{
            1.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 1.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 1.0F, 0.0F,
            0.0F, 0.0F, 0.0F, 1.0F
        };
    }

    Matrix4 lookAtMatrix(
        const Vector3 &eye,
        const Vector3 &target,
        const Vector3 &worldUp
    ) {
        const Vector3 forward =
                normalize(target - eye);

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

    Matrix4 perspectiveMatrix(
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
}

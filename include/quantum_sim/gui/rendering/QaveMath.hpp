#pragma once

#include <array>

namespace quantum_sim::gui::qave {
    /**
     * Three-component vector used by the QAVE camera and scene layout.
     */
    struct Vector3 {
        float x{};
        float y{};
        float z{};
    };

    /**
     * Column-major 4x4 matrix compatible with glUniformMatrix4fv().
     */
    using Matrix4 = std::array<float, 16>;

    /**
     * Adds two vectors component by component.
     */
    [[nodiscard]] Vector3 operator+(const Vector3 &left, const Vector3 &right) noexcept;

    /**
     * Subtracts right from left component by component.
     */
    [[nodiscard]] Vector3 operator-(const Vector3 &left, const Vector3 &right) noexcept;

    /**
     * Scales every vector component.
     */
    [[nodiscard]] Vector3 operator*(const Vector3 &value, float scalar) noexcept;

    /**
     * Computes the vector dot product.
     */
    [[nodiscard]] float dot(const Vector3 &left, const Vector3 &right) noexcept;

    /**
     * Computes a vector perpendicular to left and right.
     */
    [[nodiscard]] Vector3 cross(const Vector3 &left, const Vector3 &right) noexcept;

    /**
     * Returns the Euclidean vector length.
     */
    [[nodiscard]] float length(const Vector3 &value) noexcept;

    /**
     * Returns a unit-length copy of value.
     *
     * @throws std::runtime_error when value has effectively zero length.
     */
    [[nodiscard]] Vector3 normalize(const Vector3 &value);

    /**
     * Creates a column-major identity matrix.
     */
    [[nodiscard]] Matrix4 identityMatrix() noexcept;

    /**
     * Creates a right-handed look-at view matrix.
     */
    [[nodiscard]] Matrix4 lookAtMatrix(
        const Vector3 &eye,
        const Vector3 &target,
        const Vector3 &worldUp
    );

    /**
     * Creates a right-handed perspective projection matrix.
     */
    [[nodiscard]] Matrix4 perspectiveMatrix(
        float verticalFieldOfViewRadians,
        float aspectRatio,
        float nearPlane,
        float farPlane
    );
}

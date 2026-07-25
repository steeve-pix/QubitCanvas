#pragma once

namespace quantum_sim::math {
    /**
     * Lightweight immutable complex number used by the simulator math types.
     *
     * The project keeps its own complex type so matrix/vector code can stay small,
     * explicit, and independent from formatting choices in std::complex.
     */
    class Complex final {
    public:
        /**
         * Creates a complex number of the form real + imaginary * i.
         *
         * @param real Real component.
         * @param imaginary Imaginary component.
         */
        explicit Complex(double real = 0.0, double imaginary = 0.0);

        /**
         * @return The real component.
         */
        [[nodiscard]] double real() const noexcept;

        /**
         * @return The imaginary component.
         */
        [[nodiscard]] double imaginary() const noexcept;

        /**
         * @return The complex conjugate, real - imaginary * i.
         */
        [[nodiscard]] Complex conjugate() const noexcept;

        /**
         * @return Squared magnitude, real^2 + imaginary^2.
         */
        [[nodiscard]] double magnitudeSquared() const noexcept;

        /**
         * @return Magnitude, sqrt(real^2 + imaginary^2).
         */
        [[nodiscard]] double magnitude() const noexcept;

        /**
         * Adds two complex numbers component by component.
         *
         * @param other Value to add.
         * @return The sum of this number and other.
         */
        [[nodiscard]] Complex operator+(const Complex &other) const noexcept;

        /**
         * Multiplies two complex numbers using (a + bi)(c + di).
         *
         * @param other Value to multiply by.
         * @return Product of this number and other.
         */
        [[nodiscard]] Complex operator*(const Complex &other) const noexcept;

        /**
         * Divides both components by a real scalar.
         *
         * @param scalar Non-zero divisor.
         * @return Scaled complex number.
         * @throws std::invalid_argument if scalar is zero.
         */
        [[nodiscard]] Complex operator/(double scalar) const;

        /**
         * Adds another complex value in place.
         *
         * @param other Value to accumulate.
         * @return Reference to this updated value.
         */
        Complex &operator+=(const Complex &other) noexcept;

    private:
        double real_{0.0};
        double imaginary_{0.0};
    };
}

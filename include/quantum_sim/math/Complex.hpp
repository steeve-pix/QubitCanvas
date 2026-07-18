#pragma once

namespace quantum_sim::math {
    /**
     * @brief Represents a complex number with real and imaginary components.
     *
     * A complex number is of the form a + bi, where 'a' is the real component, and 'b' is the imaginary component.
     *
     * This class provides member functions and utilities for operations involving complex numbers.
     */
    class Complex final {
    public:
        /**
        * @brief Constructs a complex number.
        *
        * Both components default to zero, so Complex{} represents 0 + 0i.
        *
        * @param real The real component.
        * @param imaginary The imaginary component.
        */
        explicit Complex(double real = 0.0, double imaginary = 0.0);

        /**
         * @brief Returns the real component.
         * @return The value before the imaginary unit i.
         */
        [[nodiscard]] double real() const noexcept;

        /**
         * @brief Returns the imaginary component.
         * @return The coefficient of the imaginary unit i.
         */
        [[nodiscard]] double imaginary() const noexcept;

        /**
         * @brief Computes the complex conjugate.
         *
         * The conjugate of a + bi is a - bi. Conjugation is used when computing
         * the complex inner product of two vectors.
         *
         * @return A new complex number with the imaginary sign reversed.
         */
        [[nodiscard]] Complex conjugate() const noexcept;

        /**
         * @brief Computes the squared magnitude of the number.
         *
         * This avoids the square-root operation required by magnitude() and is
         * useful when only a norm comparison is needed.
         *
         * @return real^2 + imaginary^2.
         */
        [[nodiscard]] double magnitudeSquared() const noexcept;

        /**
         * @brief Computes the magnitude (absolute value).
         * @return sqrt(real^2 + imaginary^2).
         */
        [[nodiscard]] double magnitude() const noexcept;

        /**
         * @brief Adds two complex numbers component by component.
         * @param other The value to add.
         * @return The sum of this value and @p other.
         */
        [[nodiscard]] Complex operator+(const Complex &other) const noexcept;

        /**
         * @brief Multiplies two complex numbers.
         *
         * Uses (a + bi)(c + di) = (ac - bd) + (ad + bc)i.
         *
         * @param other The value to multiply by.
         * @return The product of this value and @p other.
         */
        [[nodiscard]] Complex operator*(const Complex &other) const noexcept;

        /**
         * @brief Divides both components by a real scalar.
         * @param scalar The non-zero divisor.
         * @return A new complex number containing the quotient.
         * @throws std::invalid_argument If @p scalar is zero.
         */
        [[nodiscard]] Complex operator/(double scalar) const;

        /**
         * @brief Adds another complex number to this object in place.
         * @param other The value to accumulate.
         * @return A reference to this updated object.
         */
        Complex &operator+=(const Complex &other) noexcept;

        /**
         * @brief Represents the real component of a complex number.
         *
         * This member holds the coefficient of the real part in the form a + bi,
         * where a is the value stored in @p real_ and b is the coefficient of the
         * imaginary part.
         *
         * The real component is initialized to 0.0 by default upon object construction.
         */
    private:
        double real_{0.0};
        /**
         * @brief Stores the imaginary component of the complex number.
         *
         * Represents the coefficient of the imaginary unit i. This is used
         * in mathematical operations involving complex numbers.
         *
         * Defaults to 0.0, corresponding to no imaginary component.
         */
        double imaginary_{0.0};
    };
}

#pragma once

namespace quantum_sim::math {
    class Complex final {
    public:
        /**
         * @brief Constructs a complex number with specified real and imaginary parts.
         * @param real The real component of the complex number.
         * @param imaginary The imaginary component of the complex number.
         */
        explicit Complex(double real = 0.0, double imaginary = 0.0);

        /**
         * @brief Retrieves the real component of the complex number.
         * @return The real part as a double.
         */
        [[nodiscard]] double real() const noexcept;

        /**
         * @brief Retrieves the imaginary component of the complex number.
         * @return The imaginary part as a double.
         */
        [[nodiscard]] double imaginary() const noexcept;

        /**
         * @brief Computes the complex conjugate of this number.
         * @details Flips the sign of the imaginary component (a + bi becomes a - bi).
         * @return A new Complex object representing the conjugate.
         */
        [[nodiscard]] Complex conjugate() const noexcept;

        /**
         * @brief Returns the squared magnitude of the complex number.
         * @return real² + imaginary².
         */
        [[nodiscard]] double magnitudeSquared() const noexcept;

        /**
        * @brief Adds another complex number to this one.
        * @param other The complex number to add.
        * @return A new Complex object representing the sum.
        */
        [[nodiscard]] Complex operator+(const Complex &other) const noexcept;

        /**
         * @brief Multiplies another complex number with this one.
         * @details Uses the FOIL method formula: (a+bi)(c+di) = (ac-bd) + (ad+bc)i.
         * @param other The complex number to multiply by.
         * @return A new Complex object representing the product.
         */
        [[nodiscard]] Complex operator*(const Complex &other) const noexcept;

    private:
        double real_{0.0};
        double imaginary_{0.0};
    };
}

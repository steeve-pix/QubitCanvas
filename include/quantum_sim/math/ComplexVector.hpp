#pragma once
#include "quantum_sim/math/Complex.hpp"

#include <cstddef>
#include <vector>

namespace quantum_sim::math {
    /**
     * One-dimensional complex vector used to store quantum amplitudes.
     */
    class ComplexVector final {
    public:
        /**
         * Creates a vector from the provided complex values.
         *
         * @param values Elements stored in order.
         */
        explicit ComplexVector(std::vector<Complex> values);

        /**
         * @return Number of complex values in the vector.
         */
        [[nodiscard]] std::size_t size() const noexcept;

        /**
         * Reads one element with bounds checking.
         *
         * @param index Element index.
         * @return Complex value at index.
         * @throws std::out_of_range if index is outside the vector.
         */
        [[nodiscard]] const Complex &at(std::size_t index) const;

        /**
         * @return Sum of squared magnitudes for all elements.
         */
        [[nodiscard]] double magnitudeSquared() const noexcept;

        /**
         * Checks whether the vector has unit length within a tolerance.
         *
         * @param epsilon Accepted absolute error around 1.0.
         * @return True when magnitudeSquared() is close to 1.0.
         */
        [[nodiscard]] bool isNormalized(double epsilon = 1e-9) const noexcept;

        /**
         * @return A unit-length copy of this vector.
         * @throws std::invalid_argument if the vector has zero magnitude.
         */
        [[nodiscard]] ComplexVector normalized() const;

        /**
         * Computes the quantum inner product ⟨this|other⟩.
         *
         * @param other Vector with the same size.
         * @return Complex inner product.
         * @throws std::invalid_argument if the vectors have different sizes.
         */
        [[nodiscard]] Complex innerProduct(const ComplexVector &other) const;

        /**
         * Adds two vectors element by element.
         *
         * @param other Vector with the same size.
         * @return Element-wise sum.
         * @throws std::invalid_argument if the vectors have different sizes.
         */
        [[nodiscard]] ComplexVector operator+(const ComplexVector &other) const;

        /**
         * Multiplies each element by a scalar.
         *
         * @param scalar Complex multiplier.
         * @return Scaled vector.
         */
        [[nodiscard]] ComplexVector operator*(const Complex &scalar) const;

        /**
         * Computes the tensor product of two vectors.
         *
         * @param other Right-hand vector in the Kronecker product.
         * @return Combined vector with size() * other.size() elements.
         */
        [[nodiscard]] ComplexVector tensorProduct(const ComplexVector &other) const;

    private:
        std::vector<Complex> values_;
    };
}

#pragma once
#include "quantum_sim/math/Complex.hpp"

#include <vector>
#include <cstddef>

namespace quantum_sim::math {
    /**
     * Represents a mathematical vector whose elements are complex numbers.
     *
     * Provides operations for manipulating and performing calculations on vectors
     * consisting of complex numbers.
     */
    class ComplexVector final {
        /**
         * Constructs a ComplexVector with the specified values.
         *
         * @param values The vector of Complex numbers used to initialize the ComplexVector.
         */
    public:
        explicit ComplexVector(std::vector<Complex> values);

        /**
         * Returns the number of elements in the collection.
         *
         * This method provides the total count of elements currently stored,
         * allowing operations to be performed based on the collection size.
         *
         * @return The count of elements in the collection.
         */
        [[nodiscard]] std::size_t size() const noexcept;

        /**
         * Represents a specific point or position in time.
         *
         * Provides functionality to store and manage a precise moment
         * in time, which can be used in scheduling or other time-related computations.
         */
        [[nodiscard]] const Complex &at(std::size_t index) const;

        /**
         * Computes the squared magnitude of the vector.
         *
         * The squared magnitude is calculated as the sum of the squared magnitudes
         * of all complex elements in the vector.
         *
         * @return The squared magnitude of the vector as a double.
         */
        [[nodiscard]] double magnitudeSquared() const noexcept;

        /**
         * Checks whether the vector is normalized within a specified tolerance.
         *
         * A vector is considered normalized if its magnitude squared is close to 1,
         * within the specified epsilon threshold.
         *
         * @param epsilon The tolerance value within which the vector is considered normalized.
         * @return True if the vector is normalized within the given epsilon, false otherwise.
         */
        [[nodiscard]] bool isNormalized(double epsilon = 1e-9) const noexcept;

        /**
         * Computes the normalized version of the given input.
         *
         * This method transforms the input value or structure into a normalized form,
         * typically scaling or adjusting it so that its magnitude or sum is equal to 1.
         *
         * @return The normalized result.
         */
        [[nodiscard]] ComplexVector normalized() const;

        /**
         * Computes the inner product of the current vector with another ComplexVector.
         *
         * The inner product is calculated as the sum of the element-wise products of the
         * conjugate of each element in the current vector with the corresponding element
         * in the other vector. The two vectors must have the same size; otherwise, an
         * exception is thrown.
         *
         * @param other The other ComplexVector to compute the inner product with.
         * @return The complex inner product of the two vectors as a Complex number.
         * @throws std::invalid_argument If the two vectors do not have the same size.
         */
        [[nodiscard]] Complex innerProduct(const ComplexVector &other) const;

        /**
         * Overloads an operator to define custom behavior.
         *
         * Enables a specific functionality or behavior for the operator when
         * used with the corresponding object or data type.
         *
         * @param lhs The left-hand side operand involved in the operation.
         * @param rhs The right-hand side operand involved in the operation.
         * @return The result of the operation, determined by the specific operator overload implementation.
         */
        [[nodiscard]] ComplexVector operator+(const ComplexVector &other) const;

        /**
         * Overloads an operator to provide custom functionality for a specific operation.
         *
         * Facilitates performing the defined operation between objects of the class.
         *
         * @param rhs The right-hand side operand involved in the operation.
         * @return The result of the operator operation.
         */
        [[nodiscard]] ComplexVector operator*(const Complex &scalar) const;

        /**
         * Computes the tensor product of this complex vector with another complex vector.
         *
         * The tensor product is a mathematical operation that combines two vectors into a single vector
         * with a size equal to the product of the sizes of the input vectors. Each element of the resulting
         * vector is computed as the product of an element in the first vector and an element in the second vector.
         *
         * @param other The complex vector to combine with this vector.
         * @return A new complex vector representing the tensor product of the two vectors.
         */
        [[nodiscard]] ComplexVector tensorProduct(const ComplexVector &other) const;

        /**
         * Stores a collection of complex numbers representing the elements of the vector.
         *
         * This member variable serves as the underlying storage for the ComplexVector class,
         * enabling operations and manipulations on a sequence of complex numbers.
         */
    private:
        std::vector<Complex> values_;
    };
}

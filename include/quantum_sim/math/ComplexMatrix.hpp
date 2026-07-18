#pragma once
#include "quantum_sim/math/Complex.hpp"
#include "quantum_sim/math/ComplexVector.hpp"

#include <cstddef>
#include <vector>

namespace quantum_sim::math {
    /**
     * @class ComplexMatrix
     * @brief Represents a matrix containing complex numbers.
     *
     * The ComplexMatrix class provides functionalities to perform operations
     * on matrices with elements that are complex numbers. It supports basic
     * matrix operations such as addition, subtraction, multiplication,
     * and scalar operations as well as specialized operations like conjugate and transpose.
     *
     * The class ensures matrix validity and provides methods to manage the
     * dimensions and content of the matrix.
     *
     * Usage of this class assumes that the user understands matrix algebra
     * and the concept of complex numbers.
     */
    class ComplexMatrix final {
        /**
         * @brief Constructs a ComplexMatrix object with specified dimensions and values.
         *
         * The constructor initializes a matrix with the given number of rows and columns,
         * and sets its elements using the provided vector of complex numbers.
         * An exception is thrown if the number of elements in the vector does not
         * match the product of rows and columns, ensuring proper matrix dimensions.
         *
         * @param rows The number of rows in the matrix.
         * @param columns The number of columns in the matrix.
         * @param values A vector containing complex numbers to populate the matrix.
         *               The size of the vector must equal rows * columns.
         * @throws std::invalid_argument If the size of the values vector does not
         *         match rows * columns.
         */
    public:
        ComplexMatrix(std::size_t rows, std::size_t columns, std::vector<Complex> values);

        /**
         * @brief Retrieves the number of rows in the matrix.
         *
         * This method provides the total count of rows in the ComplexMatrix.
         * The value corresponds to the size specified during the construction
         * of the matrix.
         *
         * @return The number of rows in the matrix.
         */
        [[nodiscard]] std::size_t rows() const noexcept;

        /**
         * @brief Retrieves the number of columns in the matrix.
         *
         * This method returns the number of columns in the ComplexMatrix instance.
         * It can be used to determine the structure of the matrix, especially when
         * performing operations that depend on the matrix's dimensions.
         *
         * @return The number of columns in the matrix.
         */
        [[nodiscard]] std::size_t columns() const noexcept;

        /**
         * @class at
         * @brief Provides access to elements of a data container.
         *
         * The `at` method is used to access elements in a container at a specified index
         * with bounds checking. If the index is out of range, the method typically throws
         * an exception, ensuring safer access compared to direct indexing methods.
         *
         * This method is commonly used when controlled error handling is required while
         * accessing elements in various types of containers.
         *
         * @param index The position of the element to access in the container.
         *        The value of `index` must be within the valid range of the container,
         *        otherwise an exception is thrown.
         * @return A reference to the element at the specified position in the container.
         *         For non-const containers, this reference allows modification of the element.
         */
        [[nodiscard]] const Complex &at(std::size_t row, std::size_t column) const;

        /**
         * @brief Computes the conjugate transpose of the current matrix.
         *
         * This method calculates the conjugate transpose (also known as the Hermitian transpose)
         * of the matrix. It transposes the matrix by swapping rows and columns, and then computes
         * the complex conjugate for each element.
         *
         * The resulting matrix will have its rows and columns swapped compared to the original matrix,
         * with each element replaced by its conjugate.
         *
         * @return A new ComplexMatrix instance representing the conjugate transpose of the original matrix.
         */
        [[nodiscard]] ComplexMatrix conjugateTranspose() const;

        /**
         * @brief Creates an identity matrix of the specified size.
         *
         * This method generates a square identity matrix of the given dimensions,
         * where the diagonal elements are set to 1 (represented as complex numbers
         * with real part 1 and imaginary part 0) and all other elements are set to 0.
         *
         * @param size The size of the square matrix (number of rows and columns).
         *             Must be a positive integer.
         * @return A ComplexMatrix instance representing the identity matrix.
         */
        [[nodiscard]] static ComplexMatrix identity(std::size_t size);

        [[nodiscard]] bool isUnitary(double epsilon = 1e-9) const;

        /**
         * @brief Overloads an operator to perform a specific operation.
         *
         * The operator function provides functionality for customized behavior
         * when performing operations involving objects of its class. This allows
         * instances of the class to be used in a natural and intuitive way
         * in operations like arithmetic, comparison, or logical expressions.
         *
         * Usage of the operator assumes familiarity with the semantics of the
         * overloaded operation.
         *
         * @param lhs The left-hand side operand of the operation.
         * @param rhs The right-hand side operand of the operation.
         * @return The result of the operator execution as an appropriate type.
         */
        [[nodiscard]] ComplexVector operator*(const ComplexVector &vector) const;

        /**
         * @brief Multiplies the current matrix with another matrix.
         *
         * This operator computes the matrix product of the current matrix and
         * the provided matrix. The number of columns in the current matrix must
         * match the number of rows in the provided matrix. The resulting matrix
         * will have dimensions matching the number of rows in the current matrix
         * and the number of columns in the provided matrix.
         *
         * @param matrix The matrix to multiply with the current matrix.
         * @return A new ComplexMatrix representing the result of the matrix multiplication.
         *
         * @throws std::invalid_argument Thrown when the number of columns in the current matrix
         * does not match the number of rows in the provided matrix.
         */
        [[nodiscard]] ComplexMatrix operator*(const ComplexMatrix &matrix) const;

    private:
        std::size_t rows_;
        /**
         * @var ComplexMatrix::columns_
         * @brief Stores the number of columns in the matrix.
         *
         * This member variable represents the total number of columns in a ComplexMatrix instance,
         * defining the horizontal dimension of the matrix. It is initialized during the construction
         * of the ComplexMatrix and is used throughout the class to validate operations and manage
         * the structure of the matrix. The value of this variable remains constant unless explicitly
         * modified by operations that resize or redefine the matrix.
         */
        std::size_t columns_;
        /**
         * @variable values_
         * @brief Stores the elements of the matrix as a vector of complex numbers.
         *
         * The `values_` variable is a private member of the ComplexMatrix class that contains
         * all the elements of the matrix, stored in a single-dimensional std::vector. The elements
         * are organized in row-major order, where consecutive elements in the vector represent
         * entries of the matrix progressing row by row.
         *
         * Access to this variable is managed through public methods of the ComplexMatrix class
         * to ensure proper encapsulation and integrity of the matrix structure. It is initialized
         * during matrix construction and its size must match the product of the matrix's rows
         * and columns.
         */
        std::vector<Complex> values_;
    };
}

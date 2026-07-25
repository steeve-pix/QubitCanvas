#pragma once
#include "quantum_sim/math/Complex.hpp"
#include "quantum_sim/math/ComplexVector.hpp"

#include <cstddef>
#include <vector>

namespace quantum_sim::math {
    /**
     * Row-major complex matrix used for quantum gate operations.
     */
    class ComplexMatrix final {
    public:
        /**
         * Creates a matrix from row-major values.
         *
         * @param rows Row count.
         * @param columns Column count.
         * @param values Matrix values in row-major order.
         * @throws std::invalid_argument if values.size() != rows * columns.
         */
        ComplexMatrix(std::size_t rows, std::size_t columns, std::vector<Complex> values);

        /**
         * @return Matrix row count.
         */
        [[nodiscard]] std::size_t rows() const noexcept;

        /**
         * @return Matrix column count.
         */
        [[nodiscard]] std::size_t columns() const noexcept;

        /**
         * Reads a matrix entry with bounds checking.
         *
         * @param row Zero-based row index.
         * @param column Zero-based column index.
         * @return Matrix value at row, column.
         * @throws std::out_of_range if the position is outside the matrix.
         */
        [[nodiscard]] const Complex &at(std::size_t row, std::size_t column) const;

        /**
         * @return Conjugate transpose of this matrix.
         */
        [[nodiscard]] ComplexMatrix conjugateTranspose() const;

        /**
         * Creates a square identity matrix.
         *
         * @param size Number of rows and columns.
         * @return size by size identity matrix.
         */
        [[nodiscard]] static ComplexMatrix identity(std::size_t size);

        /**
         * Checks whether this matrix is unitary.
         *
         * @param epsilon Accepted absolute error per matrix entry.
         * @return True when conjugateTranspose() * this is approximately identity.
         */
        [[nodiscard]] bool isUnitary(double epsilon = 1e-9) const;

        /**
         * Computes the Kronecker product of this matrix and another matrix.
         *
         * @param other Right-hand matrix.
         * @return Tensor-product matrix.
         */
        [[nodiscard]] ComplexMatrix tensorProduct(const ComplexMatrix &other) const;

        /**
         * Multiplies this matrix by a vector.
         *
         * @param vector Vector whose size must equal columns().
         * @return Matrix-vector product.
         * @throws std::invalid_argument if dimensions do not match.
         */
        [[nodiscard]] ComplexVector operator*(const ComplexVector &vector) const;

        /**
         * Multiplies this matrix by another matrix.
         *
         * @param matrix Matrix whose rows() must equal this columns().
         * @return Matrix product.
         * @throws std::invalid_argument if dimensions do not match.
         */
        [[nodiscard]] ComplexMatrix operator*(const ComplexMatrix &matrix) const;

    private:
        std::size_t rows_;
        std::size_t columns_;
        std::vector<Complex> values_;
    };
}

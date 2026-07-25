#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/math/ComplexVector.hpp"

#include <stdexcept>
#include <utility>
#include <cmath>

namespace quantum_sim::math {
    ComplexMatrix::ComplexMatrix(std::size_t rows, std::size_t columns, std::vector<Complex> values)
        : rows_{rows}, columns_{columns}, values_{std::move(values)} {
        if (rows_ * columns_ != values_.size()) {
            throw std::invalid_argument{
                "Matrix dimensions do not match the number of values."
            };
        }
    }

    std::size_t ComplexMatrix::rows() const noexcept {
        return rows_;
    }

    std::size_t ComplexMatrix::columns() const noexcept {
        return columns_;
    }

    const Complex &ComplexMatrix::at(std::size_t row, std::size_t column) const {
        if (row >= rows_ || column >= columns_) {
            throw std::out_of_range{"Matrix position is outside its dimensions."};
        }

        // Values are row-major: row stride is the matrix column count.
        const std::size_t index = row * columns_ + column;
        return values_[index];
    }

    ComplexMatrix ComplexMatrix::conjugateTranspose() const {
        std::vector result(rows() * columns(), Complex{});

        for (std::size_t newRow{}; newRow < rows_; ++newRow) {
            for (std::size_t newColumn{}; newColumn < columns_; ++newColumn) {
                // Swap row/column and conjugate the value to build A*.
                const std::size_t outputIndex = newColumn * rows() + newRow;

                result[outputIndex] = at(newRow, newColumn).conjugate();
            }
        }
        return ComplexMatrix{columns(), rows(), std::move(result)}; // NOLINT
    }

    ComplexMatrix ComplexMatrix::identity(std::size_t size) {
        ComplexMatrix matrix{size, size, std::move(std::vector(size * size, Complex{}))};

        for (std::size_t i{}; i < size; ++i) {
            matrix.values_[i * size + i] = Complex{1.0, 0.0};
        }

        return matrix;
    }

    bool ComplexMatrix::isUnitary(double epsilon) const {
        if (rows_ != columns_) return false;

        // A matrix is unitary when A* A equals identity within tolerance.
        const ComplexMatrix product = conjugateTranspose() * (*this);
        const ComplexMatrix expected = identity(rows());

        for (std::size_t row{}; row < rows_; ++row) {
            for (std::size_t column{}; column < columns_; ++column) {
                const Complex &actual = product.at(row, column);
                const Complex &target = expected.at(row, column);

                const bool realMatches = std::abs(actual.real() - target.real()) <= epsilon;
                const bool imaginaryMatches = std::abs(actual.imaginary() - target.imaginary()) <= epsilon;;

                if (!realMatches || !imaginaryMatches) {
                    return false;
                }
            }
        }
        return true;
    }

    ComplexMatrix ComplexMatrix::tensorProduct(const ComplexMatrix &other) const {
        const std::size_t outputRows = rows_ * other.rows_;
        const std::size_t outputColumns = columns_ * other.columns_;

        std::vector result(outputRows * outputColumns, Complex{});

        // Each source entry expands into a scaled copy of the right-hand matrix.
        for (std::size_t leftRow{}; leftRow < rows_; ++leftRow) {
            for (std::size_t rightRow{}; rightRow < other.rows_; ++rightRow) {
                for (std::size_t leftColumn{}; leftColumn < columns_; ++leftColumn) {
                    for (std::size_t rightColumn{}; rightColumn < other.columns_; ++rightColumn) {
                        const std::size_t outputRow = leftRow * other.rows_ + rightRow;
                        const std::size_t outputColumn = leftColumn * other.columns_ + rightColumn;

                        const std::size_t outputIndex =
                                outputRow * outputColumns + outputColumn;

                        result[outputIndex] = at(leftRow, leftColumn) * other.at(rightRow, rightColumn);
                    }
                }
            }
        }

        return ComplexMatrix{outputRows, outputColumns, std::move(result)};
    }

    ComplexVector ComplexMatrix::operator*(const ComplexVector &vector) const {
        if (columns_ != vector.size()) {
            throw std::invalid_argument{"Matrix column count must equal vector size."};
        }

        std::vector<Complex> output;
        output.reserve(rows());

        for (std::size_t row{}; row < rows_; ++row) {
            Complex sum{};

            // Dot product between this matrix row and the input vector.
            for (std::size_t column{}; column < columns_; ++column) {
                sum += this->at(row, column) * vector.at(column);
            }
            output.push_back(sum);
        }

        return ComplexVector{std::move(output)};
    }

    ComplexMatrix ComplexMatrix::operator*(const ComplexMatrix &matrix) const {
        if (columns_ != matrix.rows_) {
            throw std::invalid_argument{"Matrix column count must match other matrix row count"};
        }

        std::vector<Complex> output;
        for (std::size_t row{}; row < rows_; ++row) {
            for (std::size_t column{}; column < matrix.columns_; ++column) {
                Complex sum{};

                // Dot product of a row from the left matrix and a column from the right.
                for (std::size_t position{}; position < columns_; ++position) {
                    sum += this->at(row, position) * matrix.at(position, column);
                }
                output.push_back(sum);
            }
        }

        return ComplexMatrix(rows_, matrix.columns_, std::move(output));
    }
}

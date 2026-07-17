#include "quantum_sim/math/ComplexMatrix.hpp"

#include <stdexcept>
#include <utility>

#include "quantum_sim/math/ComplexVector.hpp"

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

        const std::size_t index = row * columns_ + column;
        return values_[index];
    }

    ComplexVector ComplexMatrix::operator*(const ComplexVector &vector) const {
        if (columns() != vector.size()) {
            throw std::invalid_argument{"Matrix cannot multiply"};
        }

        std::vector<Complex> output;
        output.reserve(rows());

        for (std::size_t row{}; row < rows_; ++row) {
            Complex sum{};

            for (std::size_t column{}; column < columns_; ++column) {
                sum += this->at(row, column) * vector.at(column);
            }
            output.push_back(sum);
        }

        return ComplexVector{std::move(output)};
    }
}

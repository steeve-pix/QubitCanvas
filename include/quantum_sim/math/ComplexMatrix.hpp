#pragma once
#include "quantum_sim/math/Complex.hpp"

#include <cstddef>
#include <vector>


namespace quantum_sim::math {
    class ComplexMatrix final {
    public:
        ComplexMatrix(std::size_t rows, std::size_t columns, std::vector<Complex> values);

        [[nodiscard]] std::size_t rows() const noexcept;

        [[nodiscard]] std::size_t columns() const noexcept;

        [[nodiscard]] const Complex &at(std::size_t row, std::size_t column) const;

    private:
        std::size_t rows_;
        std::size_t columns_;
        std::vector<Complex> values_;
    };
}

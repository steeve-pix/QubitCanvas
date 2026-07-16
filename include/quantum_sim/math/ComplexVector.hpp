#pragma once
#include "Complex.hpp"

#include <vector>
#include <cstddef>

namespace quantum_sim::math {
    class ComplexVector final {
    public:
        explicit ComplexVector(std::vector<Complex> values);

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] const Complex &at(std::size_t index) const;

    private:
        std::vector<Complex> value_;
    };
}

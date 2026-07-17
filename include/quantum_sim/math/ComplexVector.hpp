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

        [[nodiscard]] double magnitudeSquared() const noexcept;

        [[nodiscard]] bool isNormalized(double epsilon = 1e-9) const noexcept;

        [[nodiscard]] ComplexVector normalized() const;

        [[nodiscard]] ComplexVector operator+(const ComplexVector &other) const;

        [[nodiscard]] ComplexVector operator*(const Complex &scalar) const;

    private:
        std::vector<Complex> values_;
    };
}

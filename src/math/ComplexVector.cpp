#include "quantum_sim/math/ComplexVector.hpp"

#include <utility>
#include <cmath>

namespace quantum_sim::math {
    ComplexVector::ComplexVector(std::vector<Complex> values)
        : values_{std::move(values)} {
    }

    std::size_t ComplexVector::size() const noexcept {
        return values_.size();
    }

    const Complex &ComplexVector::at(std::size_t index) const {
        return values_.at(index);
    }

    double ComplexVector::magnitudeSquared() const noexcept {
        double total{0.0};

        for (const Complex &value: values_) {
            total += value.magnitudeSquared();
        }

        return total;
    }

    bool ComplexVector::isNormalized() const noexcept {
        double epsilon = 1e-9;
        return std::abs(magnitudeSquared() - 1.0) <= epsilon;
    }
}

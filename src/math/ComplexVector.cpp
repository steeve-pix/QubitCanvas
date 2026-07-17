#include "quantum_sim/math/ComplexVector.hpp"

#include <utility>
#include <cmath>
#include <stdexcept>

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

    ComplexVector ComplexVector::normalized() const {
        const double magnitude = std::sqrt(magnitudeSquared());

        if (magnitude <= 1e-9) throw std::domain_error{"Cannot divide by zero"};

        std::vector<Complex> normalizedValues;
        normalizedValues.reserve(values_.size());
        for (const Complex &value: values_) {
            normalizedValues.push_back(value / magnitude);
        }

        return ComplexVector{std::move(normalizedValues)};
    }

    ComplexVector ComplexVector::operator+(const ComplexVector &other) const {
        bool isEqualSize = values_.size() == other.size();
        if (!isEqualSize) throw std::invalid_argument{"Cannot add complex vectors with different sizes."};

        std::vector<Complex> result;
        result.reserve(values_.size());
        for (int i{}; i < values_.size(); ++i) {
            result.push_back(values_.at(i) + other.values_.at(i));
        }

        return ComplexVector{result};
    }
}

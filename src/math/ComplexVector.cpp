#include "quantum_sim/math/ComplexVector.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

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

        // Quantum-state normalization uses the sum of all amplitude magnitudes.
        for (const Complex &value: values_) {
            total += value.magnitudeSquared();
        }

        return total;
    }

    bool ComplexVector::isNormalized(double epsilon) const noexcept {
        return std::abs(magnitudeSquared() - 1.0) <= epsilon;
    }

    ComplexVector ComplexVector::normalized() const {
        const double magnitude = std::sqrt(magnitudeSquared());

        if (magnitude == 0.0) throw std::domain_error{"Cannot divide by zero"};

        std::vector<Complex> normalizedValues;
        normalizedValues.reserve(values_.size());

        // Preserve direction while scaling the vector to unit magnitude.
        for (const Complex &value: values_) {
            normalizedValues.push_back(value / magnitude);
        }

        return ComplexVector{std::move(normalizedValues)};
    }

    Complex ComplexVector::innerProduct(const ComplexVector &other) const {
        if (size() != other.size())
            throw std::invalid_argument{"Cannot compute the inner product of vectors with different sizes."};


        Complex result{};

        // Quantum inner products conjugate the left-hand vector.
        for (std::size_t i = 0; i < size(); ++i) {
            result += values_[i].conjugate() * other.values_[i];
        }

        return result;
    }

    ComplexVector ComplexVector::operator+(const ComplexVector &other) const {
        if (size() != other.size())
            throw std::invalid_argument{"Cannot add complex vectors with different sizes."};

        std::vector<Complex> result;
        result.reserve(size());

        for (std::size_t i{}; i < size(); ++i) {
            result.push_back(values_[i] + other.values_[i]);
        }

        return ComplexVector{std::move(result)};
    }

    ComplexVector ComplexVector::operator*(const Complex &scalar) const {
        std::vector<Complex> result;
        result.reserve(size());

        for (const Complex &value: values_) {
            result.push_back(value * scalar);
        }

        return ComplexVector{std::move(result)};
    }

    ComplexVector ComplexVector::tensorProduct(const ComplexVector &other) const {
        std::vector<Complex> result;
        result.reserve(this->size() * other.size());

        // Kronecker product keeps the left vector as the outer loop so ordering
        // matches the matrix tensor product used by gate expansion.
        for (const Complex &left: values_) {
            for (const Complex &right: other.values_) {
                result.push_back(left * right);
            }
        }

        return ComplexVector{std::move(result)};
    }
}

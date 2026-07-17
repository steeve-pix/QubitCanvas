#include "quantum_sim/math/Complex.hpp"

#include <cmath>
#include <stdexcept>

namespace quantum_sim::math {
    Complex::Complex(double real, double imaginary)
        : real_(real), imaginary_(imaginary) {
    }

    double Complex::real() const noexcept {
        return real_;
    }

    double Complex::imaginary() const noexcept {
        return imaginary_;
    }

    Complex Complex::conjugate() const noexcept {
        return Complex{real_, -imaginary_};
    }

    double Complex::magnitudeSquared() const noexcept {
        return (real_ * real_) + (imaginary_ * imaginary_);
    }

    double Complex::magnitude() const noexcept {
        return std::sqrt(magnitudeSquared());
    }

    Complex Complex::operator+(const Complex &other) const noexcept {
        return Complex{
            real_ + other.real_,
            imaginary_ + other.imaginary_
        };
    }

    Complex Complex::operator*(const Complex &other) const noexcept {
        return Complex{
            (real_ * other.real_) - (imaginary_ * other.imaginary_),
            (real_ * other.imaginary_) + (imaginary_ * other.real_)
        };
    }

    Complex Complex::operator/(double scalar) const {
        if (scalar == 0) throw std::invalid_argument{"Cannot divide a complex number by zero"};
        return Complex{
            real_ / scalar,
            imaginary_ / scalar
        };
    }

    Complex &Complex::operator+=(const Complex &other) noexcept {
        this->real_ += other.real_;
        this->imaginary_ += other.imaginary_;

        return *this;
    }
}

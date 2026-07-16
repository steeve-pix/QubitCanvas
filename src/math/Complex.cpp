#include "quantum_sim/math/Complex.hpp"

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
}

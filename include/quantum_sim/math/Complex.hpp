#pragma once

namespace quantum_sim::math {
    class Complex final {
    public:
        explicit Complex(double real = 0.0, double imaginary = 0.0);

        [[nodiscard]] double real() const noexcept;

        [[nodiscard]] double imaginary() const noexcept;

        [[nodiscard]] Complex operator+(const Complex &other) const noexcept;
        [[nodiscard]]Complex operator*(const Complex&other) const noexcept;

    private:
        double real_{0.0};
        double imaginary_{0.0};
    };
}

#include <iostream>

#include "quantum_sim/math/ComplexVector.hpp"

namespace {
    using quantum_sim::math::ComplexVector;
    using quantum_sim::math::Complex;

    int failures = 0;

    [[nodiscard]] bool approximatelyEqual(
        double left,
        double right,
        double epsilon = 1e-9) noexcept {
        return std::abs(left - right) <= epsilon;
    }

    void check(bool condition, const char *testName) {
        if (!condition) {
            std::cerr << "FAILED: " << testName << '\n';
            ++failures;
        }
    }
} //

int main() {
    const ComplexVector x{
        std::vector{
            Complex{3.0, 4.0},
            Complex{0.0, 5.0}
        }
    };

    const ComplexVector y{
        std::vector{
            Complex{7.0, 6.0},
            Complex{10.0, 5.0}
        }
    };

    const auto z = x + y;

    std::cout << z.at(1).real();


    return 0;
}

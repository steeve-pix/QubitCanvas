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
    const ComplexVector vector{
        std::vector{
            Complex{1.0, 1.0},
            Complex{0.0, 2.0}
        }
    };

    const ComplexVector result = vector * Complex{0.0, 1.0};

    std::cout << result.at(0).imaginary();


    return 0;
}

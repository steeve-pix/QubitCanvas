#include "quantum_sim/math/Complex.hpp"

#include <cmath>
#include <iostream>

namespace {
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
}

int main() {
    using quantum_sim::math::Complex;

    const Complex left{2.0, 3.0};
    const Complex right{5.0, 4.0};
    const Complex sum = left + right;

    check(
        approximatelyEqual(sum.real(), 7.0),
        "addition calculates real component"
    );

    check(
        approximatelyEqual(sum.imaginary(), 7.0),
        "addition calculates imaginary component"
    );

    if (failures == 0) {
        std::cout << "All Complex tests passed.\n";
    } else {
        std::cerr << failures << " test(s) failed.\n";
    }

    return failures == 0 ? 0 : 1;
}

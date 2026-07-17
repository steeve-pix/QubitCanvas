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
        const ComplexVector zeroState{
            std::vector{
                Complex{1.0, 0.0},
                Complex{0.0, 0.0}
            }
        };

        const ComplexVector normalizedZero = zeroState.normalized();
        check(normalizedZero.isNormalized(), "normalizing an already normalized vector remains normalized");

    if (failures == 0) {
        std::cout << "All Complex tests passed.\n";
    } else {
        std::cerr << failures << " test(s) failed.\n";
    }

    return 0;
}

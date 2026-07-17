#include <iostream>

#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/math/ComplexVector.hpp"

namespace {
    using quantum_sim::math::ComplexVector;
    using quantum_sim::math::ComplexMatrix;
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
    const ComplexMatrix matrix{
        2, 3, std::vector{
            Complex{1, 0},
            Complex{2, 0},
            Complex{3, 0},

            Complex{4, 0},
            Complex{5, 0},
            Complex{6, 0},
        }
    };

    bool isInvalidRowThrow = false;
    try {
        static_cast<void>(matrix.at(3, 3));
    } catch (const std::out_of_range&) {
        isInvalidRowThrow = true;
    }

    

    return failures == 0 ? 0 : 1;
}

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
    const ComplexMatrix xGate0{
        2,
        2,
        std::vector<Complex>{
            Complex{0.0, 0.0},
            Complex{1.0, 0.0},
            Complex{1.0, 0.0},
            Complex{0.0, 0.0}
        }
    };

    const double amplitude = 1 / std::sqrt(2.0);
    const ComplexVector superposition{
        std::vector{
            Complex{amplitude, 0.0},
            Complex{amplitude, 0.0}
        }
    };

    const ComplexVector transformed  = xGate0 * superposition;

    check(transformed.isNormalized(), "X preserves state normalization");

    return failures == 0 ? 0 : 1;
}

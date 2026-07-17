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
        std::vector<Complex>{
            Complex{1.0, 0.0},
            Complex{0.0, 0.0}
        }
    };

    const ComplexVector oneState{
        std::vector<Complex>{
            Complex{0.0, 0.0},
            Complex{1.0, 0.0}
        }
    };

    const Complex orthogonalResult = zeroState.innerProduct(oneState);

    check(
        approximatelyEqual(orthogonalResult.real(), 0.0) &&
        approximatelyEqual(orthogonalResult.imaginary(), 0.0),
        "orthogonal states have inner product zero"
    );

    return failures == 0 ? 0 : 1;
}

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
    const ComplexMatrix gate{
        2, 2,
        std::vector{
            Complex{0.0, 0.0},
            Complex{0.0, -1.0},
            Complex{1.0, 2.0},
            Complex{2.0, 0.0}
        }
    };
    const auto transposed = gate.conjugateTranspose();

    check(
        approximatelyEqual(transposed.at(0, 0).real(), 0.0) &&
        approximatelyEqual(transposed.at(0, 0).imaginary(), 0.0),
        "conjugate transpose element (0, 0)");

    check(
        approximatelyEqual(transposed.at(0, 1).real(), 1.0) &&
        approximatelyEqual(transposed.at(0, 1).imaginary(), -2.0),
        "conjugate transpose element (0, 1)");

    check(
        approximatelyEqual(transposed.at(1, 0).real(), 0.0) &&
        approximatelyEqual(transposed.at(1, 0).imaginary(), 1.0),
        "conjugate transpose element (1, 0)");

    check(approximatelyEqual(transposed.at(1, 1).real(), 2.0) &&
          approximatelyEqual(transposed.at(1, 1).imaginary(), 0.0),
          "conjugate transpose element (1, 1)");

    if (failures == 0) {
        std::cout << "All conjugate-transpose tests passed.\n";
    }

    return failures == 0 ? 0 : 1;
}

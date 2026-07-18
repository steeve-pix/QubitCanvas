#include <iostream>
#include <numbers>
#include <cmath>

#include "quantum_sim/gates/SingleQubitGates.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/math/ComplexVector.hpp"
#include "quantum_sim/quantum/Qubit.hpp"

namespace {
    using quantum_sim::math::ComplexVector;
    using quantum_sim::math::ComplexMatrix;
    using quantum_sim::math::Complex;
    using quantum_sim::quantum::Quibit;

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
    const Quibit zero{
        Complex{1.0, 0.0},
        Complex{0.0, 0.0},
    };

    check(approximatelyEqual(zero.probabilityOfOne()+zero.probabilityOfZero(), 1.0),
          "");

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

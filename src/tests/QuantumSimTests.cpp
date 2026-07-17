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
    const ComplexMatrix xGate{
        2,
        2,
        std::vector<Complex>{
            Complex{0.0, 0.0},
            Complex{1.0, 0.0},
            Complex{1.0, 0.0},
            Complex{0.0, 0.0}
        }
    };

    const ComplexVector zeroState{
        std::vector<Complex>{
            Complex{1.0, 0.0},
            Complex{0.0, 0.0}
        }
    };

    const ComplexVector result = xGate * zeroState;
    const ComplexVector flippedOne = xGate * result;

    check(
        approximatelyEqual(flippedOne.at(0).real(), 1.0) &&
        approximatelyEqual(flippedOne.at(1).real(), 0.0),
        "X matrix transforms one state into zero state"
    );


    return failures == 0 ? 0 : 1;
}

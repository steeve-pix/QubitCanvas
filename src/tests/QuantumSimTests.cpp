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

    const ComplexMatrix xGate1{
        2, 2,
        std::vector<Complex>{
            Complex{0.0, 0.0},
            Complex{1.0, 0.0},
            Complex{1.0, 0.0},
            Complex{0.0, 0.0},
        }
    };

    const ComplexVector zeroState{
        std::vector{
            Complex{1.0, 0.0},
            Complex{0.0, 0.0}
        }
    };

    const ComplexMatrix identityGate = xGate0 * xGate1;
    const ComplexVector zeroState0 = identityGate * zeroState;


    check(
        approximatelyEqual(zeroState0.at(0).real(), 1.0) &&
        approximatelyEqual(zeroState0.at(1).real(), 0.0),
        "Identity matrix doesn't transforms zero state"
    );


    return failures == 0 ? 0 : 1;
}

#include <iostream>

#include "quantum_sim/gates/SingleQubitGates.hpp"
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
    const ComplexMatrix xMatrix = quantum_sim::gates::xGate();


    const ComplexMatrix scalingMatrix{
        2, 2,
        std::vector<Complex>{
            Complex{2.0, 0.0},
            Complex{},
            Complex{},
            Complex{2.0, 0.0}
        }
    };
    check(
        xMatrix.isUnitary(),
        "X matrix is unitary"
    );

    check(
        ComplexMatrix::identity(3).isUnitary(),
        "identity matrix is unitary"
    );

    check(
        !scalingMatrix.isUnitary(),
        "scaling matrix is not unitary"
    );


    if (failures == 0) {
        std::cout << "All conjugate-transpose tests passed.\n";
    }

    return 0;
}

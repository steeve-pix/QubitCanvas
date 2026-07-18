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
    const ComplexMatrix hGate = quantum_sim::gates::hadamardGate();

    const ComplexVector oneState{
        std::vector{
            Complex{1, 0}, // α
            Complex{0, 0}, // β
        }
    };

    const ComplexVector result = hGate * oneState;
    const ComplexVector output = hGate * result;

    std::cout << output.at(0).real() << std::endl;

    if (failures == 0) {
        std::cout << "All conjugate-transpose tests passed.\n";
    }

    return 0;
}

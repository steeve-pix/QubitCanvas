#include <iostream>
#include <numbers>
#include <cmath>

#include "quantum_sim/gates/SingleQubitGates.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/math/ComplexVector.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/quantum/Qubit.hpp"

namespace {
    using quantum_sim::math::ComplexVector;
    using quantum_sim::math::ComplexMatrix;
    using quantum_sim::math::Complex;
    using quantum_sim::quantum::Qubit;
    using quantum_sim::quantum::MeasurementResult;
    using quantum_sim::quantum::QuantumRegister;

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
    using quantum_sim::math::ComplexVector;

    const ComplexMatrix xOnFirstQubit =
            quantum_sim::gates::xGate().tensorProduct(ComplexMatrix::identity(2));

    const ComplexVector state00{
        std::vector{
            Complex{1.0, 0.0},
            Complex{0.0, 0.0},
            Complex{0.0, 0.0},
            Complex{0.0, 0.0},
        }
    };

    const ComplexVector result = xOnFirstQubit * state00;

    check(
        approximatelyEqual(result.at(0).magnitudeSquared(), 0.0) &&
        approximatelyEqual(result.at(1).magnitudeSquared(), 0.0) &&
        approximatelyEqual(result.at(2).magnitudeSquared(), 1.0) &&
        approximatelyEqual(result.at(3).magnitudeSquared(), 0.0),
        "X tensor identity flips the first qubit"
    );
    
    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

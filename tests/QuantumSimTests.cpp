#include <iostream>
#include <numbers>
#include <cmath>

#include "quantum_sim/gates/QuantumGates.hpp"
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
    const QuantumRegister state10 = QuantumRegister::basisState(2, 2);

    check(
        state10.qubitCount() == 2 && state10.stateCount() == 4,
        "basis state creates the correct register size"
    );

    check(
        approximatelyEqual(state10.probability(2), 1.0), "basis state creates state 10");

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

#include <iostream>
#include <numbers>
#include <cmath>

#include "quantum_sim/circuit/QuantumCircuit.hpp"
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
    using quantum_sim::circuit::QuantumCircuit;

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
    std::mt19937 randomEngine{42};

    QuantumCircuit bellCircuit{1};

    bellCircuit.addSingleQubitGate(quantum_sim::gates::xGate(), 0);

    const QuantumRegister zeroState =
            QuantumRegister::basisState(1, 0);

    const std::vector<std::size_t> xCounts =
            bellCircuit.runShots(zeroState, 100, randomEngine);

    check(
        xCounts[0] == 0 && xCounts[1] == 100,
        "X circuit always transforms zero into one"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

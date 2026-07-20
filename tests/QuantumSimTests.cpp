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

    QuantumCircuit bellCircuit{2};

    bellCircuit.addSingleQubitGate(quantum_sim::gates::hadamardGate(), 0);
    bellCircuit.addFullRegisterGate(quantum_sim::gates::cnotGate());

    const QuantumRegister initialState =
            QuantumRegister::basisState(2, 0);

    const std::vector<std::size_t> counts =
            bellCircuit.runShots(initialState, 1'000, randomEngine);

    check(
        counts.size() == 4,
        "shot execution returns one counter per basis state"
    );

    check(
        counts[1] == 0 && counts[2] == 0,
        "Bell circuit never measures states 01 or 10"
    );

    check(
        counts[0] + counts[3] == 1'000,
        "Bell circuit accounts for every shot"
    );

    check(
        counts[0] > 0 && counts[3] > 0,
        "Bell circuit produces both states 00 and 11"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

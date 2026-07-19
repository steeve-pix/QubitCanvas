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
    const QuantumRegister register00
    {
        2,
        ComplexVector{
            std::vector{
                Complex{1.0, 0.0},
                Complex{0.0, 0.0},
                Complex{0.0, 0.0},
                Complex{0.0, 0.0}
            }
        }
    };

    const QuantumRegister afterHadamard{
        register00.applySingleQubitGate(quantum_sim::gates::hadamardGate(), 0)
    };

    const QuantumRegister bellState{
        afterHadamard.applyGate(quantum_sim::gates::cnotGate())
    };

    std::mt19937 randomEngine{42};
    QuantumRegister measuredBellState = bellState;

    const std::size_t firstResult =
            measuredBellState.measure(randomEngine);

    const std::size_t secondResult =
      measuredBellState.measure(randomEngine);

    check(
        secondResult == firstResult,
        "repeated measurement returns the collapsed state"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

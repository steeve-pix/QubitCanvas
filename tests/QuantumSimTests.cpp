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
    const QuantumRegister register00{
        2,
        ComplexVector{
            std::vector{
                Complex{1.0, 0.0}, // |00⟩
                Complex{0.0, 0.0}, // |01⟩
                Complex{0.0, 0.0}, // |10⟩
                Complex{0.0, 0.0}, // |11⟩
            }
        }
    };

    const QuantumRegister flippedFirst =
            register00.applySingleQubitGate(quantum_sim::gates::xGate(), 0);

    check(
        approximatelyEqual(flippedFirst.amplitude(2).magnitudeSquared(), 1.0),
        "X on qubit zero transforms 00 into 10"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

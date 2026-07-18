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

    const QuantumRegister register01{
        2,
        ComplexVector{
            std::vector{
                Complex{0.0, 0.0}, // |00⟩
                Complex{1.0, 0.0}, // |01⟩
                Complex{0.0, 0.0}, // |10⟩
                Complex{0.0, 0.0}, // |11⟩
            }
        }
    };

    check(register01.qubitCount() == 2, "quantum register reports its qubit count");
    check(register01.stateCount() == 4, "two-qubit register contains four states");
    check(approximatelyEqual(register01.amplitude(1).magnitudeSquared(), 1.0),
          "register stores the amplitude for state 01");

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/gates/QuantumGates.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/math/ComplexVector.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/quantum/Qubit.hpp"
#include "quantum_sim/visualization/ConsoleVisualizer.hpp"
#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"

#include <sstream>
#include <iostream>
#include <numbers>

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
    const double amplitude =
            1.0 / std::sqrt(2.0);

    const QuantumRegister plusState{
        1,
        ComplexVector{
            std::vector{
                Complex{amplitude, 0.0},
                Complex{0.0, amplitude}
            }
        }
    };
    const quantum_sim::quantum::BlochAngles angles = plusState.blochAngles();

    check(
        approximatelyEqual(
            angles.theta,
            std::numbers::pi / 2.0
        ),
        "|+> has Bloch polar angle pi/2 radians"
    );

    check(
        approximatelyEqual(angles.phi, 0.0),
        "|+> has Bloch azimuth angle 0 radians"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

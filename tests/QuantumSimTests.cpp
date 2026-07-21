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
    const ComplexMatrix rxPi = quantum_sim::gates::rxGate(std::numbers::pi);
    const ComplexMatrix ryPi = quantum_sim::gates::ryGate(std::numbers::pi);
    const ComplexMatrix rzPi = quantum_sim::gates::rzGate(std::numbers::pi);
    const ComplexVector zeroState{
        std::vector{
            Complex{1.0, 0.0},
            Complex{0.0, 0.0},
        }
    };

    const ComplexVector resultRxPi = rxPi * zeroState;
    const ComplexVector resultRyPi = ryPi * zeroState;
    const ComplexVector resultRzPi = rzPi * zeroState;

    check(
        approximatelyEqual(resultRxPi.at(0).real(), 0.0) &&
        approximatelyEqual(resultRyPi.at(0).real(), 0.0) &&
        approximatelyEqual(resultRzPi.at(0).real(), 0.0) &&
        approximatelyEqual(resultRxPi.at(0).imaginary(), 0.0) &&
        approximatelyEqual(resultRyPi.at(0).imaginary(), 0.0)
        && approximatelyEqual(resultRzPi.at(0).imaginary(), -1.0),
        "Rx(pi) & Ry(pi) removes the |0> amplitude"
    );

    check(
        approximatelyEqual(resultRxPi.at(1).real(), 0.0) &&
        approximatelyEqual(resultRyPi.at(1).real(), 1.0) &&
        approximatelyEqual(resultRzPi.at(1).real(), 0.0) &&
        approximatelyEqual(resultRxPi.at(1).imaginary(), -1.0) &&
        approximatelyEqual(resultRyPi.at(1).imaginary(), 0.0)
        && approximatelyEqual(resultRzPi.at(1).imaginary(), 0.0),
        "Rx(pi) & Ry(pi) transforms |0> into -i|1>"
    );

    check(
        rxPi.isUnitary() && ryPi.isUnitary() && rzPi.isUnitary(),
        "Rx, Ry & Rz gate is unitary"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

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
    const QuantumCircuit rotationCircuit = quantum_sim::algorithms::rxRotationCircuit(std::numbers::pi / 2.0);

    const QuantumRegister initialState = QuantumRegister::basisState(1, 0);

    const QuantumRegister result = rotationCircuit.execute(initialState);

    check(
        quantum_sim::debug::gateExplanation("Rx").find(
            "X axis"
        ) != std::string::npos,
        "Rx explanation mentions the X axis"
    );

    check(
        quantum_sim::debug::gateExplanation("Ry").find(
            "Y axis"
        ) != std::string::npos,
        "Ry explanation mentions the Y axis"
    );

    check(
        quantum_sim::debug::gateExplanation("Rz").find(
            "phase"
        ) != std::string::npos,
        "Rz explanation mentions phase"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

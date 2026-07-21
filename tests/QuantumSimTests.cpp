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
    const QuantumCircuit ryRotationCircuit =
            quantum_sim::algorithms::ryRotationCircuit(
                std::numbers::pi
            );
    const QuantumRegister ryInitialState =
            QuantumRegister::basisState(1, 0);

    const QuantumRegister ryResult =
            ryRotationCircuit.execute(ryInitialState);

    check(
        ryRotationCircuit.instructionCount() == 1,
        "Ry rotation demonstration contains one instruction"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

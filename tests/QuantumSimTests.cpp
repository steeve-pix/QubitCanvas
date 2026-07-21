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
    QuantumCircuit circuit{1};
    circuit.addSingleQubitGate("Z", quantum_sim::gates::zGate(), 0);
    const QuantumRegister initialState = QuantumRegister::basisState(1, 1);
    const auto trace = circuit.executeWithTrace(initialState);

    std::ostringstream comparisonOutput;

    quantum_sim::visualization::printStateComparison(
        initialState,
        trace[0].state,
        comparisonOutput
    );

    const std::string comparison =
            comparisonOutput.str();

    check(
        comparison.find(
            "Phase: 0.00 rad -> 3.14 rad"
        ) != std::string::npos,
        "State comparison shows the phase transition in radians"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

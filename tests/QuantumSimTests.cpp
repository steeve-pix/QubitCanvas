#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/gates/QuantumGates.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/math/ComplexVector.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/quantum/Qubit.hpp"
#include "quantum_sim/visualization/ConsoleVisualizer.hpp"

#include <sstream>
#include <iostream>

#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"

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
    const QuantumRegister initialState = QuantumRegister::basisState(2, 0);

    const QuantumCircuit bellCircuit = quantum_sim::algorithms::bellStateCircuit();

    const QuantumRegister result = bellCircuit.execute(initialState);

    const std::vector<quantum_sim::circuit::CircuitInstructionInfo> instructions =
            bellCircuit.instructionInfo();

    check(
        bellCircuit.qubitCount() == 2,
        "Bell algorithm creates a two-qubit circuit"
    );

    check(
        bellCircuit.instructionCount() == 2,
        "Bell algorithm contains two instructions"
    );

    check(
        approximatelyEqual(result.probability(0), 0.5) &&
        approximatelyEqual(result.probability(3), 0.5),
        "Bell algorithm creates equal probabilities for 00 and 11"
    );

    check(
        approximatelyEqual(result.probability(1), 0.0) &&
        approximatelyEqual(result.probability(2), 0.0),
        "Bell algorithm removes probabilities for 01 and 10"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

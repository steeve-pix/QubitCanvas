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
    const QuantumRegister initialState = QuantumRegister::basisState(3, 0);

    const QuantumCircuit circuit = quantum_sim::algorithms::equalSuperpositionCircuit(3);

    const QuantumRegister result = circuit.execute(initialState);

    bool allProbabilitiesEqual = true;

    for (std::size_t state{}; state < result.stateCount(); ++state) {
        if (!approximatelyEqual(result.probability(state), 0.125)) {
            allProbabilitiesEqual = false;
            break;
        }
    }

    check(
        allProbabilitiesEqual,
        "Equal-superposition circuit distributes probability equally"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

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
    const QuantumCircuit ghzCircuit = quantum_sim::algorithms::ghzStateCircuit();
    const QuantumRegister initialState = QuantumRegister::basisState(3, 0);
    const QuantumRegister result = ghzCircuit.execute(initialState);

    check(
        approximatelyEqual(result.probability(0), 0.5),
        "GHZ algorithm gives |000> a probability of 50%"
    );

    check(
        approximatelyEqual(result.probability(7), 0.5),
        "GHZ algorithm gives |111> a probability of 50%"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

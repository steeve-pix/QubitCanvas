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

#include "quantum_sim/debug/DebuggerSession.hpp"

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
    // Regression coverage for debugger snapshots on a non-trivial entangling circuit.
    const QuantumCircuit circuit =
            quantum_sim::algorithms::ghzStateCircuit();

    const QuantumRegister initialState =
            QuantumRegister::basisState(3, 0);

    quantum_sim::debug::DebuggerSession session{
        circuit,
        initialState
    };

    session.restart();

    // After restart, the "before" state of the first step should be the original |000>.
    const quantum_sim::debug::DebuggerSnapshot snapshot =
            session.snapshot();

    check(
        approximatelyEqual(
            snapshot.beforeState.get().probability(0),
            1.0
        ),
        "Debugger snapshot exposes the state before the current instruction"
    );

    // The GUI default relies on this script to provide a rich density-history stack.
    const QuantumCircuit qftCircuit =
            quantum_sim::algorithms::qftCircuit(4);

    check(
        qftCircuit.qubitCount() == 4,
        "QFT showcase uses the requested qubit count"
    );

    check(
        qftCircuit.instructionCount() == 44,
        "QFT showcase keeps a 44-step render history"
    );

    const QuantumRegister qftFinalState =
            qftCircuit.execute(
                QuantumRegister::basisState(4, 0)
            );

    double totalProbability =
            0.0;

    for (std::size_t stateIndex = 0; stateIndex < qftFinalState.stateCount(); ++stateIndex) {
        totalProbability +=
                qftFinalState.probability(stateIndex);
    }

    check(
        approximatelyEqual(totalProbability, 1.0),
        "QFT showcase preserves total probability"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

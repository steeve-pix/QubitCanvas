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
#include <cmath>
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

    const double halfTurn =
            std::numbers::pi;

    const ComplexMatrix rx =
            quantum_sim::gates::rxGate(halfTurn);

    const ComplexMatrix ry =
            quantum_sim::gates::ryGate(halfTurn);

    const ComplexMatrix rz =
            quantum_sim::gates::rzGate(halfTurn);

    check(
        approximatelyEqual(rx.at(0, 0).real(), 0.0) &&
        approximatelyEqual(rx.at(0, 1).imaginary(), -1.0) &&
        approximatelyEqual(rx.at(1, 0).imaginary(), -1.0),
        "Rx(pi) uses the expected half-angle matrix"
    );

    check(
        approximatelyEqual(ry.at(0, 1).real(), -1.0) &&
        approximatelyEqual(ry.at(1, 0).real(), 1.0) &&
        approximatelyEqual(ry.at(1, 1).real(), 0.0),
        "Ry(pi) uses the expected half-angle matrix"
    );

    check(
        approximatelyEqual(rz.at(0, 0).imaginary(), -1.0) &&
        approximatelyEqual(rz.at(1, 1).imaginary(), 1.0) &&
        approximatelyEqual(rz.at(0, 1).magnitude(), 0.0),
        "Rz(pi) applies opposite diagonal phases"
    );

    const QuantumRegister rxResult =
            quantum_sim::algorithms::rxRotationCircuit(halfTurn).execute(
                QuantumRegister::basisState(1, 0)
            );

    const QuantumRegister ryResult =
            quantum_sim::algorithms::ryRotationCircuit(halfTurn).execute(
                QuantumRegister::basisState(1, 0)
            );

    check(
        approximatelyEqual(rxResult.probability(1), 1.0),
        "Rx(pi) rotates |0> to |1> up to phase"
    );

    check(
        approximatelyEqual(ryResult.probability(1), 1.0),
        "Ry(pi) rotates |0> to |1>"
    );

    const double metadataAngle =
            std::numbers::pi / 3.0;

    const auto rotationInstructions =
            quantum_sim::algorithms::rzRotationCircuit(metadataAngle).instructionInfo();

    check(
        rotationInstructions.size() == 1U &&
        rotationInstructions.front().angleRadians.has_value() &&
        approximatelyEqual(
            rotationInstructions.front().angleRadians.value(),
            metadataAngle
        ),
        "Rotation circuits retain their angle in instruction metadata"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

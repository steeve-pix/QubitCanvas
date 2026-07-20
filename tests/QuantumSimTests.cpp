#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/gates/QuantumGates.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/math/ComplexVector.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/quantum/Qubit.hpp"
#include "quantum_sim/visualization/ConsoleVisualizer.hpp"

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
    QuantumCircuit bellCircuit{2};


    bellCircuit.addSingleQubitGate(quantum_sim::gates::hadamardGate(), 0);
    bellCircuit.addFullRegisterGate(quantum_sim::gates::cnotGate());
    const QuantumRegister initialState = QuantumRegister::basisState(2, 0);


    const std::vector<quantum_sim::circuit::TraceStep> trace =
            bellCircuit.executeWithTrace(initialState);

    const QuantumRegister bellState = bellCircuit.execute(initialState);

    const std::vector<quantum_sim::quantum::StateInfo> allStates =
            bellState.states();

    std::ostringstream output;

    std::ostringstream shotOutput;

    const std::vector<std::size_t> counts{
        50,
        0,
        0,
        50
    };

    quantum_sim::visualization::printShotBars(bellState, counts, shotOutput, 100);

    quantum_sim::visualization::printProbabilityBars(bellState, output, 10);

    const std::string visualization =
            output.str();

    const std::string shotVisualization =
            shotOutput.str();

    check(
        trace.size() == 2,
        "execution trace contains one step per instruction"
    );

    check(
        approximatelyEqual(trace[0].state.probability(0), 0.5) &&
        approximatelyEqual(trace[0].state.probability(2), 0.5),
        "first trace step contains the state after Hadamard"
    );

    check(
        approximatelyEqual(trace[1].state.probability(0), 0.5) &&
        approximatelyEqual(trace[1].state.probability(3), 0.5),
        "second trace step contains the state after CNOT"
    );

    check(
        trace[0].description ==
        "Single-qubit gate on qubit 0",
        "trace describes the single-qubit instruction"
    );

    check(
        trace[1].description ==
        "Full-register gate",
        "trace describes the full-register instruction"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

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


    bellCircuit.addSingleQubitGate("H", quantum_sim::gates::hadamardGate(), 0);
    bellCircuit.addFullRegisterGate("CNOT", quantum_sim::gates::cnotGate());
    const QuantumRegister initialState = QuantumRegister::basisState(2, 0);

    std::ostringstream traceOutput;

    const std::vector<quantum_sim::circuit::TraceStep> trace =
            bellCircuit.executeWithTrace(initialState);

    quantum_sim::visualization::printExecutionTrace(initialState, trace, traceOutput);

    const std::string traceVisualization =
            traceOutput.str();

    const QuantumRegister bellState = bellCircuit.execute(initialState);

    check(
        traceVisualization.find("Initial state:")
        != std::string::npos,
        "trace visualizer displays the initial state"
    );

    check(
        traceVisualization.find("After H on qubit 0:")
        != std::string::npos,
        "trace visualizer displays the Hadamard step"
    );

    check(
        traceVisualization.find("After CNOT:")
        != std::string::npos,
        "trace visualizer displays the CNOT step"
    );

    check(
        traceVisualization.find("|00> [##########] 100.00%")
        != std::string::npos,
        "trace visualizer displays the initial 00 state"
    );

    check(
        traceVisualization.find("|10> [#####     ] 50.00%")
        != std::string::npos,
        "trace visualizer displays the state after Hadamard"
    );

    check(
        traceVisualization.find("|11> [#####     ] 50.00%")
        != std::string::npos,
        "trace visualizer displays the Bell state after CNOT"
    );

    std::cout << traceVisualization;
    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

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

    const QuantumRegister bellState = bellCircuit.execute(initialState);

    std::ostringstream diagramOutput;

    quantum_sim::visualization::printCircuitDiagram(
        bellCircuit,
        diagramOutput
    );

    const std::string diagram =
            diagramOutput.str();

    check(
    diagram.find("q0: |0> --H----CNOT--")
        != std::string::npos,
    "circuit diagram displays Hadamard on qubit zero"
);

    check(
        diagram.find("q1: |0> -------CNOT--")
            != std::string::npos,
        "circuit diagram displays CNOT on the second qubit line"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

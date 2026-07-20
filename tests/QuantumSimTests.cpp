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
    bellCircuit.addControlledGate("CNOT", quantum_sim::gates::cnotGate(), 0, 1);

    const QuantumRegister initialState = QuantumRegister::basisState(2, 0);

    const QuantumRegister bellState = bellCircuit.execute(initialState);

    const std::vector<quantum_sim::circuit::CircuitInstructionInfo> instructions =
            bellCircuit.instructionInfo();

    check(
        instructions[1].name == "CNOT" &&
        instructions[1].kind ==
        quantum_sim::circuit::CircuitInstructionKind::FullRegister &&
        !instructions[1].targetQubit.has_value() &&
        instructions[1].controlQubit.has_value() &&
        instructions[1].controlQubit.value() == 0 &&
        instructions[1].secondaryTargetQubit.has_value() &&
        instructions[1].secondaryTargetQubit.value() == 1,
        "circuit describes the CNOT control and target"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

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

    std::vector<quantum_sim::circuit::CircuitInstructionInfo> instructions =
            bellCircuit.instructionInfo();

    check(
        instructions.size() == 2,
        "circuit exposes one description per instruction"
    );
    check(
        instructions[0].name == "H" &&
        instructions[0].kind ==
        quantum_sim::circuit::CircuitInstructionKind::SingleQubit &&
        instructions[0].targetQubit.has_value() &&
        instructions[0].targetQubit.value() == 0,
        "circuit describes the Hadamard instruction"
    );

    check(
        instructions[1].name == "CNOT" &&
        instructions[1].kind ==
        quantum_sim::circuit::CircuitInstructionKind::FullRegister &&
        !instructions[1].targetQubit.has_value(),
        "circuit describes the CNOT instruction"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

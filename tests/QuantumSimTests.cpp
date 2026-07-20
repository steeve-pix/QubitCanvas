#include <iostream>
#include <numbers>
#include <cmath>

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/gates/QuantumGates.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/math/ComplexVector.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/quantum/Qubit.hpp"

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
    QuantumCircuit circuit{2};

    const QuantumRegister initialState = QuantumRegister::basisState(2, 0);

    circuit.addSingleQubitGate(quantum_sim::gates::hadamardGate(), 0);
    circuit.addFullRegisterGate(quantum_sim::gates::cnotGate());

    const QuantumRegister bellState = circuit.execute(initialState);

    const quantum_sim::quantum::StateInfo info00 = bellState.stateInfo(0);

    check(
        info00.label == "|00>",
        "state info contains the basis state label"
    );

    check(
        approximatelyEqual(info00.probability, 0.5),
        "state info contains the basis state probability"
    );

    const double expectedAmplitude =
            1.0 / std::sqrt(2.0);

    check(
        approximatelyEqual(
            info00.amplitude.real(),
            expectedAmplitude
        ) &&
        approximatelyEqual(
            info00.amplitude.imaginary(),
            0.0
        ),
        "state info contains the complex amplitude"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

#include <iostream>

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

    const std::vector<quantum_sim::quantum::StateInfo> allStates =
            bellState.states();

    check(allStates.size() == 4,
          "state inspection returns every basis state"
    );

    check(
        allStates[0].label == "|00>" &&
        allStates[1].label == "|01>" &&
        allStates[2].label == "|10>" &&
        allStates[3].label == "|11>",
        "state inspection preserves basis state order"
    );

    check(
        approximatelyEqual(allStates[0].probability, 0.5) &&
        approximatelyEqual(allStates[1].probability, 0.0) &&
        approximatelyEqual(allStates[2].probability, 0.0) &&
        approximatelyEqual(allStates[3].probability, 0.5),
        "state inspection returns Bell state probabilities"
    );


    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

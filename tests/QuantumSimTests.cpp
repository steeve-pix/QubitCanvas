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
    QuantumCircuit circuit{2};

    const QuantumRegister initialState = QuantumRegister::basisState(2, 0);

    circuit.addSingleQubitGate(quantum_sim::gates::hadamardGate(), 0);
    circuit.addFullRegisterGate(quantum_sim::gates::cnotGate());

    const QuantumRegister bellState = circuit.execute(initialState);

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
        shotVisualization.find(
            "|00> [#####     ] 50 (50.00%)"
        ) != std::string::npos,
        "shot visualizer displays state 00 counts"
    );

    check(
        shotVisualization.find(
            "|01> [          ] 0 (0.00%)"
        ) != std::string::npos,
        "shot visualizer displays zero-count states"
    );

    check(
        shotVisualization.find(
            "|11> [#####     ] 50 (50.00%)"
        ) != std::string::npos,
        "shot visualizer displays state 11 counts"
    );

    std::cout << visualization << std::endl;
    std::cout << shotVisualization << std::endl;
    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

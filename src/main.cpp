#include "quantum_sim/gates/QuantumGates.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

#include <cstddef>
#include <iostream>
#include <random>

int main() {
    using quantum_sim::quantum::MeasurementResult;
    using quantum_sim::quantum::QuantumRegister;

    std::random_device seedSource;
    std::mt19937 randomEngine{seedSource()};

    constexpr std::size_t shotCount = 1'000;

    std::size_t bothZeroCount = 0;
    std::size_t bothOneCount = 0;
    std::size_t mismatchCount = 0;

    for (std::size_t shot = 0; shot < shotCount; ++shot) {
        const QuantumRegister register00 = QuantumRegister::basisState(2, 0);

        const QuantumRegister afterHadamard = register00.applySingleQubitGate(quantum_sim::gates::hadamardGate(), 0);
        QuantumRegister bellState = afterHadamard.applyGate(quantum_sim::gates::cnotGate());

        const MeasurementResult first = bellState.measureQubit(0, randomEngine);
        const MeasurementResult second = bellState.measureQubit(1, randomEngine);

        if (first == MeasurementResult::Zero &&
            second == MeasurementResult::Zero) {
            ++bothZeroCount;
        } else if (first == MeasurementResult::One &&
                   second == MeasurementResult::One) {
            ++bothOneCount;
        } else {
            ++mismatchCount;
        }
    }

    std::cout << "Shots: " << shotCount << '\n';
    std::cout << "Both zero:  " << bothZeroCount << '\n';
    std::cout << "Both one:  " << bothOneCount << '\n';
    std::cout << "Mismatch: " << mismatchCount << '\n';

    return 0;
}

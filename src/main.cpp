#include "quantum_sim/math/Complex.hpp"
#include "quantum_sim/quantum/Qubit.hpp"
#include <iostream>
#include <random>
#include <cstddef>

#include "quantum_sim/gates/QuantumGates.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

int main() {
    using quantum_sim::math::Complex;
    using quantum_sim::quantum::MeasurementResult;
    using quantum_sim::quantum::Qubit;

    std::random_device seedSource;
    std::mt19937 randomEngine{seedSource()};

    constexpr std::size_t shotCount = 1'000;

    std::size_t zeroCount = 0;
    std::size_t oneCount = 0;

    for (std::size_t shot = 0; shot < shotCount; ++shot) {
        const Qubit zeroState{
            Complex{1.0, 0.0},
            Complex{},
        };

        Qubit superposition = zeroState.apply(quantum_sim::gates::hadamardGate());
        const MeasurementResult result = superposition.measure(randomEngine);
        if (result == MeasurementResult::Zero) {
            ++zeroCount;
        } else {
            ++oneCount;
        }
    }

    std::cout << "Shots: " << shotCount << '\n';
    std::cout << "Zero:  " << zeroCount << '\n';
    std::cout << "One:   " << oneCount << '\n';
    return 0;
}

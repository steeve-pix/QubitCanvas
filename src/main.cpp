#include "quantum_sim/math/Complex.hpp"
#include "quantum_sim/quantum/Qubit.hpp"
#include <iostream>
#include <random>
#include <cstddef>

#include "quantum_sim/gates/QuantumGates.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

int main() {
    using quantum_sim::math::Complex;
    using quantum_sim::math::ComplexVector;
    using quantum_sim::quantum::MeasurementResult;
    using quantum_sim::quantum::Qubit;
    using quantum_sim::quantum::QuantumRegister;

    std::random_device seedSource;
    std::mt19937 randomEngine{seedSource()};

    constexpr std::size_t shotCount = 1'000;

    std::size_t count00 = 0;
    std::size_t count11 = 0;
    std::size_t unexpectedCount = 0;

    for (std::size_t shot = 0; shot < shotCount; ++shot) {
        const QuantumRegister register00{
            2, ComplexVector{
                std::vector{
                    Complex{1.0, 0.0},
                    Complex{0.0, 0.0},
                    Complex{0.0, 0.0},
                    Complex{0.0, 0.0},
                }
            }
        };

        const QuantumRegister afterHadamard = register00.applySingleQubitGate(quantum_sim::gates::hadamardGate(), 0);
        QuantumRegister bellState = afterHadamard.applyGate(quantum_sim::gates::cnotGate());

        const std::size_t result =
                bellState.measure(randomEngine);
        if (result == 0) {
            ++count00;
        } else if (result == 3) {
            ++count11;
        } else {
            ++unexpectedCount;
        }
    }

    std::cout << "Shots: " << shotCount << '\n';
    std::cout << "|00>:  " << count00 << '\n';
    std::cout << "|11>:  " << count11 << '\n';
    std::cout << "Other: " << unexpectedCount << '\n';

    return 0;
}

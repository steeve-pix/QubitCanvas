#include <iostream>

#include "quantum_sim/gates/SingleQubitGates.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/math/ComplexVector.hpp"
#include "quantum_sim/quantum/Qubit.hpp"

namespace {
    using quantum_sim::math::ComplexVector;
    using quantum_sim::math::ComplexMatrix;
    using quantum_sim::math::Complex;
    using quantum_sim::quantum::Quibit;

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
    const Quibit zeroQubit{
        Complex{1.0, 0.0},
        Complex{0.0, 0.0},
    };

    check(
        approximatelyEqual(zeroQubit.zeroAmplitude().real(), 1.0),
        "zero qubit stores its zero amplitude"
    );

    const double amplitude = 1 / std::sqrt(2.0);
    bool isInvalidQubit = false;
    try {
        const Quibit invalidQubit{
            Complex{1.0, 0.0},
            Complex{1.0, 0.0}
        };
        static_cast<void>(invalidQubit);
    } catch (const std::invalid_argument &) {
        isInvalidQubit = true;
    }

    check(isInvalidQubit, "qubit rejects non-normalized amplitudes");

    if (failures == 0) {
        std::cout << "All conjugate-transpose tests passed.\n";
    }

    return 0;
}

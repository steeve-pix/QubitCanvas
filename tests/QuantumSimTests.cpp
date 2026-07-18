#include <iostream>
#include <numbers>
#include <cmath>

#include "quantum_sim/gates/SingleQubitGates.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/math/ComplexVector.hpp"
#include "quantum_sim/quantum/Qubit.hpp"

namespace {
    using quantum_sim::math::ComplexVector;
    using quantum_sim::math::ComplexMatrix;
    using quantum_sim::math::Complex;
    using quantum_sim::quantum::Qubit;
    using quantum_sim::quantum::MeasurementResult;

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
    std::mt19937 randomEngine{42};

    Qubit zero{
        Complex{1.0, 0.0},
        Complex{0.0, 0.0},
    };
    Qubit one{
        Complex{0.0, 0.0},
        Complex{1.0, 0.0}
    };
    const double amplitude = std::cos(std::numbers::pi / 4);
    Qubit superposition{
        Complex{amplitude, 0.0},
        Complex{amplitude, 0.0}
    };

    const MeasurementResult firstResult =
            superposition.measure(randomEngine);

    const MeasurementResult secondResult =
            superposition.measure(randomEngine);

    check(
        secondResult == firstResult,
        "repeated measurement returns the collapsed result"
    );
    if (firstResult == MeasurementResult::Zero) {
        check(
            approximatelyEqual(superposition.probabilityOfZero(), 1.0) &&
            approximatelyEqual(superposition.probabilityOfOne(), 0.0),
            "zero measurement collapses superposition to zero"
        );
    } else {
        check(
            approximatelyEqual(superposition.probabilityOfZero(), 0.0) &&
            approximatelyEqual(superposition.probabilityOfOne(), 1.0),
            "one measurement collapses superposition to one"
        );
    }

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

#include <iostream>

#include "quantum_sim/math/ComplexVector.hpp"

namespace {
    using quantum_sim::math::ComplexVector;
    using quantum_sim::math::Complex;

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
    const ComplexVector zeroState{
        std::vector{
            Complex{1.0, 0.0},
            Complex{0.0, 0.0}
        }
    };

    check(
        zeroState.size() == 2,
        "complex vector reports its size"
    );

    check(
        approximatelyEqual(zeroState.at(0).real(), 1.0) &&
        approximatelyEqual(zeroState.at(0).imaginary(), 0.0),
        "complex vector stores its first amplitude"
    );

    check(
        approximatelyEqual(zeroState.at(1).real(), 0.0) &&
        approximatelyEqual(zeroState.at(1).imaginary(), 0.0),
        "complex vector stores its second amplitude"
    );

    bool throwOutOfRange = false;
    try {
        auto val = zeroState.at(2);
        static_cast<void>(val);
    } catch (const std::out_of_range &) {
        throwOutOfRange = true;
    }
    check(throwOutOfRange, "complex vector rejects an invalid index");


    if (failures == 0) {
        std::cout << "All Complex tests passed.\n";
    } else {
        std::cerr << failures << " test(s) failed.\n";
    }

    return 0;
}

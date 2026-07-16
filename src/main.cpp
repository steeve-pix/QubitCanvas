#include <iostream>

#include "quantum_sim/math/Complex.hpp"

int main() {
    const quantum_sim::math::Complex first{1.0, 2.0};
    const quantum_sim::math::Complex second{3.0, 4.0};
    const auto result = first * second;

    std::cout << result.real() << " + "
            << result.imaginary() << "i\n";

    return 0;
}

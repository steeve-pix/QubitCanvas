#include <iostream>

#include "quantum_sim/math/Complex.hpp"

int main() {
    const quantum_sim::math::Complex zero{};
    const quantum_sim::math::Complex first{3.0, 4.0};
    const quantum_sim::math::Complex second{1.0, 1.0};
    const quantum_sim::math::Complex imaginary{0.0, 2.0};

    std::cout << zero.magnitudeSquared() << '\n';
    std::cout << first.magnitudeSquared() << '\n';
    std::cout << second.magnitudeSquared() << '\n';
    std::cout << imaginary.magnitudeSquared() << '\n';
    
    return 0;
}

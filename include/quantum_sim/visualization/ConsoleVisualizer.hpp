#pragma once

#include "quantum_sim/quantum/QuantumRegister.hpp"

#include <cstddef>
#include <iosfwd>

namespace quantum_sim::visualization {
    void printProbabilityBars(const quantum::QuantumRegister &state, std::ostream &output, std::size_t barWidth = 50);
}

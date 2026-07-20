#pragma once

#include "quantum_sim/quantum/QuantumRegister.hpp"

#include <cstddef>
#include <iosfwd>
#include <vector>

namespace quantum_sim::visualization {
    void printProbabilityBars(const quantum::QuantumRegister &state, std::ostream &output, std::size_t barWidth = 50);

    void printShotBars(const quantum::QuantumRegister &state, const std::vector<std::size_t> &counts,
                       std::ostream &output, std::size_t barWidth = 50);
}

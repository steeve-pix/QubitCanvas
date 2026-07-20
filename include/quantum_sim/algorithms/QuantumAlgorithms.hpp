#pragma once

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include <cstddef>

namespace quantum_sim::algorithms {
    [[nodiscard]] circuit::QuantumCircuit bellStateCircuit();

    [[nodiscard]] circuit::QuantumCircuit equalSuperpositionCircuit(std::size_t qubitCount);
}

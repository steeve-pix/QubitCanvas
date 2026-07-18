#include "quantum_sim/quantum/QuantumRegister.hpp"

#include <cmath>
#include <stdexcept>

namespace quantum_sim::quantum {
    QuantumRegister::QuantumRegister(std::size_t qubitCount, math::ComplexVector amplitudes)
        : qubitCount_(qubitCount), amplitudes_(amplitudes) {
        if (qubitCount == 0)
            throw std::invalid_argument{
                "A quantum register must contain at least one qubit."
            };
        if (qubitCount_ >= std::numeric_limits<std::size_t>::digits)
            throw std::invalid_argument{
                "Qubit count is too large to represent its state count."
            };

        const std::size_t expectedStateCount =
                std::size_t{1} << qubitCount_;

        if (amplitudes.size() != expectedStateCount)
            throw std::invalid_argument{
                "Amplitude count must equal 2 raised to the qubit count."
            };

        if (!amplitudes.isNormalized())
            throw std::invalid_argument{
                "Quantum register amplitudes must be normalized."
            };
    }

    std::size_t QuantumRegister::qubitCount() const noexcept {
        return qubitCount_;
    }

    std::size_t QuantumRegister::stateCount() const noexcept {
        return amplitudes_.size();
    }

    const math::Complex &QuantumRegister::amplitude(std::size_t stateIndex) const {
        return amplitudes_.at(stateIndex);
    }
}

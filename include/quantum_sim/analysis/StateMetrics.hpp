#pragma once

#include "quantum_sim/quantum/QuantumRegister.hpp"

#include <cstddef>
#include <vector>

namespace quantum_sim::analysis {
    /**
     * Reduced-state diagnostics for one qubit versus the rest of a pure register.
     */
    struct QubitMetrics {
        std::size_t qubit{};
        double probabilityZero{};
        double probabilityOne{};
        double coherenceMagnitude{};
        double purity{};
        double entropyBits{};
        double blochLength{};
    };

    /**
     * Numerically stable state-vector analysis used by the Inspector.
     */
    class StateMetrics final {
    public:
        /**
         * Computes |<left|right>|^2.
         *
         * @throws std::invalid_argument when register dimensions differ.
         */
        [[nodiscard]] static double fidelity(
            const quantum::QuantumRegister &left,
            const quantum::QuantumRegister &right
        );

        /**
         * Partially traces out every qubit except the requested one.
         *
         * Purity is Tr(rho^2). Entropy is the base-2 von Neumann entropy;
         * because the complete register is pure, it is also the entanglement
         * entropy between this qubit and the remainder of the register.
         *
         * @throws std::out_of_range when qubit is outside the register.
         */
        [[nodiscard]] static QubitMetrics forQubit(
            const quantum::QuantumRegister &state,
            std::size_t qubit
        );

        /**
         * @return Reduced metrics for every qubit in q0..q(n-1) order.
         */
        [[nodiscard]] static std::vector<QubitMetrics> forRegister(
            const quantum::QuantumRegister &state
        );
    };
}

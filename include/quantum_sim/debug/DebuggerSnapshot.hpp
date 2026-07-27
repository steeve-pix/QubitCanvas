#pragma once
#include <cstddef>
#include <functional>
#include <optional>

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"

namespace quantum_sim::debug {
    /**
     * Read-only view of the debugger state for one UI frame.
     *
     * References point back into DebuggerSession-owned storage and remain valid until
     * the session is rebuilt or advanced again.
     */
    struct DebuggerSnapshot {
        /**
         * Displayed timeline position. Zero represents the untouched initial
         * register; values 1..stepCount represent executed instructions.
         */
        std::size_t currentStepNumber;

        /**
         * Number of executable circuit instructions.
         */
        std::size_t stepCount;

        /**
         * Current instruction, or nullopt while displaying initial step zero.
         */
        std::optional<std::reference_wrapper<const circuit::CircuitInstructionInfo> > instruction;

        /**
         * Register before the current instruction, or the initial register at step zero.
         */
        std::reference_wrapper<const quantum::QuantumRegister> beforeState;

        /**
         * Register displayed at the current timeline position.
         */
        std::reference_wrapper<const quantum::QuantumRegister> afterState;

        /**
         * Whether the debugger can advance to another timeline position.
         */
        bool canMoveNext;

        /**
         * Whether the debugger can move toward the initial state.
         */
        bool canMovePrevious;
    };
}

#pragma once

#include "imgui.h"

#include <cstddef>
#include <optional>
#include <string>

#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/debug/DebuggerSession.hpp"
#include "quantum_sim/gui/rendering/BlochSphereRenderer.hpp"

namespace quantum_sim::gui {
    class InspectorPanel {
    public:
        bool draw(
            debug::DebuggerSession &session,
            const debug::DebuggerSnapshot &snapshot,
            const circuit::QuantumCircuit &circuit,
            std::optional<std::size_t> selectedInstructionIndex,
            ImFont *headingFont
        );

    private:
        BlochSphereRenderer blochSphereRenderer_;
        double navigationConfirmationUntil_{0.0};
        std::string navigationConfirmationMessage_;

        void showNavigationConfirmation(std::string message);

        void moveToPreviousInstruction(
            debug::DebuggerSession &session
        );

        void moveToNextInstruction(
            debug::DebuggerSession &session
        );

        void restartDebugger(
            debug::DebuggerSession &session
        );

        void jumpToInstruction(
            debug::DebuggerSession &session,
            std::size_t instructionIndex
        );

        void drawNavigationConfirmation() const;

        void drawDebuggerControls(
            debug::DebuggerSession &session,
            const debug::DebuggerSnapshot &snapshot
        );

        bool drawInstructionSummary(
            debug::DebuggerSession &session,
            const debug::DebuggerSnapshot &snapshot,
            const circuit::QuantumCircuit &circuit,
            std::optional<std::size_t> selectedInstructionIndex
        );

        void drawQuantumState(
            const quantum::QuantumRegister &state
        );

        void drawProbabilities(
            const quantum::QuantumRegister &state
        );

        void drawAmplitudes(
            const quantum::QuantumRegister &state
        );

        void drawBlochInformation(
            const quantum::QuantumRegister &state
        );

        void drawHeader(
            const debug::DebuggerSnapshot &snapshot,
            std::optional<std::size_t> selectedInstructionIndex,
            ImFont *headingFont
        );

        [[nodiscard]] const quantum::QuantumRegister &resolveInspectedState(
            const debug::DebuggerSession &session,
            const debug::DebuggerSnapshot &snapshot,
            std::optional<std::size_t> selectedInstructionIndex
        ) const;
    };
}

#pragma once

#include "imgui.h"
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/debug/DebuggerSession.hpp"
#include "quantum_sim/gui/rendering/BlochSphereRenderer.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string>

namespace quantum_sim::gui {
    /**
     * Right-side panel for debugger controls and quantum-state inspection.
     */
    class InspectorPanel {
    public:
        /**
         * Draws the inspector for the current frame.
         *
         * @param session Debugger session that navigation controls can mutate.
         * @param snapshot Current debugger state to display.
         * @param circuit Circuit whose instruction metadata is inspected.
         * @param selectedInstructionIndex Optional circuit selection from the renderer.
         * @param headingFont Font used for the panel title.
         * @return True when the panel jumped to the selected instruction.
         */
        bool draw(
            debug::DebuggerSession &session,
            const debug::DebuggerSnapshot &snapshot,
            const circuit::QuantumCircuit &circuit,
            std::optional<std::size_t> selectedInstructionIndex,
            ImFont *headingFont
        );

    private:
        BlochSphereRenderer blochSphereRenderer_;
        std::array<char, 64> amplitudeFilter_{};
        bool showOnlyLiveAmplitudes_{true};
        bool sortAmplitudesByProbability_{true};
        int maximumVisibleAmplitudes_{96};
        int inspectedBlochQubit_{0};
        double navigationConfirmationUntil_{0.0};
        std::string navigationConfirmationMessage_;

        /**
         * Shows a short-lived status message after navigation.
         *
         * @param message Message to display.
         */
        void showNavigationConfirmation(std::string message);

        /**
         * Moves the debugger one instruction backward when possible.
         *
         * @param session Session to mutate.
         */
        void moveToPreviousInstruction(debug::DebuggerSession &session);

        /**
         * Moves the debugger one instruction forward when possible.
         *
         * @param session Session to mutate.
         */
        void moveToNextInstruction(debug::DebuggerSession &session);

        /**
         * Resets debugger navigation to the first instruction.
         *
         * @param session Session to mutate.
         */
        void restartDebugger(debug::DebuggerSession &session);

        /**
         * Moves debugger navigation to a specific instruction.
         *
         * @param session Session to mutate.
         * @param instructionIndex Target instruction index.
         */
        void jumpToInstruction(debug::DebuggerSession &session, std::size_t instructionIndex);

        /**
         * Draws the current short-lived navigation message, if active.
         */
        void drawNavigationConfirmation() const;

        /**
         * Draws previous/next/restart controls and keyboard shortcuts.
         *
         * @param session Session controlled by the buttons.
         * @param snapshot Snapshot used to enable or disable movement.
         */
        void drawDebuggerControls(debug::DebuggerSession &session, const debug::DebuggerSnapshot &snapshot);

        /**
         * Draws selected/current instruction name and explanation.
         *
         * @param session Session used when jumping to a selection.
         * @param snapshot Current debugger state.
         * @param circuit Circuit metadata source.
         * @param selectedInstructionIndex Optional renderer selection.
         * @return True when a jump-to-selection occurred.
         */
        bool drawInstructionSummary(
            debug::DebuggerSession &session,
            const debug::DebuggerSnapshot &snapshot,
            const circuit::QuantumCircuit &circuit,
            std::optional<std::size_t> selectedInstructionIndex
        );

        /**
         * Draws all quantum-state inspection widgets.
         *
         * @param state Register to inspect.
         */
        void drawQuantumState(const quantum::QuantumRegister &state);

        /**
         * Draws per-qubit marginal probability bars.
         *
         * @param state Register to summarize.
         */
        void drawProbabilities(const quantum::QuantumRegister &state);

        /**
         * Draws compact color cells for all amplitudes.
         *
         * @param state Register to visualize.
         */
        void drawStateHeatmap(const quantum::QuantumRegister &state);

        /**
         * Draws searchable, capped amplitude rows for large registers.
         *
         * @param state Register to list.
         */
        void drawAmplitudes(const quantum::QuantumRegister &state);

        /**
         * Draws Bloch-vector data and the interactive Bloch sphere.
         *
         * @param state Register whose selected qubit is inspected.
         */
        void drawBlochInformation(const quantum::QuantumRegister &state);

        /**
         * Draws the inspector title and step readout.
         *
         * @param snapshot Current debugger state.
         * @param selectedInstructionIndex Optional renderer selection.
         * @param headingFont Font used for the title.
         */
        void drawHeader(
            const debug::DebuggerSnapshot &snapshot,
            std::optional<std::size_t> selectedInstructionIndex,
            ImFont *headingFont
        );

        /**
         * Chooses whether the inspector shows selected-instruction state or current debugger state.
         *
         * @param session Debugger session containing trace states.
         * @param snapshot Current debugger state.
         * @param selectedInstructionIndex Optional renderer selection.
         * @return Register state to inspect for this frame.
         */
        [[nodiscard]] const quantum::QuantumRegister &resolveInspectedState(
            const debug::DebuggerSession &session,
            const debug::DebuggerSnapshot &snapshot,
            std::optional<std::size_t> selectedInstructionIndex
        ) const;
    };
}

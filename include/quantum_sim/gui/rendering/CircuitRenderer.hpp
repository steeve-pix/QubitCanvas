#pragma once
#include "CircuitStyle.hpp"
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/debug/DebuggerSession.hpp"

#include <cstddef>
#include <optional>
#include <string>

struct ImDrawList;
struct ImVec2;

namespace quantum_sim::gui {
    /**
     * Completed placement data for a two-qubit gate.
     */
    struct ControlledPlacement {
        std::string gateName;
        std::size_t controlQubit;
        std::size_t targetQubit;
        std::size_t instructionIndex;
    };

    /**
     * Completed placement data for a single-qubit gate.
     */
    struct SingleQubitPlacement {
        std::string gateName;
        std::size_t targetQubit;
        std::size_t instructionIndex;
    };

    /**
     * ImGui draw-list renderer for the interactive circuit canvas.
     */
    class CircuitRenderer {
    public:
        /**
         * Draws the circuit and updates transient interaction state.
         *
         * @param circuit Circuit to render.
         * @param snapshot Debugger state used to highlight the active instruction.
         * @param pendingGate Gate currently being placed, if any.
         */
        void draw(const circuit::QuantumCircuit &circuit, const debug::DebuggerSnapshot &snapshot,
                  const std::optional<std::string> &pendingGate);

        /**
         * Creates a renderer with a style.
         *
         * @param style Layout, color, and animation constants.
         */
        explicit CircuitRenderer(CircuitStyle style = {});

        /**
         * @return Current style values.
         */
        [[nodiscard]] const CircuitStyle &style() const noexcept;

        /**
         * Replaces style values.
         *
         * @param style New style values.
         */
        void setStyle(CircuitStyle style);

        /**
         * @return Selected instruction index, if any gate is selected.
         */
        [[nodiscard]] std::optional<std::size_t> selectedInstructionIndex() const noexcept;

        /**
         * Clears the currently selected instruction.
         */
        void clearSelection() noexcept;

        /**
         * @return Completed controlled/two-qubit placement without consuming it.
         */
        [[nodiscard]] std::optional<ControlledPlacement> completedControlledPlacement() const noexcept;

        /**
         * Returns and clears a completed controlled/two-qubit placement.
         *
         * @return Placement data, or nullopt when placement is incomplete.
         */
        [[nodiscard]] std::optional<ControlledPlacement> consumeCompletedControlledPlacement() noexcept;

        /**
         * Returns and clears a completed single-qubit placement.
         *
         * @return Placement data, or nullopt when placement is incomplete.
         */
        [[nodiscard]] std::optional<SingleQubitPlacement> consumeCompletedSingleQubitPlacement();

        /**
         * @return True when a two-qubit placement has picked its first qubit.
         */
        [[nodiscard]] bool hasPendingControlQubit() const noexcept;

        /**
         * Clears all in-progress placement state.
         */
        void cancelPlacement() noexcept;

        /**
         * @return Current insertion slot while placing a gate, if any.
         */
        [[nodiscard]] std::optional<std::size_t> pendingInsertionIndex() const noexcept;

    private:
        /**
         * Draws one boxed gate symbol.
         *
         * @param drawList ImGui draw target.
         * @param center Gate center position in screen coordinates.
         * @param label Gate label.
         * @param highlighted Whether this is the active debugger step.
         * @param hovered Whether the mouse is over the gate.
         * @param selected Whether this gate is selected.
         * @param placementPreview Whether this gate is a placement preview.
         */
        void drawGate(ImDrawList *drawList, const ImVec2 &center, const std::string &label, bool highlighted,
                      bool hovered, bool selected, bool placementPreview);

        std::optional<std::size_t> selectedInstructionIndex_;
        CircuitStyle style_;
        std::optional<std::size_t> pendingControlQubit_;
        std::optional<std::size_t> pendingTargetQubit_;
        std::optional<SingleQubitPlacement> completedSingleQubitPlacement_;
        std::optional<std::size_t> pendingInsertionIndex_;
        std::optional<std::string> pendingControlledGateName_;
    };
}

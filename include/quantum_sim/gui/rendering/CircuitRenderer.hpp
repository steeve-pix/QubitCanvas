#pragma once
#include "CircuitStyle.hpp"
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/debug/DebuggerSession.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

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
     * Completed placement data for a compact three-qubit gate.
     */
    struct ThreeQubitPlacement {
        std::string gateName;
        std::size_t firstQubit;
        std::size_t secondQubit;
        std::size_t thirdQubit;
        std::size_t instructionIndex;
    };

    /**
     * Completed drag movement for one existing circuit instruction.
     */
    struct InstructionMove {
        std::size_t fromIndex;
        std::size_t toIndex;
    };

    /**
     * Screen-space point used to attach controls to a rendered instruction.
     *
     * Keeping this as two plain floats avoids leaking ImGui value types into
     * application state while still allowing an overlay to follow a gate.
     */
    struct InstructionScreenAnchor {
        float x{};
        float y{};
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
         * @return Screen-space center of the selected gate from the latest draw.
         */
        [[nodiscard]] std::optional<InstructionScreenAnchor>
        selectedInstructionScreenAnchor() const noexcept;

        /**
         * @return Sorted instruction indices in the current multi-selection.
         */
        [[nodiscard]] const std::vector<std::size_t> &
        selectedInstructionIndices() const noexcept;

        /**
         * Returns and clears a debugger-step jump requested by double-clicking.
         *
         * @return Displayed step number, where zero is the initial state and
         *         1..N correspond to circuit instructions, or nullopt when no
         *         double-click navigation request is pending.
         */
        [[nodiscard]] std::optional<std::size_t> consumeStepJumpRequest() noexcept;

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
         * Returns and clears a completed three-qubit placement.
         *
         * @return Three selected operands and insertion index, or nullopt while
         *         the placement is incomplete.
         */
        [[nodiscard]] std::optional<ThreeQubitPlacement> consumeCompletedThreeQubitPlacement() noexcept;

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
         * @return Number of distinct operands selected for the active placement.
         */
        [[nodiscard]] std::size_t placementOperandCount() const noexcept;

        /**
         * Clears all in-progress placement state.
         */
        void cancelPlacement() noexcept;

        /**
         * @return Current insertion slot while placing a gate, if any.
         */
        [[nodiscard]] std::optional<std::size_t> pendingInsertionIndex() const noexcept;

        /**
         * Returns and clears a gate movement requested by a canvas drag.
         *
         * @return Source and destination instruction indices, or nullopt.
         */
        [[nodiscard]] std::optional<InstructionMove> consumeInstructionMoveRequest() noexcept;

        /**
         * Keeps repeated placement at the slot immediately after a new gate.
         *
         * @param insertedInstructionIndex Index of the newly inserted gate.
         */
        void continuePlacementAfter(std::size_t insertedInstructionIndex) noexcept;

        /**
         * Selects and reveals an instruction after an external edit.
         *
         * @param instructionIndex Instruction to select.
         */
        void selectInstruction(std::size_t instructionIndex);

        /**
         * Replaces the current selection and focuses its final instruction.
         *
         * @param instructionIndices Instruction indices to select.
         */
        void selectInstructions(
            std::vector<std::size_t> instructionIndices
        );

        /**
         * Requests horizontal focus on one displayed timeline step.
         *
         * @param stepNumber Zero for the initial state, otherwise instruction step.
         */
        void requestFocusStep(std::size_t stepNumber) noexcept;

        /**
         * Increases timeline spacing and disables automatic fitting.
         */
        void zoomIn() noexcept;

        /**
         * Decreases timeline spacing and disables automatic fitting.
         */
        void zoomOut() noexcept;

        /**
         * Fits the complete timeline to the available width when practical.
         */
        void fitToView() noexcept;

        /**
         * @return Current user-controlled timeline zoom multiplier.
         */
        [[nodiscard]] float viewZoom() const noexcept;

        /**
         * @return True while timeline spacing follows the available width.
         */
        [[nodiscard]] bool isFittingToView() const noexcept;

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
        std::optional<InstructionScreenAnchor> selectedInstructionScreenAnchor_;
        std::vector<std::size_t> selectedInstructionIndices_;
        std::optional<std::size_t> selectionAnchorIndex_;
        std::optional<std::size_t> requestedStepJumpNumber_;
        CircuitStyle style_;
        std::optional<std::size_t> pendingControlQubit_;
        std::optional<std::size_t> pendingTargetQubit_;
        std::optional<std::size_t> pendingThirdQubit_;
        std::optional<SingleQubitPlacement> completedSingleQubitPlacement_;
        std::optional<std::size_t> pendingInsertionIndex_;
        std::optional<float> insertionMouseXLock_;
        std::optional<std::string> pendingControlledGateName_;
        std::optional<std::size_t> draggedInstructionIndex_;
        std::optional<std::size_t> dragDestinationIndex_;
        std::optional<InstructionMove> completedInstructionMove_;
        std::optional<std::size_t> requestedFocusStepNumber_;
        std::optional<std::size_t> lastFocusedStepNumber_;
        std::size_t lastFocusedInstructionCount_{0};
        float effectiveWireSpacing_{70.0F};
        float viewZoom_{1.0F};
        bool fitToWindow_{true};
        bool placementModeWasActive_{false};

        /**
         * @return True when one instruction belongs to the multi-selection.
         */
        [[nodiscard]] bool isInstructionSelected(
            std::size_t instructionIndex
        ) const noexcept;

        /**
         * Applies plain, Ctrl-toggle, or Shift-range selection semantics.
         */
        void updateInstructionSelection(
            std::size_t instructionIndex
        );

        /**
         * Makes one instruction the complete selection.
         */
        void setSingleInstructionSelection(
            std::size_t instructionIndex
        );
    };
}

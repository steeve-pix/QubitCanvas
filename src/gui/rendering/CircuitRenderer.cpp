#include "quantum_sim/gui/rendering/CircuitRenderer.hpp"
#include "quantum_sim/gui/GateNotation.hpp"
#include "quantum_sim/gui/QuantumNotation.hpp"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace {
    [[nodiscard]] float animationPulse(const float speed) {
        const auto time =
                static_cast<float>(ImGui::GetTime());

        return 0.5F + 0.5F * std::sin(time * speed);
    }

    [[nodiscard]] ImU32 activeOrange(
        const quantum_sim::gui::CircuitStyle &style,
        const float pulse,
        const int alpha = 255
    ) {
        return IM_COL32(
            style.activeRed,
            static_cast<int>( style.activeGreenBase + pulse * style.activeGreenPulse ),
            static_cast<int>( style.activeBlueBase + pulse * style.activeBluePulse ), alpha);
    }

    [[nodiscard]] ImU32 withAlpha(
        const ImU32 color,
        const int alpha
    ) {
        const int clampedAlpha =
                std::clamp(alpha, 0, 255);

        return (color & IM_COL32(255, 255, 255, 0))
               | IM_COL32(0, 0, 0, clampedAlpha);
    }

    [[nodiscard]] bool isPointInsideRect(
        const ImVec2 &point,
        const ImVec2 &minimum,
        const ImVec2 &maximum
    ) {
        return point.x >= minimum.x &&
               point.x <= maximum.x &&
               point.y >= minimum.y &&
               point.y <= maximum.y;
    }

    [[nodiscard]] float gateHalfWidthForLabel(
        const quantum_sim::gui::CircuitStyle &style,
        const std::string &label
    ) {
        const float desiredHalfWidth =
                ImGui::CalcTextSize(
                    label.c_str()
                ).x *
                0.5F +
                style.gateLabelPaddingX;

        return std::clamp(
            desiredHalfWidth,
            style.gateHalfWidth,
            style.maximumGateHalfWidth
        );
    }

    [[nodiscard]] bool isInteractionGateName(
        const std::string &gateName
    ) noexcept;

    [[nodiscard]] std::string_view gateBehavior(
        const std::string &gateName
    ) noexcept {
        if (gateName == "H") {
            return "Creates or removes an equal superposition.";
        }
        if (gateName == "X") {
            return "Exchanges |0\xE2\x9F\xA9 and |1\xE2\x9F\xA9 like a quantum bit flip.";
        }
        if (gateName == "Y") {
            return "Combines a bit flip with a phase change around the Y axis.";
        }
        if (gateName == "Z") {
            return "Flips the phase of the |1\xE2\x9F\xA9 component.";
        }
        if (gateName == "S") {
            return "Applies a quarter-turn phase to the |1\xE2\x9F\xA9 component.";
        }
        if (gateName == "Sdg") {
            return "Reverses the quarter-turn phase applied by S.";
        }
        if (gateName == "T") {
            return "Applies an eighth-turn phase to the |1\xE2\x9F\xA9 component.";
        }
        if (gateName == "Tdg") {
            return "Reverses the eighth-turn phase applied by T.";
        }
        if (gateName == "SX") {
            return "Performs half of an X rotation; applying it twice produces X.";
        }
        if (gateName == "SXdg") {
            return "Reverses the half-X rotation applied by square-root X.";
        }
        if (gateName == "P") {
            return "Applies the selected phase without changing basis probabilities.";
        }
        if (gateName == "U") {
            return "Applies a general one-qubit rotation using three angles.";
        }
        if (gateName == "Rx") {
            return "Rotates the qubit around the Bloch X axis.";
        }
        if (gateName == "Ry") {
            return "Rotates the qubit around the Bloch Y axis.";
        }
        if (gateName == "Rz") {
            return "Rotates the qubit around the Bloch Z axis.";
        }
        if (gateName == "SWAP") {
            return "Exchanges the quantum states of the connected qubits.";
        }
        if (gateName == "iSWAP") {
            return "Exchanges |01\xE2\x9F\xA9 and |10\xE2\x9F\xA9 while applying a phase of i.";
        }
        if (gateName == "sqrtSWAP") {
            return "Performs a half exchange; applying it twice produces SWAP.";
        }
        if (gateName == "CSWAP") {
            return "Exchanges the two targets only when the control is |1\xE2\x9F\xA9.";
        }
        if (gateName == "CCX") {
            return "Flips the target only when both controls are |1\xE2\x9F\xA9.";
        }
        if (gateName == "DCX") {
            return "Applies controlled-X in both directions.";
        }
        if (gateName == "ECR") {
            return "Applies an echoed cross-resonance entangling operation.";
        }
        if (gateName == "fSim") {
            return "Combines excitation exchange with a conditional phase.";
        }
        if (gateName == "RXX") {
            return "Jointly rotates the pair through X tensor X coupling.";
        }
        if (gateName == "RYY") {
            return "Jointly rotates the pair through Y tensor Y coupling.";
        }
        if (gateName == "RZZ") {
            return "Applies a parity-dependent phase through Z tensor Z coupling.";
        }
        if (
            gateName == "CX" ||
            gateName == "CY" ||
            gateName == "CZ" ||
            gateName == "CH" ||
            gateName == "CS" ||
            gateName == "CSdg" ||
            gateName == "CT" ||
            gateName == "CTdg" ||
            gateName == "CP" ||
            gateName == "CRx" ||
            gateName == "CRy" ||
            gateName == "CRz"
        ) {
            return "Applies the target operation only when the control is |1\xE2\x9F\xA9.";
        }

        return {};
    }

    [[nodiscard]] std::string instructionOperands(
        const quantum_sim::circuit::CircuitInstructionInfo &instruction
    ) {
        const auto qubitLabel =
                [](const std::size_t qubit) {
                    return "q" + std::to_string(qubit);
                };

        if (
            instruction.controlQubit.has_value() &&
            instruction.secondaryTargetQubit.has_value() &&
            instruction.tertiaryTargetQubit.has_value()
        ) {
            if (instruction.name == "CSWAP") {
                return "Control " +
                       qubitLabel(instruction.controlQubit.value()) +
                       "  |  Exchange " +
                       qubitLabel(instruction.secondaryTargetQubit.value()) +
                       " \xE2\x86\x94 " +
                       qubitLabel(instruction.tertiaryTargetQubit.value());
            }

            return "Controls " +
                   qubitLabel(instruction.controlQubit.value()) +
                   ", " +
                   qubitLabel(instruction.secondaryTargetQubit.value()) +
                   "  |  Target " +
                   qubitLabel(instruction.tertiaryTargetQubit.value());
        }

        if (
            instruction.controlQubit.has_value() &&
            instruction.secondaryTargetQubit.has_value()
        ) {
            const bool exchangeGate =
                    instruction.name == "SWAP" ||
                    instruction.name == "iSWAP" ||
                    instruction.name == "sqrtSWAP";

            if (exchangeGate) {
                return "Exchange " +
                       qubitLabel(instruction.controlQubit.value()) +
                       " \xE2\x86\x94 " +
                       qubitLabel(instruction.secondaryTargetQubit.value());
            }

            if (isInteractionGateName(instruction.name)) {
                return "Pair " +
                       qubitLabel(instruction.controlQubit.value()) +
                       ", " +
                       qubitLabel(instruction.secondaryTargetQubit.value());
            }

            return "Control " +
                   qubitLabel(instruction.controlQubit.value()) +
                   "  |  Target " +
                   qubitLabel(instruction.secondaryTargetQubit.value());
        }

        if (instruction.targetQubit.has_value()) {
            return "Target " +
                   qubitLabel(instruction.targetQubit.value());
        }

        return "Full register";
    }

    void showInstructionTooltip(
        const quantum_sim::circuit::CircuitInstructionInfo &instruction,
        const std::size_t instructionIndex,
        const std::size_t instructionCount
    ) {
        const std::string operands =
                instructionOperands(instruction);

        ImGui::BeginTooltip();
        ImGui::TextColored(
            ImVec4{0.35F, 0.80F, 1.0F, 1.0F},
            "%s",
            instruction.name.c_str()
        );
        ImGui::SameLine();
        ImGui::TextDisabled(
            "step %zu/%zu",
            instructionIndex + 1U,
            instructionCount
        );
        ImGui::Separator();
        ImGui::TextUnformatted(operands.c_str());

        if (instruction.angleRadians.has_value()) {
            const std::string angleText =
                    quantum_sim::gui::notation::formatAngleMeasurement(
                        instruction.angleRadians.value()
                    );

            ImGui::Text(
                "Angle  %s",
                angleText.c_str()
            );
        }

        const std::string_view behavior =
                gateBehavior(instruction.name);

        if (!behavior.empty()) {
            ImGui::Spacing();
            ImGui::PushTextWrapPos(
                ImGui::GetCursorPosX() +
                ImGui::GetFontSize() * 32.0F
            );
            ImGui::TextUnformatted(
                behavior.data(),
                behavior.data() + behavior.size()
            );
            ImGui::PopTextWrapPos();
        } else if (
            instruction.kind ==
            quantum_sim::circuit::CircuitInstructionKind::Reflection
        ) {
            ImGui::Spacing();
            ImGui::TextUnformatted(
                "Applies a reflection across the full register state space."
            );
        } else if (
            instruction.kind ==
            quantum_sim::circuit::CircuitInstructionKind::FullRegister
        ) {
            ImGui::Spacing();
            ImGui::TextUnformatted(
                "Applies one operation to the complete register."
            );
        }

        ImGui::EndTooltip();
    }

    void drawExchangeBadge(
        ImDrawList *drawList,
        const ImVec2 &center,
        const char *label,
        const ImU32 backgroundColor,
        const ImU32 foregroundColor,
        const quantum_sim::gui::CircuitStyle &style
    ) {
        const ImVec2 labelSize =
                ImGui::CalcTextSize(label);

        const ImVec2 minimum{
            center.x -
            labelSize.x * 0.5F -
            style.exchangeBadgePaddingX,
            center.y -
            labelSize.y * 0.5F -
            style.exchangeBadgePaddingY
        };

        const ImVec2 maximum{
            center.x +
            labelSize.x * 0.5F +
            style.exchangeBadgePaddingX,
            center.y +
            labelSize.y * 0.5F +
            style.exchangeBadgePaddingY
        };

        drawList->AddRectFilled(
            minimum,
            maximum,
            backgroundColor,
            style.exchangeBadgeCornerRadius
        );

        drawList->AddRect(
            minimum,
            maximum,
            foregroundColor,
            style.exchangeBadgeCornerRadius,
            0,
            1.0F
        );

        drawList->AddText(
            ImVec2{
                center.x - labelSize.x * 0.5F,
                center.y - labelSize.y * 0.5F
            },
            foregroundColor,
            label
        );
    }

    void drawInsetLine(
        ImDrawList *drawList,
        const ImVec2 &start,
        const ImVec2 &end,
        const ImU32 color,
        const float thickness,
        const float endpointInset
    ) {
        const float deltaX = end.x - start.x;
        const float deltaY = end.y - start.y;
        const float length =
                std::sqrt(deltaX * deltaX + deltaY * deltaY);

        if (length <= endpointInset * 2.0F) {
            return;
        }

        const float insetRatio = endpointInset / length;

        drawList->AddLine(
            ImVec2{
                start.x + deltaX * insetRatio,
                start.y + deltaY * insetRatio
            },
            ImVec2{
                end.x - deltaX * insetRatio,
                end.y - deltaY * insetRatio
            },
            color,
            thickness
        );
    }

    void drawExchangePortJoins(
        ImDrawList *drawList,
        const ImVec2 &upperLeft,
        const ImVec2 &upperRight,
        const ImVec2 &lowerLeft,
        const ImVec2 &lowerRight,
        const ImU32 color,
        const float thickness,
        const float overlap
    ) {
        const float interiorOverlap = thickness * 0.5F;

        for (const ImVec2 &port : {upperLeft, lowerLeft}) {
            drawList->AddLine(
                ImVec2{port.x - overlap, port.y},
                ImVec2{port.x + interiorOverlap, port.y},
                color,
                thickness
            );
        }

        for (const ImVec2 &port : {upperRight, lowerRight}) {
            drawList->AddLine(
                ImVec2{port.x - interiorOverlap, port.y},
                ImVec2{port.x + overlap, port.y},
                color,
                thickness
            );
        }
    }

    [[nodiscard]] bool isTwoQubitGateName(
        const std::string &gateName
    ) noexcept {
        return gateName == "CX" ||
               gateName == "CY" ||
               gateName == "CZ" ||
               gateName == "CH" ||
               gateName == "CS" ||
               gateName == "CSdg" ||
               gateName == "CT" ||
               gateName == "CTdg" ||
               gateName == "CP" ||
               gateName == "CRx" ||
               gateName == "CRy" ||
               gateName == "CRz" ||
               gateName == "SWAP" ||
               gateName == "iSWAP" ||
               gateName == "sqrtSWAP" ||
               gateName == "DCX" ||
               gateName == "ECR" ||
               gateName == "fSim" ||
               gateName == "RXX" ||
               gateName == "RYY" ||
               gateName == "RZZ";
    }

    [[nodiscard]] bool isThreeQubitGateName(
        const std::string &gateName
    ) noexcept {
        return gateName == "CCX" ||
               gateName == "CSWAP";
    }

    [[nodiscard]] bool isInteractionGateName(
        const std::string &gateName
    ) noexcept {
        return gateName == "RXX" ||
               gateName == "RYY" ||
               gateName == "RZZ" ||
               gateName == "DCX" ||
               gateName == "ECR" ||
               gateName == "fSim";
    }

}

namespace quantum_sim::gui {
    void CircuitRenderer::draw(
        const circuit::QuantumCircuit &circuit,
        const debug::DebuggerSnapshot &snapshot,
        const std::optional<std::string> &pendingGate
    ) {
        ImGui::BeginChild(
            "CircuitCanvas",
            ImVec2{style_.canvasWidth, style_.canvasHeight},
            false,
            ImGuiWindowFlags_HorizontalScrollbar
        );

        bool gateClickedThisFrame = false;

        const ImVec2 canvasMin =
                ImGui::GetWindowPos();

        const ImVec2 canvasMax{
            canvasMin.x + ImGui::GetWindowWidth(),
            canvasMin.y + ImGui::GetWindowHeight()
        };

        bool gateDropDelivered = false;

        if (
            ImGui::BeginDragDropTargetCustom(
                ImRect{canvasMin, canvasMax},
                ImGui::GetID("##CircuitGateDropTarget")
            )
        ) {
            const ImGuiPayload *payload =
                    ImGui::AcceptDragDropPayload(
                        "QUBITCANVAS_GATE",
                        ImGuiDragDropFlags_AcceptNoDrawDefaultRect
                    );

            gateDropDelivered =
                    payload != nullptr &&
                    payload->IsDelivery();

            ImGui::EndDragDropTarget();
        }

        const ImVec2 cursorPosition =
                ImGui::GetCursorScreenPos();

        const ImVec2 origin{
            cursorPosition.x + style_.canvasPaddingX,
            cursorPosition.y + style_.canvasPaddingY
        };
        const auto instructions =
                circuit.instructionInfo();

        ImDrawList *drawList =
                ImGui::GetWindowDrawList();

        drawList->AddRectFilled(
            canvasMin,
            canvasMax,
            style_.canvasBackgroundColor
        );

        const bool placementModeActive =
                pendingGate.has_value();

        const bool placementJustStarted =
                placementModeActive &&
                !placementModeWasActive_;

        placementModeWasActive_ =
                placementModeActive;

        if (placementModeActive) {
            // Default new placements to append until the mouse picks another slot.
            if (!pendingInsertionIndex_.has_value()) {
                pendingInsertionIndex_ =
                        instructions.size();
            }

            if (placementJustStarted) {
                requestedFocusStepNumber_ =
                        instructions.size() + 1U;
            }
        } else {
            pendingInsertionIndex_.reset();
            insertionMouseXLock_.reset();
        }

        const bool controlledPlacement =
                placementModeActive &&
                (
                    isTwoQubitGateName(
                        pendingGate.value()
                    ) ||
                    isThreeQubitGateName(
                        pendingGate.value()
                    )
                );

        if (controlledPlacement) {
            // Remember the family so a completed two-click placement can be emitted.
            pendingControlledGateName_ =
                    pendingGate.value();
        } else {
            pendingControlledGateName_.reset();
            pendingControlQubit_.reset();
            pendingTargetQubit_.reset();
            pendingThirdQubit_.reset();
        }

        const bool singleQubitPlacement =
                placementModeActive &&
                !controlledPlacement;

        const std::size_t qubitCount =
                std::max<std::size_t>(
                    circuit.qubitCount(),
                    1
                );

        const float availableWireHeight =
                std::max(
                    0.0F,
                    ImGui::GetWindowHeight() -
                    style_.canvasPaddingY * 2.0F -
                    style_.topMargin -
                    style_.gateHalfHeight -
                    10.0F
                );

        effectiveWireSpacing_ =
                qubitCount > 1U
                    ? std::clamp(
                        availableWireHeight /
                            static_cast<float>(qubitCount - 1U),
                        40.0F,
                        style_.wireSpacing
                    )
                    : style_.wireSpacing;

        const float calculatedContentHeight = style_.canvasPaddingY + style_.topMargin + effectiveWireSpacing_ *
                                              static_cast<float>(qubitCount > 0 ? qubitCount - 1 : 0) + style_.
                                              gateHalfHeight + style_.canvasPaddingY;

        const float circuitContentHeight =
                std::max(calculatedContentHeight, style_.minimumCanvasHeight);

        // Step zero occupies its own column before every executable instruction.
        const std::size_t timelineColumnCount =
                instructions.size() + 1U;

        const float wireStartX =
                origin.x + style_.wireStartOffset;

        const float firstGateX =
                wireStartX + style_.firstGateOffset;

        const float availableWidth =
                ImGui::GetContentRegionAvail().x;

        const float usableGateWidth = availableWidth - style_.wireStartOffset - style_.firstGateOffset - style_.
                                      rightPadding;

        if (
            ImGui::IsWindowHovered() &&
            ImGui::GetIO().KeyCtrl &&
            std::abs(ImGui::GetIO().MouseWheel) > 0.0F
        ) {
            fitToWindow_ = false;
            viewZoom_ =
                    std::clamp(
                        viewZoom_ +
                        ImGui::GetIO().MouseWheel * 0.08F,
                        0.55F,
                        1.80F
                    );
        }

        if (
            ImGui::IsWindowHovered() &&
            ImGui::GetIO().KeyShift &&
            !ImGui::GetIO().KeyCtrl &&
            std::abs(ImGui::GetIO().MouseWheel) > 0.0F
        ) {
            ImGui::SetScrollX(
                std::clamp(
                    ImGui::GetScrollX() -
                        ImGui::GetIO().MouseWheel * 90.0F,
                    0.0F,
                    ImGui::GetScrollMaxX()
                )
            );
        }

        float gateSpacing =
                style_.gateSpacing * viewZoom_;

        if (
            fitToWindow_ &&
            timelineColumnCount > 1U
        ) {
            const float fittedSpacing =
                    usableGateWidth /
                    static_cast<float>(timelineColumnCount - 1U);

            gateSpacing =
                    std::clamp(
                        fittedSpacing,
                        style_.minimumGateSpacing * 0.88F,
                        style_.gateSpacing
                    );
        }

        const float lastGateX =
                firstGateX +
                gateSpacing *
                static_cast<float>(timelineColumnCount - 1U);

        const float firstInstructionX =
                firstGateX + gateSpacing;

        const float firstWireY =
                origin.y + style_.topMargin;

        const float lastWireY =
                firstWireY +
                effectiveWireSpacing_ *
                static_cast<float>(qubitCount - 1);

        if (
            placementModeActive &&
            ImGui::IsWindowHovered()
        ) {
            // Convert mouse X position into the nearest instruction insertion slot.
            const ImVec2 insertionMousePosition =
                    ImGui::GetMousePos();

            if (
                insertionMouseXLock_.has_value() &&
                std::abs(
                    insertionMousePosition.x -
                    insertionMouseXLock_.value()
                ) > 6.0F
            ) {
                insertionMouseXLock_.reset();
            }

            if (!insertionMouseXLock_.has_value()) {
                const float relativeMouseX =
                        insertionMousePosition.x - firstInstructionX;

                const float rawInsertionIndex =
                        relativeMouseX / gateSpacing;

                const long long roundedInsertionIndex =
                        std::llround(rawInsertionIndex);

                const long long maximumInsertionIndex =
                        static_cast<long long>(
                            instructions.size()
                        );

                const long long clampedInsertionIndex =
                        std::clamp(
                            roundedInsertionIndex,
                            0LL,
                            maximumInsertionIndex
                        );

                pendingInsertionIndex_ =
                        static_cast<std::size_t>(
                            clampedInsertionIndex
                        );
            }
        }

        if (placementModeActive) {
            const std::size_t insertionSlotCount =
                    instructions.size() + 1;

            const float slotMarkerY =
                    origin.y +
                    style_.timelineLabelOffsetY +
                    22.0F;

            for (
                std::size_t candidateIndex = 0;
                candidateIndex < insertionSlotCount;
                ++candidateIndex
            ) {
                const float insertionX =
                        firstInstructionX +
                        gateSpacing *
                        static_cast<float>(candidateIndex);

                const bool selectedSlot =
                        pendingInsertionIndex_.has_value() &&
                        pendingInsertionIndex_.value() == candidateIndex;

                drawList->AddCircleFilled(
                    ImVec2{
                        insertionX,
                        slotMarkerY
                    },
                    selectedSlot ? 4.0F : 2.5F,
                    selectedSlot
                        ? style_.placementGuideColor
                        : style_.inactiveColumnGuideColor
                );
            }
        }

        const std::size_t insertionIndex =
                pendingInsertionIndex_.value_or(instructions.size());

        const float placementX =
                firstInstructionX +
                gateSpacing * static_cast<float>(insertionIndex);

        const float shiftedLastGateX =
                placementModeActive
                    ? lastGateX + gateSpacing
                    : lastGateX;

        const float displayedWireEndX =
                shiftedLastGateX + style_.rightPadding;

        if (placementModeActive) {
            const float guideStartY =
                    firstWireY - style_.columnGuideVerticalPadding;

            const float guideEndY =
                    lastWireY + style_.columnGuideVerticalPadding;

            const float dashStep =
                    style_.placementGuideDashLength +
                    style_.placementGuideGapLength;

            for (float y = guideStartY; y < guideEndY; y += dashStep) {
                const float dashEndY =
                        std::min(y + style_.placementGuideDashLength, guideEndY);


                drawList->AddLine(
                    ImVec2{placementX, y},
                    ImVec2{placementX, dashEndY},
                    style_.placementGuideColor,
                    style_.placementGuideThickness
                );
            }
        }

        const float pulse =
                animationPulse(style_.animationSpeed);

        const ImU32 backgroundColor =
                style_.canvasBackgroundColor;

        // ---------------------------------------------------------
        // Layer 1: subtle instruction-column guides
        // ---------------------------------------------------------

        const bool initialStepHighlighted =
                snapshot.currentStepNumber == 0U;

        drawList->AddLine(
            ImVec2{
                firstGateX,
                firstWireY - style_.columnGuideVerticalPadding
            },
            ImVec2{
                firstGateX,
                lastWireY + style_.columnGuideVerticalPadding
            },
            initialStepHighlighted
                ? style_.activeColumnGuideColor
                : style_.inactiveColumnGuideColor,
            initialStepHighlighted
                ? style_.activeColumnGuideThickness
                : style_.columnGuideThickness
        );

        for (std::size_t instructionIndex = 0; instructionIndex < instructions.size(); ++instructionIndex) {
            std::size_t displayedInstructionIndex =
                    instructionIndex + 1U;

            if (placementModeActive &&
                pendingInsertionIndex_.has_value() &&
                instructionIndex >= pendingInsertionIndex_.value()) {
                ++displayedInstructionIndex;
            }

            const float x =
                    firstGateX + gateSpacing * static_cast<float>(displayedInstructionIndex);

            const bool highlighted =
                    snapshot.currentStepNumber ==
                    instructionIndex + 1U;

            const ImU32 columnGuideColor =
                    highlighted
                        ? style_.activeColumnGuideColor
                        : style_.inactiveColumnGuideColor;

            drawList->AddLine(
                ImVec2{x, firstWireY - style_.columnGuideVerticalPadding},
                ImVec2{x, lastWireY + style_.columnGuideVerticalPadding},
                columnGuideColor,
                highlighted
                    ? style_.activeColumnGuideThickness
                    : style_.columnGuideThickness
            );
        }

        // ---------------------------------------------------------
        // Layer 2: qubit labels and horizontal wires
        // ---------------------------------------------------------

        for (std::size_t qubit = 0; qubit < circuit.qubitCount(); ++qubit) {
            const float y =
                    firstWireY +
                    effectiveWireSpacing_ *
                    static_cast<float>(qubit);

            const std::string label =
                    "q" + std::to_string(qubit);

            drawList->AddText(
                ImVec2{
                    origin.x,
                    y - style_.qubitLabelOffsetY
                },
                style_.qubitLabelColor,
                label.c_str()
            );

            drawList->AddLine(
                ImVec2{wireStartX, y},
                ImVec2{displayedWireEndX, y},
                style_.wireColor,
                style_.wireThickness
            );
        }

        const ImVec2 mousePosition =
                ImGui::GetMousePos();

        const auto beginInstructionDrag =
                [&](const std::size_t instructionIndex, const bool hovered) {
            if (
                !placementModeActive &&
                hovered &&
                ImGui::IsMouseDragging(
                    ImGuiMouseButton_Left,
                    5.0F
                )
            ) {
                draggedInstructionIndex_ =
                        instructionIndex;
                dragDestinationIndex_ =
                        instructionIndex;
            }
        };

        if (singleQubitPlacement) {
            const float placementBandHalfHeight =
                    std::max(
                        16.0F,
                        effectiveWireSpacing_ * 0.47F
                    );

            for (std::size_t qubit = 0; qubit < circuit.qubitCount(); ++qubit) {
                const float y =
                        firstWireY +
                        effectiveWireSpacing_ *
                        static_cast<float>(qubit);

                const bool hovered =
                        ImGui::IsWindowHovered() &&
                        isPointInsideRect(
                            mousePosition,
                            ImVec2{
                                wireStartX,
                                y - placementBandHalfHeight
                            },
                            ImVec2{
                                displayedWireEndX,
                                y + placementBandHalfHeight
                            }
                        );

                const bool clicked =
                        hovered &&
                        (
                            ImGui::IsMouseClicked(
                                ImGuiMouseButton_Left
                            ) ||
                            gateDropDelivered
                        );

                if (hovered) {
                    ImGui::SetMouseCursor(
                        ImGuiMouseCursor_Hand
                    );

                    if (insertionIndex >= instructions.size()) {
                        ImGui::SetTooltip(
                            "Append %s to q%zu",
                            pendingGate->c_str(),
                            qubit
                        );
                    } else {
                        ImGui::SetTooltip(
                            "Insert %s on q%zu before step %zu",
                            pendingGate->c_str(),
                            qubit,
                            insertionIndex + 1U
                        );
                    }
                }

                if (clicked) {
                    // The application consumes this placement next frame and edits the circuit.
                    completedSingleQubitPlacement_ =
                            SingleQubitPlacement{
                                pendingGate.value(), qubit, pendingInsertionIndex_.value_or(instructions.size())
                            };
                }

                if (hovered) {
                    drawList->AddLine(
                        ImVec2{
                            wireStartX,
                            y
                        },
                        ImVec2{
                            displayedWireEndX,
                            y
                        },
                        style_.placementWireColor,
                        style_.wireThickness + 0.8F
                    );

                    drawGate(
                        drawList,
                        ImVec2{placementX, y},
                        std::string{
                            gate_notation::circuitLabel(
                                pendingGate.value()
                            )
                        },
                        false,
                        true,
                        false,
                        true
                    );
                } else {
                    drawList->AddCircle(
                        ImVec2{placementX, y},
                        4.0F,
                        style_.placementGuideColor,
                        16,
                        1.3F
                    );
                }
            }
        }
        if (controlledPlacement) {
            std::optional<std::size_t> hoveredPlacementQubit;
            const float placementBandHalfHeight =
                    std::max(
                        16.0F,
                        effectiveWireSpacing_ * 0.47F
                    );

            for (
                std::size_t qubit = 0;
                qubit < circuit.qubitCount();
                ++qubit
            ) {
                const float y =
                        firstWireY +
                        effectiveWireSpacing_ *
                        static_cast<float>(qubit);

                if (
                    ImGui::IsWindowHovered() &&
                    isPointInsideRect(
                        mousePosition,
                        ImVec2{
                            wireStartX,
                            y - placementBandHalfHeight
                        },
                        ImVec2{
                            displayedWireEndX,
                            y + placementBandHalfHeight
                        }
                    )
                ) {
                    hoveredPlacementQubit =
                            qubit;
                    break;
                }
            }

            if (
                pendingControlQubit_.has_value() &&
                hoveredPlacementQubit.has_value() &&
                hoveredPlacementQubit.value() !=
                    pendingControlQubit_.value()
            ) {
                const float controlY =
                        firstWireY +
                        effectiveWireSpacing_ *
                        static_cast<float>(
                            pendingControlQubit_.value()
                        );

                const float targetY =
                        firstWireY +
                        effectiveWireSpacing_ *
                        static_cast<float>(
                            hoveredPlacementQubit.value()
                        );

                drawList->AddLine(
                    ImVec2{placementX, controlY},
                    ImVec2{placementX, targetY},
                    style_.placementWireColor,
                    style_.controlledConnectionThickness
                );
            }

            for (
                std::size_t qubit = 0;
                qubit < circuit.qubitCount();
                ++qubit
            ) {
                const bool isSelectedTarget =
                        pendingTargetQubit_.has_value() &&
                        pendingTargetQubit_.value() == qubit;

                const bool isSelectedThird =
                        pendingThirdQubit_.has_value() &&
                        pendingThirdQubit_.value() == qubit;
                const float y =
                        firstWireY +
                        effectiveWireSpacing_ *
                        static_cast<float>(qubit);

                const bool hovered =
                        ImGui::IsWindowHovered() &&
                        isPointInsideRect(
                        mousePosition,
                        ImVec2{
                            wireStartX,
                            y - placementBandHalfHeight
                        },
                        ImVec2{
                            displayedWireEndX,
                            y + placementBandHalfHeight
                        }
                        );

                const bool clicked =
                        hovered &&
                        (
                            ImGui::IsMouseClicked(
                                ImGuiMouseButton_Left
                            ) ||
                            gateDropDelivered
                        );

                const bool isSelectedControl =
                        pendingControlQubit_.has_value() &&
                        pendingControlQubit_.value() == qubit;

                const bool choosingTarget =
                        pendingControlQubit_.has_value();

                const bool threeQubitPlacement =
                        isThreeQubitGateName(
                            pendingGate.value()
                        );

                const std::string_view targetPreviewLabel =
                        gate_notation::circuitLabel(
                            pendingGate.value()
                        );

                const bool symmetricInteraction =
                        isInteractionGateName(
                            pendingGate.value()
                        );

                const bool exchangePlacement =
                        pendingGate.value() == "SWAP" ||
                        pendingGate.value() == "iSWAP" ||
                        pendingGate.value() == "sqrtSWAP";

                const char *previewLabel =
                        threeQubitPlacement
                            ? (
                                pendingGate.value() == "CCX"
                                    ? (
                                        isSelectedControl ||
                                        !pendingControlQubit_.has_value()
                                            ? "C1"
                                            : isSelectedTarget ||
                                              !pendingTargetQubit_.has_value()
                                                  ? "C2"
                                                  : "CCX"
                                    )
                                    : (
                                        isSelectedControl ||
                                        !pendingControlQubit_.has_value()
                                            ? "C"
                                            : isSelectedTarget ||
                                              !pendingTargetQubit_.has_value()
                                                  ? "SW1"
                                                  : "SW2"
                                    )
                            )
                            : exchangePlacement
                            ? (
                                isSelectedControl ||
                                !pendingControlQubit_.has_value()
                                    ? "SW1"
                                    : "SW2"
                            )
                            : symmetricInteraction
                            ? targetPreviewLabel.data()
                            : choosingTarget
                            ? isSelectedControl
                                  ? "C"
                                  : targetPreviewLabel.data()
                            : "C";

                if (hovered) {
                    ImGui::SetMouseCursor(
                        ImGuiMouseCursor_Hand
                    );

                    ImGui::SetTooltip(
                        choosingTarget
                            ? "Complete %s on q%zu"
                            : "Start %s on q%zu",
                        pendingGate->c_str(),
                        qubit
                    );
                }

                if (clicked) {
                    if (!pendingControlQubit_.has_value()) {
                        // First click chooses the control/first qubit.
                        pendingControlQubit_ =
                                qubit;
                    } else if (isSelectedControl) {
                        // Clicking the first qubit again cancels that half of placement.
                        pendingControlQubit_.reset();
                        pendingTargetQubit_.reset();
                        pendingThirdQubit_.reset();
                    } else if (
                        threeQubitPlacement &&
                        !pendingTargetQubit_.has_value()
                    ) {
                        pendingTargetQubit_ =
                                qubit;
                    } else if (
                        threeQubitPlacement &&
                        isSelectedTarget
                    ) {
                        pendingTargetQubit_.reset();
                        pendingThirdQubit_.reset();
                    } else if (threeQubitPlacement) {
                        pendingThirdQubit_ =
                                qubit;
                    } else {
                        // Second click chooses the target/second qubit.
                        pendingTargetQubit_ =
                                qubit;
                    }
                }

                if (
                    hovered ||
                    isSelectedControl ||
                    isSelectedTarget ||
                    isSelectedThird
                ) {
                    drawList->AddLine(
                        ImVec2{wireStartX, y},
                        ImVec2{displayedWireEndX, y},
                        style_.placementWireColor,
                        style_.wireThickness + 0.8F
                    );

                    drawGate(
                        drawList,
                        ImVec2{placementX, y},
                        previewLabel,
                        false,
                        hovered,
                        isSelectedControl ||
                            isSelectedTarget ||
                            isSelectedThird,
                        true
                    );
                } else {
                    drawList->AddCircle(
                        ImVec2{placementX, y},
                        4.0F,
                        style_.placementGuideColor,
                        16,
                        1.3F
                    );
                }
            }
        }

        // ---------------------------------------------------------
        // Layer 3: timeline, execution marker and instructions
        // ---------------------------------------------------------

        drawList->AddText(
            ImVec2{
                origin.x,
                origin.y + style_.timelineLabelOffsetY
            },
            style_.inactiveTimelineColor,
            "STEPS"
        );

        const std::size_t stepNumberWidth =
                std::to_string(
                    std::max<std::size_t>(
                        instructions.size(),
                        1U
                    )
                ).size();

        const std::string widestStepLabel(
            stepNumberWidth,
            '0'
        );

        const float stepBadgeWidth =
                std::max(
                    style_.stepBadgeMinimumWidth,
                    ImGui::CalcTextSize(
                        widestStepLabel.c_str()
                    ).x +
                    style_.stepBadgePaddingX * 2.0F
                );

        const std::string initialStepLabel{"0"};

        const ImVec2 initialLabelSize =
                ImGui::CalcTextSize(
                    initialStepLabel.c_str()
                );

        const ImVec2 initialLabelPosition{
            firstGateX - initialLabelSize.x * 0.5F,
            origin.y + style_.timelineLabelOffsetY
        };

        const ImVec2 initialBadgeMin{
            firstGateX - stepBadgeWidth * 0.5F,
            initialLabelPosition.y - style_.stepBadgePaddingY
        };

        const ImVec2 initialBadgeMax{
            firstGateX + stepBadgeWidth * 0.5F,
            initialLabelPosition.y
            + initialLabelSize.y
            + style_.stepBadgePaddingY
        };

        drawList->AddRectFilled(
            initialBadgeMin,
            initialBadgeMax,
            initialStepHighlighted
                ? style_.activeStepBadgeFillColor
                : style_.stepBadgeFillColor,
            style_.stepBadgeCornerRadius
        );

        drawList->AddText(
            initialLabelPosition,
            initialStepHighlighted
                ? activeOrange(style_, pulse)
                : style_.inactiveTimelineColor,
            initialStepLabel.c_str()
        );

        bool initialIdentityHovered = false;

        for (
            std::size_t qubit = 0U;
            qubit < circuit.qubitCount();
            ++qubit
        ) {
            const float y =
                    firstWireY +
                    effectiveWireSpacing_ *
                    static_cast<float>(qubit);

            const bool identityHovered =
                    ImGui::IsWindowHovered() &&
                    isPointInsideRect(
                        mousePosition,
                        ImVec2{
                            firstGateX - style_.gateHalfWidth,
                            y - style_.gateHalfHeight
                        },
                        ImVec2{
                            firstGateX + style_.gateHalfWidth,
                            y + style_.gateHalfHeight
                        }
                    );

            initialIdentityHovered =
                    initialIdentityHovered ||
                    identityHovered;

            drawGate(
                drawList,
                ImVec2{firstGateX, y},
                "I",
                false,
                identityHovered,
                false,
                false
            );
        }

        const bool initialBadgeHovered =
                ImGui::IsWindowHovered() &&
                isPointInsideRect(
                    mousePosition,
                    initialBadgeMin,
                    initialBadgeMax
                );

        const bool initialStepHovered =
                !placementModeActive &&
                (
                    initialBadgeHovered ||
                    initialIdentityHovered
                );

        const bool initialStepDoubleClicked =
                initialStepHovered &&
                ImGui::IsMouseDoubleClicked(
                    ImGuiMouseButton_Left
                );

        const bool initialStepClicked =
                initialStepHovered &&
                ImGui::IsMouseClicked(
                    ImGuiMouseButton_Left
                );

        if (initialStepDoubleClicked) {
            gateClickedThisFrame = true;
            clearSelection();
            requestedStepJumpNumber_ = 0U;
        } else if (initialStepClicked) {
            gateClickedThisFrame = true;
            clearSelection();
        }

        if (initialStepHovered) {
            ImGui::SetMouseCursor(
                ImGuiMouseCursor_Hand
            );

            ImGui::SetTooltip(
                "Step 0 of %zu\nInitial state\nIdentity on all qubits",
                instructions.size()
            );
        }

        if (initialStepHighlighted) {
            drawList->AddLine(
                ImVec2{
                    firstGateX,
                    origin.y + style_.executionStemStartOffsetY
                },
                ImVec2{
                    firstGateX,
                    firstWireY - style_.executionStemEndGap
                },
                activeOrange(
                    style_,
                    pulse,
                    style_.executionStemAlpha
                ),
                style_.executionStemThickness
            );
        }

        for (std::size_t instructionIndex = 0;
             instructionIndex < instructions.size();
             ++instructionIndex) {
            const circuit::CircuitInstructionInfo &instruction =
                    instructions[instructionIndex];

            std::size_t displayedInstructionIndex =
                    instructionIndex + 1U;

            if (
                placementModeActive &&
                pendingInsertionIndex_.has_value() &&
                instructionIndex >= pendingInsertionIndex_.value()
            ) {
                ++displayedInstructionIndex;
            }

            const float x =
                    firstGateX + gateSpacing * static_cast<float>(displayedInstructionIndex);

            const bool highlighted =
                    snapshot.currentStepNumber ==
                    instructionIndex + 1U;

            const ImU32 instructionColor =
                    highlighted
                        ? activeOrange(style_, pulse)
                        : style_.inactiveTimelineColor;


            const std::string instructionLabel =
                    std::to_string(instructionIndex + 1);

            const ImVec2 labelSize =
                    ImGui::CalcTextSize(
                        instructionLabel.c_str()
                    );

            const ImVec2 labelPosition{
                x - labelSize.x / 2.0F,
                origin.y + style_.timelineLabelOffsetY
            };

            const ImVec2 badgeMin{
                x - stepBadgeWidth * 0.5F,
                labelPosition.y - style_.stepBadgePaddingY
            };

            const ImVec2 badgeMax{
                x + stepBadgeWidth * 0.5F,
                labelPosition.y
                + labelSize.y
                + style_.stepBadgePaddingY
            };

            drawList->AddRectFilled(
                badgeMin,
                badgeMax,
                highlighted
                    ? style_.activeStepBadgeFillColor
                    : style_.stepBadgeFillColor,
                style_.stepBadgeCornerRadius);

            drawList->AddText(
                labelPosition,
                instructionColor,
                instructionLabel.c_str()
            );

            const bool stepBadgeHovered =
                    ImGui::IsWindowHovered() &&
                    isPointInsideRect(
                        mousePosition,
                        badgeMin,
                        badgeMax
                    );

            const bool stepBadgeClicked =
                    stepBadgeHovered &&
                    !placementModeActive &&
                    ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            const bool stepBadgeDoubleClicked =
                    stepBadgeHovered &&
                    !placementModeActive &&
                    ImGui::IsMouseDoubleClicked(
                        ImGuiMouseButton_Left
                    );

            if (stepBadgeDoubleClicked) {
                gateClickedThisFrame = true;
                setSingleInstructionSelection(
                    instructionIndex
                );
                requestedStepJumpNumber_ =
                        instructionIndex + 1U;
            } else if (stepBadgeClicked) {
                // Step badges select gates without requiring a precise gate click.
                gateClickedThisFrame = true;
                updateInstructionSelection(
                    instructionIndex
                );
            }

            if (stepBadgeHovered) {
                ImGui::SetMouseCursor(
                    ImGuiMouseCursor_Hand
                );

                showInstructionTooltip(
                    instruction,
                    instructionIndex,
                    instructions.size()
                );
            }

            // Short execution stem. It stops before the first wire.
            if (highlighted) {
                drawList->AddLine(
                    ImVec2{x, origin.y + style_.executionStemStartOffsetY},
                    ImVec2{x, firstWireY - style_.executionStemEndGap},
                    activeOrange(style_, pulse, style_.executionStemAlpha),
                    style_.executionStemThickness
                );
            }

            ImU32 gateColor =
                    highlighted
                        ? activeOrange(style_, pulse)
                        : style_.controlledGateColor;

            // -----------------------------------------------------
            // Compact three-qubit gates such as CCX and CSWAP
            // -----------------------------------------------------

            if (
                instruction.controlQubit.has_value() &&
                instruction.secondaryTargetQubit.has_value() &&
                instruction.tertiaryTargetQubit.has_value()
            ) {
                const float firstY =
                        firstWireY +
                        effectiveWireSpacing_ *
                        static_cast<float>(
                            instruction.controlQubit.value()
                        );

                const float secondY =
                        firstWireY +
                        effectiveWireSpacing_ *
                        static_cast<float>(
                            instruction.secondaryTargetQubit.value()
                        );

                const float thirdY =
                        firstWireY +
                        effectiveWireSpacing_ *
                        static_cast<float>(
                            instruction.tertiaryTargetQubit.value()
                        );

                const bool isControlledSwap =
                        instruction.name == "CSWAP";

                const float exchangeMinimumY =
                        std::min(secondY, thirdY);

                const float exchangeMaximumY =
                        std::max(secondY, thirdY);

                const float exchangeCenterY =
                        (exchangeMinimumY + exchangeMaximumY) * 0.5F;

                const float exchangeHalfWidth =
                        style_.exchangePathHalfWidth;

                const float minimumY =
                        std::min({
                            firstY,
                            secondY,
                            thirdY
                        });

                const float maximumY =
                        std::max({
                            firstY,
                            secondY,
                            thirdY
                        });

                const float hitHalfWidth =
                        isControlledSwap
                            ? std::max(
                                style_.gateHalfWidth,
                                exchangeHalfWidth
                            )
                            : style_.gateHalfWidth;

                const ImVec2 hitMinimum{
                    x -
                    hitHalfWidth -
                    style_.selectedGateOutlinePadding,
                    minimumY -
                    style_.gateHalfHeight -
                    style_.selectedGateOutlinePadding
                };

                const ImVec2 hitMaximum{
                    x +
                    hitHalfWidth +
                    style_.selectedGateOutlinePadding,
                    maximumY +
                    style_.gateHalfHeight +
                    style_.selectedGateOutlinePadding
                };

                const bool hovered =
                        ImGui::IsWindowHovered() &&
                        isPointInsideRect(
                            mousePosition,
                            hitMinimum,
                            hitMaximum
                        );

                if (hovered) {
                    ImGui::SetMouseCursor(
                        ImGuiMouseCursor_Hand
                    );

                    showInstructionTooltip(
                        instruction,
                        instructionIndex,
                        instructions.size()
                    );
                }

                beginInstructionDrag(
                    instructionIndex,
                    hovered
                );

                const bool clicked =
                        !placementModeActive &&
                        hovered &&
                        ImGui::IsMouseClicked(
                            ImGuiMouseButton_Left
                        );

                const bool doubleClicked =
                        !placementModeActive &&
                        hovered &&
                        ImGui::IsMouseDoubleClicked(
                            ImGuiMouseButton_Left
                        );

                if (doubleClicked) {
                    gateClickedThisFrame = true;
                    setSingleInstructionSelection(
                        instructionIndex
                    );
                    requestedStepJumpNumber_ =
                            instructionIndex + 1U;
                } else if (clicked) {
                    gateClickedThisFrame = true;
                    updateInstructionSelection(
                        instructionIndex
                    );
                }

                const bool selected =
                        isInstructionSelected(
                            instructionIndex
                        );

                if (!highlighted && hovered) {
                    gateColor =
                            style_.hoveredControlledGateColor;
                }

                for (const float operandY : {firstY, secondY, thirdY}) {
                    drawList->AddRectFilled(
                        ImVec2{
                            x - style_.symbolGapHalfWidth,
                            operandY - style_.symbolGapHalfHeight
                        },
                        ImVec2{
                            x + style_.symbolGapHalfWidth,
                            operandY + style_.symbolGapHalfHeight
                        },
                        backgroundColor
                    );
                }

                if (isControlledSwap) {
                    // Exchange paths own the complete target ports. Clearing
                    // their full width prevents straight-wire stubs at each fork.
                    for (const float targetY : {secondY, thirdY}) {
                        drawList->AddRectFilled(
                            ImVec2{
                                x - exchangeHalfWidth,
                                targetY - style_.symbolGapHalfHeight
                            },
                            ImVec2{
                                x + exchangeHalfWidth,
                                targetY + style_.symbolGapHalfHeight
                            },
                            backgroundColor
                        );
                    }
                }

                if (selected) {
                    drawList->AddRect(
                        hitMinimum,
                        hitMaximum,
                        style_.selectedGateOutlineColor,
                        style_.selectedGateOutlineCornerRadius,
                        0,
                        style_.selectedGateOutlineThickness
                    );
                }

                const float connectionMinimumY =
                        isControlledSwap
                            ? std::min(firstY, exchangeCenterY)
                            : minimumY;

                const float connectionMaximumY =
                        isControlledSwap
                            ? std::max(firstY, exchangeCenterY)
                            : maximumY;

                if (highlighted) {
                    const int glowAlpha =
                            static_cast<int>(
                                style_.controlledGlowBaseAlpha +
                                pulse *
                                style_.controlledGlowPulseAlpha
                            );

                    drawList->AddLine(
                        ImVec2{x, connectionMinimumY},
                        ImVec2{x, connectionMaximumY},
                        withAlpha(
                            style_.controlledGlowColor,
                            glowAlpha
                        ),
                        style_.controlledGlowLineThickness
                    );
                }

                drawList->AddLine(
                    ImVec2{x, connectionMinimumY},
                    ImVec2{x, connectionMaximumY},
                    gateColor,
                    style_.controlledConnectionThickness
                );

                drawList->AddCircleFilled(
                    ImVec2{x, firstY},
                    style_.controlRadius,
                    gateColor
                );

                if (instruction.name == "CCX") {
                    drawList->AddCircleFilled(
                        ImVec2{x, secondY},
                        style_.controlRadius,
                        gateColor
                    );

                    drawGate(
                        drawList,
                        ImVec2{x, thirdY},
                        std::string{
                            gate_notation::circuitLabel(
                                instruction.name
                            )
                        },
                        highlighted,
                        hovered,
                        false,
                        false
                    );
                } else {
                    const ImVec2 upperLeft{
                        x - exchangeHalfWidth,
                        exchangeMinimumY
                    };

                    const ImVec2 upperRight{
                        x + exchangeHalfWidth,
                        exchangeMinimumY
                    };

                    const ImVec2 lowerLeft{
                        x - exchangeHalfWidth,
                        exchangeMaximumY
                    };

                    const ImVec2 lowerRight{
                        x + exchangeHalfWidth,
                        exchangeMaximumY
                    };

                    const auto drawExchangePaths =
                            [&](const ImU32 color, const float thickness) {
                        drawList->AddLine(
                            upperLeft,
                            lowerRight,
                            color,
                            thickness
                        );

                        drawList->AddLine(
                            lowerLeft,
                            upperRight,
                            color,
                            thickness
                        );
                    };

                    const std::size_t secondQubit =
                            instruction.secondaryTargetQubit.value();

                    const std::size_t thirdQubit =
                            instruction.tertiaryTargetQubit.value();

                    const std::size_t targetSeparation =
                            secondQubit > thirdQubit
                                ? secondQubit - thirdQubit
                                : thirdQubit - secondQubit;

                    if (targetSeparation > 1U) {
                        // Opaque underlays create clear overpasses where long
                        // exchange paths cross unrelated qubit wires.
                        drawInsetLine(
                            drawList,
                            upperLeft,
                            lowerRight,
                            backgroundColor,
                            style_.exchangePathUnderlayThickness,
                            style_.exchangePathUnderlayInset
                        );

                        drawInsetLine(
                            drawList,
                            lowerLeft,
                            upperRight,
                            backgroundColor,
                            style_.exchangePathUnderlayThickness,
                            style_.exchangePathUnderlayInset
                        );
                    }

                    if (highlighted) {
                        const int glowAlpha =
                                static_cast<int>(
                                    style_.controlledGlowBaseAlpha +
                                    pulse *
                                    style_.controlledGlowPulseAlpha
                                );

                        drawExchangePaths(
                            withAlpha(
                                style_.controlledGlowColor,
                                glowAlpha
                            ),
                            style_.controlledGlowLineThickness
                        );

                        drawExchangePortJoins(
                            drawList,
                            upperLeft,
                            upperRight,
                            lowerLeft,
                            lowerRight,
                            withAlpha(
                                style_.controlledGlowColor,
                                glowAlpha
                            ),
                            style_.controlledGlowLineThickness,
                            style_.exchangePortOverlap
                        );
                    }

                    drawExchangePaths(
                        gateColor,
                        style_.controlledConnectionThickness
                    );

                    drawExchangePortJoins(
                        drawList,
                        upperLeft,
                        upperRight,
                        lowerLeft,
                        lowerRight,
                        gateColor,
                        style_.controlledConnectionThickness,
                        style_.exchangePortOverlap
                    );

                    drawExchangeBadge(
                        drawList,
                        ImVec2{x, exchangeCenterY},
                        gate_notation::exchangeBadge(
                            instruction.name
                        ).data(),
                        backgroundColor,
                        gateColor,
                        style_
                    );
                }

                continue;
            }

            // -----------------------------------------------------
            // Controlled gate such as CX
            // -----------------------------------------------------

            if (instruction.controlQubit.has_value() &&
                instruction.secondaryTargetQubit.has_value()) {
                const float controlY = firstWireY + effectiveWireSpacing_ * static_cast<float>(instruction.controlQubit.
                                           value());

                const float targetY = firstWireY + effectiveWireSpacing_ * static_cast<float>(instruction.
                                          secondaryTargetQubit.value());

                const ImVec2 controlCenter{
                    x,
                    controlY
                };

                const ImVec2 targetCenter{
                    x,
                    targetY
                };

                const float controlledMinY =
                        std::min(controlY, targetY);

                const float controlledMaxY =
                        std::max(controlY, targetY);

                const float controlledHitHalfWidth =
                        std::max(style_.gateHalfWidth, style_.targetRadius) +
                        style_.selectedGateOutlinePadding;

                const float controlledHitPaddingY =
                        style_.selectedGateOutlinePadding +
                        style_.targetRadius;

                const ImVec2 controlledHitMin{
                    x - controlledHitHalfWidth,
                    controlledMinY - controlledHitPaddingY
                };

                const ImVec2 controlledHitMax{
                    x + controlledHitHalfWidth,
                    controlledMaxY + controlledHitPaddingY
                };

                const bool hovered =
                        ImGui::IsWindowHovered() &&
                        isPointInsideRect(
                            mousePosition,
                            controlledHitMin,
                            controlledHitMax
                        );

                if (hovered) {
                    ImGui::SetMouseCursor(
                        ImGuiMouseCursor_Hand
                    );

                    showInstructionTooltip(
                        instruction,
                        instructionIndex,
                        instructions.size()
                    );
                }

                beginInstructionDrag(
                    instructionIndex,
                    hovered
                );

                const bool clicked =
                        !placementModeActive && hovered &&
                        ImGui::IsMouseClicked(ImGuiMouseButton_Left);

                const bool doubleClicked =
                        !placementModeActive &&
                        hovered &&
                        ImGui::IsMouseDoubleClicked(
                            ImGuiMouseButton_Left
                        );

                if (doubleClicked) {
                    gateClickedThisFrame = true;
                    setSingleInstructionSelection(
                        instructionIndex
                    );

                    requestedStepJumpNumber_ =
                            instructionIndex + 1U;
                } else if (clicked) {
                    gateClickedThisFrame = true;
                    updateInstructionSelection(
                        instructionIndex
                    );
                }

                const bool selected =
                        isInstructionSelected(
                            instructionIndex
                        );

                const bool isSwap =
                        instruction.name == "SWAP";

                const bool isISwap =
                        instruction.name == "iSWAP";

                const bool isSqrtSwap =
                        instruction.name == "sqrtSWAP";

                const bool isSwapFamily =
                        isSwap ||
                        isISwap ||
                        isSqrtSwap;

                const bool isInteraction =
                        isInteractionGateName(
                            instruction.name
                        );

                if (!highlighted && hovered) {
                    gateColor = style_.hoveredControlledGateColor;
                }

                // Remove the horizontal wire beneath both symbols.
                drawList->AddRectFilled(
                    ImVec2{
                        x - style_.symbolGapHalfWidth,
                        controlY - style_.symbolGapHalfHeight
                    },
                    ImVec2{
                        x + style_.symbolGapHalfWidth,
                        controlY + style_.symbolGapHalfHeight
                    },
                    backgroundColor
                );

                drawList->AddRectFilled(
                    ImVec2{
                        x - style_.symbolGapHalfWidth,
                        targetY - style_.symbolGapHalfHeight
                    },
                    ImVec2{
                        x + style_.symbolGapHalfWidth,
                        targetY + style_.symbolGapHalfHeight
                    },
                    backgroundColor
                );

                if (isSwapFamily) {
                    const float exchangeHalfWidth =
                            style_.exchangePathHalfWidth;

                    const ImVec2 upperLeft{
                        x - exchangeHalfWidth,
                        controlledMinY
                    };

                    const ImVec2 upperRight{
                        x + exchangeHalfWidth,
                        controlledMinY
                    };

                    const ImVec2 lowerLeft{
                        x - exchangeHalfWidth,
                        controlledMaxY
                    };

                    const ImVec2 lowerRight{
                        x + exchangeHalfWidth,
                        controlledMaxY
                    };

                    // Clear both wire segments so the diagonals read as exchanged paths.
                    for (const float endpointY : {controlledMinY, controlledMaxY}) {
                        drawList->AddRectFilled(
                            ImVec2{
                                x - exchangeHalfWidth,
                                endpointY -
                                style_.symbolGapHalfHeight
                            },
                            ImVec2{
                                x + exchangeHalfWidth,
                                endpointY +
                                style_.symbolGapHalfHeight
                            },
                            backgroundColor
                        );
                    }

                    const auto drawExchangePaths =
                            [&](const ImU32 color, const float thickness) {
                        drawList->AddLine(
                            upperLeft,
                            lowerRight,
                            color,
                            thickness
                        );

                        drawList->AddLine(
                            lowerLeft,
                            upperRight,
                            color,
                            thickness
                        );
                    };

                    const std::size_t controlQubit =
                            instruction.controlQubit.value();

                    const std::size_t targetQubit =
                            instruction.secondaryTargetQubit.value();

                    const std::size_t targetSeparation =
                            controlQubit > targetQubit
                                ? controlQubit - targetQubit
                                : targetQubit - controlQubit;

                    if (targetSeparation > 1U) {
                        drawInsetLine(
                            drawList,
                            upperLeft,
                            lowerRight,
                            backgroundColor,
                            style_.exchangePathUnderlayThickness,
                            style_.exchangePathUnderlayInset
                        );

                        drawInsetLine(
                            drawList,
                            lowerLeft,
                            upperRight,
                            backgroundColor,
                            style_.exchangePathUnderlayThickness,
                            style_.exchangePathUnderlayInset
                        );
                    }

                    if (selected) {
                        const float horizontalPadding =
                                style_.selectedGateOutlinePadding;

                        const float verticalPadding =
                                style_.selectedGateOutlinePadding;

                        drawList->AddRect(
                            ImVec2{
                                x -
                                exchangeHalfWidth -
                                horizontalPadding,
                                controlledMinY -
                                verticalPadding
                            },
                            ImVec2{
                                x +
                                exchangeHalfWidth +
                                horizontalPadding,
                                controlledMaxY +
                                verticalPadding
                            },
                            style_.selectedGateOutlineColor,
                            style_.selectedGateOutlineCornerRadius,
                            0,
                            style_.selectedGateOutlineThickness
                        );
                    }

                    // Glow follows the SWAP geometry instead of producing circles.
                    if (highlighted) {
                        const int glowAlpha =
                                static_cast<int>(
                                    style_.controlledGlowBaseAlpha +
                                    pulse * style_.controlledGlowPulseAlpha
                                );

                        const ImU32 glowColor =
                                withAlpha(style_.controlledGlowColor, glowAlpha);

                        drawExchangePaths(
                            glowColor,
                            style_.controlledGlowLineThickness
                        );

                        drawExchangePortJoins(
                            drawList,
                            upperLeft,
                            upperRight,
                            lowerLeft,
                            lowerRight,
                            glowColor,
                            style_.controlledGlowLineThickness,
                            style_.exchangePortOverlap
                        );
                    }

                    drawExchangePaths(
                        gateColor,
                        style_.controlledConnectionThickness
                    );

                    drawExchangePortJoins(
                        drawList,
                        upperLeft,
                        upperRight,
                        lowerLeft,
                        lowerRight,
                        gateColor,
                        style_.controlledConnectionThickness,
                        style_.exchangePortOverlap
                    );

                    drawExchangeBadge(
                        drawList,
                        ImVec2{
                            x,
                            (
                                controlledMinY +
                                controlledMaxY
                            ) * 0.5F
                        },
                        gate_notation::exchangeBadge(
                            instruction.name
                        ).data(),
                        backgroundColor,
                        gateColor,
                        style_
                    );

                    continue;
                }

                if (isInteraction) {
                    const char *axisLabel =
                            gate_notation::circuitLabel(
                                instruction.name
                            ).data();

                    const auto drawInteractionBox =
                            [&](const ImVec2 &center, const ImU32 color) {
                        const ImVec2 minimum{
                            center.x - style_.gateHalfWidth,
                            center.y - style_.gateHalfHeight
                        };

                        const ImVec2 maximum{
                            center.x + style_.gateHalfWidth,
                            center.y + style_.gateHalfHeight
                        };

                        drawList->AddRectFilled(
                            minimum,
                            maximum,
                            backgroundColor,
                            style_.gateCornerRadius
                        );

                        drawList->AddRect(
                            minimum,
                            maximum,
                            color,
                            style_.gateCornerRadius,
                            0,
                            style_.controlledConnectionThickness
                        );

                        const ImVec2 labelSize =
                                ImGui::CalcTextSize(
                                    axisLabel
                                );

                        drawList->AddText(
                            ImVec2{
                                center.x - labelSize.x * 0.5F,
                                center.y - labelSize.y * 0.5F
                            },
                            color,
                            axisLabel
                        );
                    };

                    const float direction =
                            targetY >= controlY
                                ? 1.0F
                                : -1.0F;

                    const ImVec2 connectionStart{
                        x,
                        controlY +
                        direction * style_.gateHalfHeight
                    };

                    const ImVec2 connectionEnd{
                        x,
                        targetY -
                        direction * style_.gateHalfHeight
                    };

                    if (selected) {
                        drawList->AddLine(
                            controlCenter,
                            targetCenter,
                            style_.selectedGateOutlineColor,
                            style_.controlledConnectionThickness +
                            style_.selectedGateOutlineThickness * 2.0F
                        );

                        for (const ImVec2 center : {controlCenter, targetCenter}) {
                            drawList->AddRect(
                                ImVec2{
                                    center.x -
                                    style_.gateHalfWidth -
                                    style_.selectedGateOutlinePadding,
                                    center.y -
                                    style_.gateHalfHeight -
                                    style_.selectedGateOutlinePadding
                                },
                                ImVec2{
                                    center.x +
                                    style_.gateHalfWidth +
                                    style_.selectedGateOutlinePadding,
                                    center.y +
                                    style_.gateHalfHeight +
                                    style_.selectedGateOutlinePadding
                                },
                                style_.selectedGateOutlineColor,
                                style_.selectedGateOutlineCornerRadius,
                                0,
                                style_.selectedGateOutlineThickness
                            );
                        }
                    }

                    if (highlighted) {
                        const int glowAlpha =
                                static_cast<int>(
                                    style_.controlledGlowBaseAlpha +
                                    pulse *
                                    style_.controlledGlowPulseAlpha
                                );

                        const ImU32 glowColor =
                                withAlpha(
                                    style_.controlledGlowColor,
                                    glowAlpha
                                );

                        drawList->AddLine(
                            connectionStart,
                            connectionEnd,
                            glowColor,
                            style_.controlledGlowLineThickness
                        );
                    }

                    drawList->AddLine(
                        connectionStart,
                        connectionEnd,
                        gateColor,
                        style_.controlledConnectionThickness
                    );

                    drawInteractionBox(
                        controlCenter,
                        gateColor
                    );

                    drawInteractionBox(
                        targetCenter,
                        gateColor
                    );

                    continue;
                }

                const float direction =
                        targetY >= controlY
                            ? 1.0F
                            : -1.0F;

                // Stop the vertical connection at the symbol edges.
                const ImVec2 connectionStart{
                    x,
                    controlY +
                    direction * style_.controlRadius
                };

                const ImVec2 connectionEnd{
                    x,
                    targetY -
                    direction * style_.gateHalfHeight
                };

                // Selection is drawn behind the controlled gate.
                if (selected) {
                    drawList->AddLine(
                        controlCenter,
                        targetCenter,
                        style_.selectedGateOutlineColor,
                        style_.controlledConnectionThickness +
                        style_.selectedGateOutlineThickness * 2.0F
                    );

                    drawList->AddRect(
                        ImVec2{
                            targetCenter.x -
                            style_.gateHalfWidth -
                            style_.selectedGateOutlinePadding,

                            targetCenter.y -
                            style_.gateHalfHeight -
                            style_.selectedGateOutlinePadding
                        },
                        ImVec2{
                            targetCenter.x +
                            style_.gateHalfWidth +
                            style_.selectedGateOutlinePadding,

                            targetCenter.y +
                            style_.gateHalfHeight +
                            style_.selectedGateOutlinePadding
                        },
                        style_.selectedGateOutlineColor,
                        style_.selectedGateOutlineCornerRadius,
                        0,
                        style_.selectedGateOutlineThickness
                    );

                    drawList->AddCircle(
                        targetCenter,
                        style_.targetRadius +
                        style_.selectedGateOutlinePadding,
                        style_.selectedGateOutlineColor,
                        32,
                        style_.selectedGateOutlineThickness
                    );
                }

                // Glow first, so solid shapes remain crisp.
                if (highlighted) {
                    const int glowAlpha =
                            static_cast<int>(style_.controlledGlowBaseAlpha + pulse * style_.controlledGlowPulseAlpha);

                    const ImU32 glowColor =
                            withAlpha(style_.controlledGlowColor, glowAlpha);

                    drawList->AddLine(
                        connectionStart,
                        connectionEnd,
                        glowColor,
                        style_.controlledGlowLineThickness
                    );

                    drawList->AddCircleFilled(
                        controlCenter,
                        style_.controlGlowRadius,
                        glowColor
                    );

                    drawList->AddRectFilled(
                        ImVec2{
                            targetCenter.x -
                            style_.gateHalfWidth -
                            style_.targetGlowPadding,

                            targetCenter.y -
                            style_.gateHalfHeight -
                            style_.targetGlowPadding
                        },
                        ImVec2{
                            targetCenter.x +
                            style_.gateHalfWidth +
                            style_.targetGlowPadding,

                            targetCenter.y +
                            style_.gateHalfHeight +
                            style_.targetGlowPadding
                        },
                        glowColor,
                        style_.gateCornerRadius
                    );
                }

                // Solid controlled-gate connection.
                drawList->AddLine(
                    connectionStart,
                    connectionEnd,
                    gateColor,
                    style_.controlledConnectionThickness
                );

                // Control dot.
                drawList->AddCircleFilled(
                    controlCenter,
                    style_.controlRadius,
                    gateColor
                );

                const char *targetLabel =
                        gate_notation::circuitLabel(
                            instruction.name
                        ).data();

                const ImVec2 targetBoxMin{
                    targetCenter.x - style_.gateHalfWidth,
                    targetCenter.y - style_.gateHalfHeight
                };

                const ImVec2 targetBoxMax{
                    targetCenter.x + style_.gateHalfWidth,
                    targetCenter.y + style_.gateHalfHeight
                };

                // Background fill.
                drawList->AddRectFilled(
                    targetBoxMin,
                    targetBoxMax,
                    backgroundColor,
                    style_.gateCornerRadius
                );

                // Purple outline.
                drawList->AddRect(
                    targetBoxMin,
                    targetBoxMax,
                    gateColor,
                    style_.gateCornerRadius,
                    0,
                    style_.controlledConnectionThickness
                );

                // Center the X, Y, or Z.
                const ImVec2 targetLabelSize =
                        ImGui::CalcTextSize(targetLabel);

                drawList->AddText(
                    ImVec2{
                        targetCenter.x - targetLabelSize.x * 0.5F,
                        targetCenter.y - targetLabelSize.y * 0.5F
                    },
                    gateColor,
                    targetLabel
                );

                continue;
            }

            // -----------------------------------------------------
            // Normal single-qubit gate
            // -----------------------------------------------------

            if (!instruction.targetQubit.has_value()) {
                continue;
            }

            const float y = firstWireY + effectiveWireSpacing_ * static_cast<float>(instruction.targetQubit.value());

            const std::string circuitLabel{
                gate_notation::circuitLabel(
                    instruction.name
                )
            };

            const float gateHalfWidth =
                    gateHalfWidthForLabel(
                        style_,
                        circuitLabel
                    );

            drawList->AddRectFilled(
                ImVec2{x - gateHalfWidth - 1.0F, y - style_.wireGapHalfHeight},
                ImVec2{x + gateHalfWidth + 1.0F, y + style_.wireGapHalfHeight},
                backgroundColor
            );

            const bool hovered =
                    ImGui::IsWindowHovered() &&
                    isPointInsideRect(
                        mousePosition,
                        ImVec2{
                            x - gateHalfWidth,
                            y - style_.gateHalfHeight
                        },
                        ImVec2{
                            x + gateHalfWidth,
                            y + style_.gateHalfHeight
                        }
                    );

            if (hovered) {
                ImGui::SetMouseCursor(
                    ImGuiMouseCursor_Hand
                );

                showInstructionTooltip(
                    instruction,
                    instructionIndex,
                    instructions.size()
                );
            }

            beginInstructionDrag(
                instructionIndex,
                hovered
            );

            const bool clicked =
                    hovered &&
                    ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            const bool doubleClicked =
                    hovered &&
                    !placementModeActive &&
                    ImGui::IsMouseDoubleClicked(
                        ImGuiMouseButton_Left
                    );

            if (doubleClicked) {
                gateClickedThisFrame = true;
                setSingleInstructionSelection(
                    instructionIndex
                );

                requestedStepJumpNumber_ =
                        instructionIndex + 1U;
            } else if (clicked) {
                gateClickedThisFrame = true;
                updateInstructionSelection(
                    instructionIndex
                );
            }

            const bool selected =
                    isInstructionSelected(
                        instructionIndex
                    );

            drawGate(
                drawList,
                ImVec2{x, y},
                circuitLabel,
                highlighted,
                hovered,
                selected,
                false
            );

            if (instruction.angleRadians.has_value()) {
                const std::string angleLabel =
                        quantum_sim::gui::notation::formatRadians(
                            instruction.angleRadians.value()
                        );

                const ImVec2 angleLabelSize =
                        ImGui::CalcTextSize(
                            angleLabel.c_str()
                        );

                drawList->AddText(
                    ImVec2{
                        x - angleLabelSize.x * 0.5F,
                        y - style_.gateHalfHeight - angleLabelSize.y - 3.0F
                    },
                    instructionColor,
                    angleLabel.c_str()
                );
            }
        }

        if (
            draggedInstructionIndex_.has_value() &&
            !instructions.empty()
        ) {
            if (
                ImGui::IsMouseDown(
                    ImGuiMouseButton_Left
                )
            ) {
                const float rawDestination =
                        (
                            mousePosition.x -
                            firstInstructionX
                        ) /
                        gateSpacing;

                const long long roundedDestination =
                        std::llround(rawDestination);

                dragDestinationIndex_ =
                        static_cast<std::size_t>(
                            std::clamp(
                                roundedDestination,
                                0LL,
                                static_cast<long long>(
                                    instructions.size() - 1U
                                )
                            )
                        );

                const float destinationX =
                        firstInstructionX +
                        gateSpacing *
                        static_cast<float>(
                            dragDestinationIndex_.value()
                        );

                drawList->AddLine(
                    ImVec2{
                        destinationX,
                        firstWireY -
                        style_.columnGuideVerticalPadding
                    },
                    ImVec2{
                        destinationX,
                        lastWireY +
                        style_.columnGuideVerticalPadding
                    },
                    style_.reorderGuideColor,
                    2.2F
                );

                ImGui::SetTooltip(
                    "Move step %zu to step %zu",
                    draggedInstructionIndex_.value() + 1U,
                    dragDestinationIndex_.value() + 1U
                );
            } else {
                if (
                    dragDestinationIndex_.has_value() &&
                    dragDestinationIndex_.value() !=
                        draggedInstructionIndex_.value()
                ) {
                    completedInstructionMove_ =
                            InstructionMove{
                                draggedInstructionIndex_.value(),
                                dragDestinationIndex_.value()
                            };

                    setSingleInstructionSelection(
                        dragDestinationIndex_.value()
                    );

                    requestedFocusStepNumber_ =
                            dragDestinationIndex_.value() + 1U;
                }

                draggedInstructionIndex_.reset();
                dragDestinationIndex_.reset();
            }
        }

        const float circuitContentWidth =
                displayedWireEndX
                - origin.x
                + style_.canvasPaddingX;

        ImGui::Dummy(
            ImVec2{
                circuitContentWidth,
                circuitContentHeight
            }
        );

        const bool activeStepNeedsFocus =
                requestedFocusStepNumber_.has_value() ||
                !lastFocusedStepNumber_.has_value() ||
                lastFocusedStepNumber_.value() !=
                    snapshot.currentStepNumber;

        if (activeStepNeedsFocus) {
            const std::size_t activeStepNumber =
                    requestedFocusStepNumber_.value_or(
                        std::min(
                            snapshot.currentStepNumber,
                            instructions.size()
                        )
                    );

            const float firstGateContentX =
                    firstGateX -
                    canvasMin.x +
                    ImGui::GetScrollX();

            const float activeStepContentX =
                    firstGateContentX +
                    gateSpacing *
                    static_cast<float>(activeStepNumber);

            const float centeredScrollX =
                    activeStepContentX -
                    ImGui::GetWindowWidth() * 0.52F;

            ImGui::SetScrollX(
                std::max(
                    centeredScrollX,
                    0.0F
                )
            );

            lastFocusedStepNumber_ =
                    snapshot.currentStepNumber;

            lastFocusedInstructionCount_ =
                    instructions.size();

            requestedFocusStepNumber_.reset();
        }

        if (
            ImGui::IsWindowHovered() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !gateClickedThisFrame
        ) {
            clearSelection();
        }

        ImGui::EndChild();
    }

    CircuitRenderer::CircuitRenderer(CircuitStyle style)
        : style_{std::move(style)} {
    }

    const CircuitStyle &CircuitRenderer::style() const noexcept {
        return style_;
    }

    void CircuitRenderer::setStyle(CircuitStyle style) {
        style_ = std::move(style);
    }

    std::optional<std::size_t> CircuitRenderer::selectedInstructionIndex() const noexcept {
        return selectedInstructionIndex_;
    }

    const std::vector<std::size_t> &
    CircuitRenderer::selectedInstructionIndices() const noexcept {
        return selectedInstructionIndices_;
    }

    std::optional<std::size_t> CircuitRenderer::consumeStepJumpRequest() noexcept {
        const std::optional<std::size_t> request =
                requestedStepJumpNumber_;

        requestedStepJumpNumber_.reset();
        return request;
    }

    void CircuitRenderer::clearSelection() noexcept {
        selectedInstructionIndex_.reset();
        selectedInstructionIndices_.clear();
        selectionAnchorIndex_.reset();
    }

    std::optional<ControlledPlacement> CircuitRenderer::completedControlledPlacement() const noexcept {
        if (!pendingControlledGateName_.has_value() ||
            isThreeQubitGateName(pendingControlledGateName_.value()) ||
            !pendingControlQubit_.has_value() ||
            !pendingTargetQubit_.has_value() ||
            !pendingInsertionIndex_.has_value()) {
            return std::nullopt;
        }

        return ControlledPlacement{
            pendingControlledGateName_.value(),
            pendingControlQubit_.value(),
            pendingTargetQubit_.value(),
            pendingInsertionIndex_.value()
        };
    }

    std::optional<ControlledPlacement> CircuitRenderer::consumeCompletedControlledPlacement() noexcept {
        const auto placement =
                completedControlledPlacement();

        if (!placement.has_value()) {
            return std::nullopt;
        }

        // Reset only the two-click state; pending gate is owned by GuiApplication.
        pendingControlQubit_.reset();
        pendingTargetQubit_.reset();

        return placement;
    }

    std::optional<ThreeQubitPlacement> CircuitRenderer::consumeCompletedThreeQubitPlacement() noexcept {
        if (
            !pendingControlledGateName_.has_value() ||
            !isThreeQubitGateName(
                pendingControlledGateName_.value()
            ) ||
            !pendingControlQubit_.has_value() ||
            !pendingTargetQubit_.has_value() ||
            !pendingThirdQubit_.has_value() ||
            !pendingInsertionIndex_.has_value()
        ) {
            return std::nullopt;
        }

        const ThreeQubitPlacement placement{
            pendingControlledGateName_.value(),
            pendingControlQubit_.value(),
            pendingTargetQubit_.value(),
            pendingThirdQubit_.value(),
            pendingInsertionIndex_.value()
        };

        pendingControlQubit_.reset();
        pendingTargetQubit_.reset();
        pendingThirdQubit_.reset();

        return placement;
    }

    std::optional<SingleQubitPlacement> CircuitRenderer::consumeCompletedSingleQubitPlacement() {
        if (!completedSingleQubitPlacement_.has_value()) {
            return std::nullopt;
        }

        std::optional<SingleQubitPlacement> placement =
                std::move(completedSingleQubitPlacement_);

        // Consuming transfers ownership to GuiApplication.
        completedSingleQubitPlacement_.reset();

        return placement;
    }

    bool CircuitRenderer::hasPendingControlQubit() const noexcept {
        return pendingControlQubit_.has_value();
    }

    std::size_t CircuitRenderer::placementOperandCount() const noexcept {
        return static_cast<std::size_t>(
                   pendingControlQubit_.has_value()
               ) +
               static_cast<std::size_t>(
                   pendingTargetQubit_.has_value()
               ) +
               static_cast<std::size_t>(
                   pendingThirdQubit_.has_value()
               );
    }

    void CircuitRenderer::cancelPlacement() noexcept {
        pendingControlledGateName_.reset();
        pendingControlQubit_.reset();
        pendingTargetQubit_.reset();
        pendingThirdQubit_.reset();
        pendingInsertionIndex_.reset();
        insertionMouseXLock_.reset();
        completedSingleQubitPlacement_.reset();
    }

    std::optional<std::size_t> CircuitRenderer::pendingInsertionIndex() const noexcept {
        return pendingInsertionIndex_;
    }

    std::optional<InstructionMove> CircuitRenderer::consumeInstructionMoveRequest() noexcept {
        const std::optional<InstructionMove> request =
                completedInstructionMove_;

        completedInstructionMove_.reset();
        return request;
    }

    void CircuitRenderer::continuePlacementAfter(
        const std::size_t insertedInstructionIndex
    ) noexcept {
        pendingInsertionIndex_ =
                insertedInstructionIndex + 1U;
        insertionMouseXLock_ =
                ImGui::GetMousePos().x;

        pendingControlQubit_.reset();
        pendingTargetQubit_.reset();
        pendingThirdQubit_.reset();
        requestedFocusStepNumber_ =
                insertedInstructionIndex + 1U;
    }

    void CircuitRenderer::selectInstruction(
        const std::size_t instructionIndex
    ) {
        setSingleInstructionSelection(
            instructionIndex
        );
        requestedFocusStepNumber_ =
                instructionIndex + 1U;
    }

    void CircuitRenderer::selectInstructions(
        std::vector<std::size_t> instructionIndices
    ) {
        std::sort(
            instructionIndices.begin(),
            instructionIndices.end()
        );

        instructionIndices.erase(
            std::unique(
                instructionIndices.begin(),
                instructionIndices.end()
            ),
            instructionIndices.end()
        );

        selectedInstructionIndices_ =
                std::move(instructionIndices);

        if (selectedInstructionIndices_.empty()) {
            selectedInstructionIndex_.reset();
            selectionAnchorIndex_.reset();
            return;
        }

        selectedInstructionIndex_ =
                selectedInstructionIndices_.back();

        selectionAnchorIndex_ =
                selectedInstructionIndex_;

        requestedFocusStepNumber_ =
                selectedInstructionIndex_.value() + 1U;
    }

    bool CircuitRenderer::isInstructionSelected(
        const std::size_t instructionIndex
    ) const noexcept {
        return std::binary_search(
            selectedInstructionIndices_.begin(),
            selectedInstructionIndices_.end(),
            instructionIndex
        );
    }

    void CircuitRenderer::updateInstructionSelection(
        const std::size_t instructionIndex
    ) {
        const ImGuiIO &io =
                ImGui::GetIO();

        if (io.KeyShift) {
            const std::size_t anchor =
                    selectionAnchorIndex_.value_or(
                        selectedInstructionIndex_.value_or(
                            instructionIndex
                        )
                    );

            if (!io.KeyCtrl) {
                selectedInstructionIndices_.clear();
            }

            const std::size_t first =
                    std::min(anchor, instructionIndex);

            const std::size_t last =
                    std::max(anchor, instructionIndex);

            for (
                std::size_t index = first;
                index <= last;
                ++index
            ) {
                selectedInstructionIndices_.push_back(index);
            }

            std::sort(
                selectedInstructionIndices_.begin(),
                selectedInstructionIndices_.end()
            );

            selectedInstructionIndices_.erase(
                std::unique(
                    selectedInstructionIndices_.begin(),
                    selectedInstructionIndices_.end()
                ),
                selectedInstructionIndices_.end()
            );

            selectedInstructionIndex_ =
                    instructionIndex;
            return;
        }

        if (io.KeyCtrl) {
            const auto existing =
                    std::lower_bound(
                        selectedInstructionIndices_.begin(),
                        selectedInstructionIndices_.end(),
                        instructionIndex
                    );

            if (
                existing != selectedInstructionIndices_.end() &&
                *existing == instructionIndex
            ) {
                selectedInstructionIndices_.erase(existing);
            } else {
                selectedInstructionIndices_.insert(
                    existing,
                    instructionIndex
                );
            }

            selectedInstructionIndex_ =
                    selectedInstructionIndices_.empty()
                        ? std::nullopt
                        : std::optional<std::size_t>{
                            selectedInstructionIndices_.back()
                        };

            selectionAnchorIndex_ =
                    instructionIndex;
            return;
        }

        if (
            selectedInstructionIndices_.size() == 1U &&
            selectedInstructionIndices_.front() ==
                instructionIndex
        ) {
            clearSelection();
            return;
        }

        setSingleInstructionSelection(
            instructionIndex
        );
    }

    void CircuitRenderer::setSingleInstructionSelection(
        const std::size_t instructionIndex
    ) {
        selectedInstructionIndex_ =
                instructionIndex;

        selectedInstructionIndices_ = {
            instructionIndex
        };

        selectionAnchorIndex_ =
                instructionIndex;
    }

    void CircuitRenderer::requestFocusStep(
        const std::size_t stepNumber
    ) noexcept {
        requestedFocusStepNumber_ =
                stepNumber;
    }

    void CircuitRenderer::zoomIn() noexcept {
        fitToWindow_ = false;
        viewZoom_ =
                std::min(
                    viewZoom_ + 0.12F,
                    1.80F
                );
    }

    void CircuitRenderer::zoomOut() noexcept {
        fitToWindow_ = false;
        viewZoom_ =
                std::max(
                    viewZoom_ - 0.12F,
                    0.55F
                );
    }

    void CircuitRenderer::fitToView() noexcept {
        fitToWindow_ = true;
        viewZoom_ = 1.0F;
        requestedFocusStepNumber_ = 0U;
    }

    float CircuitRenderer::viewZoom() const noexcept {
        return viewZoom_;
    }

    bool CircuitRenderer::isFittingToView() const noexcept {
        return fitToWindow_;
    }

    void CircuitRenderer::drawGate(ImDrawList *drawList, const ImVec2 &center, const std::string &label,
                                   const bool highlighted, const bool hovered, const bool selected,
                                   bool placementPreview
    ) {
        const float gateHalfWidth =
                gateHalfWidthForLabel(
                    style_,
                    label
                );

        const ImVec2 topLeft{
            center.x - gateHalfWidth,
            center.y - style_.gateHalfHeight
        };
        const ImVec2 bottomRight{
            center.x + gateHalfWidth,
            center.y + style_.gateHalfHeight
        };

        const float pulse =
                animationPulse(style_.animationSpeed);

        const ImU32 fillColor =
                highlighted
                    ? activeOrange(style_, pulse)
                    : hovered
                          ? style_.hoveredGateFillColor
                          : placementPreview
                                ? style_.placementPreviewFillColor
                                : style_.gateFillColor;

        const ImU32 outlineColor =
                highlighted
                    ? style_.activeGateOutlineColor
                    : hovered
                          ? style_.hoveredGateOutlineColor
                          : placementPreview
                                ? style_.placementPreviewOutlineColor
                                : style_.inactiveGateColor;

        if (selected) {
            drawList->AddRect(
                ImVec2{
                    topLeft.x - style_.selectedGateOutlinePadding,
                    topLeft.y - style_.selectedGateOutlinePadding
                },
                ImVec2{
                    bottomRight.x + style_.selectedGateOutlinePadding,
                    bottomRight.y + style_.selectedGateOutlinePadding
                },
                style_.selectedGateOutlineColor,
                style_.selectedGateOutlineCornerRadius,
                0,
                style_.selectedGateOutlineThickness
            );
        }

        // Draw glow behind the solid gate.
        if (highlighted) {
            const int glowAlpha =
                    static_cast<int>(style_.gateGlowBaseAlpha + pulse * style_.gateGlowPulseAlpha);

            drawList->AddRectFilled(
                ImVec2{
                    topLeft.x - style_.outerGateGlowPadding,
                    topLeft.y - style_.outerGateGlowPadding
                },
                ImVec2{
                    bottomRight.x + style_.outerGateGlowPadding,
                    bottomRight.y + style_.outerGateGlowPadding
                },
                withAlpha(style_.outerGateGlowColor, glowAlpha),
                style_.outerGateGlowCornerRadius
            );

            drawList->AddRectFilled(
                ImVec2{
                    topLeft.x - style_.innerGateGlowPadding,
                    topLeft.y - style_.innerGateGlowPadding
                },
                ImVec2{
                    bottomRight.x + style_.innerGateGlowPadding,
                    bottomRight.y + style_.innerGateGlowPadding
                },
                withAlpha(
                    style_.innerGateGlowColor,
                    glowAlpha
                    + static_cast<int>(style_.innerGateGlowAlphaBoost)),
                style_.innerGateGlowCornerRadius
            );
        }

        drawList->AddRectFilled(
            topLeft,
            bottomRight,
            fillColor,
            style_.gateCornerRadius
        );

        drawList->AddRect(
            topLeft,
            bottomRight,
            outlineColor,
            style_.gateCornerRadius,
            0,
            style_.gateOutlineThickness
        );

        ImFont *font =
                ImGui::GetFont();

        const float baseFontSize =
                ImGui::GetFontSize();

        const ImVec2 baseTextSize =
                ImGui::CalcTextSize(
                    label.c_str()
                );

        const float availableTextWidth =
                std::max(
                    1.0F,
                    gateHalfWidth *
                    2.0F -
                    style_.gateLabelPaddingX *
                    2.0F
                );

        const float textScale =
                baseTextSize.x > availableTextWidth
                    ? availableTextWidth /
                        baseTextSize.x
                    : 1.0F;

        const float fittedFontSize =
                baseFontSize *
                textScale;

        const ImVec2 textSize =
                font->CalcTextSizeA(
                    fittedFontSize,
                    10000.0F,
                    0.0F,
                    label.c_str()
                );

        const ImVec2 textPosition{
            center.x - textSize.x / 2.0F,
            center.y - textSize.y / 2.0F
        };

        drawList->AddText(
            font,
            fittedFontSize,
            textPosition,
            style_.gateTextColor,
            label.c_str()
        );
    }
}

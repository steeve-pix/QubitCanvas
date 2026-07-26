#include "quantum_sim/gui/rendering/CircuitRenderer.hpp"
#include "quantum_sim/gui/QuantumNotation.hpp"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
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

        if (placementModeActive) {
            // Default new placements to append until the mouse picks another slot.
            if (!pendingInsertionIndex_.has_value()) {
                pendingInsertionIndex_ =
                        instructions.size();
            }
        } else {
            pendingInsertionIndex_.reset();
        }

        const bool controlledPlacement =
                placementModeActive &&
                (
                    pendingGate.value() == "CX" ||
                    pendingGate.value() == "CY" ||
                    pendingGate.value() == "CZ" ||
                    pendingGate.value() == "SWAP" ||
                    pendingGate.value() == "iSWAP"
                );

        if (controlledPlacement) {
            // Remember the family so a completed two-click placement can be emitted.
            pendingControlledGateName_ =
                    pendingGate.value();
        } else {
            pendingControlledGateName_.reset();
            pendingControlQubit_.reset();
            pendingTargetQubit_.reset();
        }

        const bool singleQubitPlacement =
                placementModeActive &&
                !controlledPlacement;

        const std::size_t qubitCount =
                std::max<std::size_t>(
                    circuit.qubitCount(),
                    1
                );

        const float calculatedContentHeight = style_.canvasPaddingY + style_.topMargin + style_.wireSpacing *
                                              static_cast<float>(qubitCount > 0 ? qubitCount - 1 : 0) + style_.
                                              gateHalfHeight + style_.canvasPaddingY;

        const float circuitContentHeight =
                std::max(calculatedContentHeight, style_.minimumCanvasHeight);

        const std::size_t instructionCount =
                std::max<std::size_t>(instructions.size(), 1);

        const float wireStartX =
                origin.x + style_.wireStartOffset;

        const float firstGateX =
                wireStartX + style_.firstGateOffset;

        const float availableWidth =
                ImGui::GetContentRegionAvail().x;

        const float usableGateWidth = availableWidth - style_.wireStartOffset - style_.firstGateOffset - style_.
                                      rightPadding;

        float gateSpacing = style_.gateSpacing;

        if (instructionCount > 1) {
            const float fittedSpacing = usableGateWidth / static_cast<float>(instructionCount - 1);

            gateSpacing =
                    std::clamp(fittedSpacing, style_.minimumGateSpacing, style_.gateSpacing);
        }

        const float lastGateX =
                firstGateX +
                gateSpacing *
                static_cast<float>(instructionCount - 1);

        const float firstWireY =
                origin.y + style_.topMargin;

        const float lastWireY =
                firstWireY +
                style_.wireSpacing *
                static_cast<float>(qubitCount - 1);

        if (
            placementModeActive &&
            ImGui::IsWindowHovered()
        ) {
            // Convert mouse X position into the nearest instruction insertion slot.
            const ImVec2 insertionMousePosition =
                    ImGui::GetMousePos();

            const float relativeMouseX =
                    insertionMousePosition.x - firstGateX;

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
                        firstGateX +
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
                firstGateX + gateSpacing * static_cast<float>(insertionIndex);

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

        for (std::size_t instructionIndex = 0; instructionIndex < instructions.size(); ++instructionIndex) {
            std::size_t displayedInstructionIndex =
                    instructionIndex;

            if (placementModeActive &&
                pendingInsertionIndex_.has_value() &&
                instructionIndex >= pendingInsertionIndex_.value()) {
                ++displayedInstructionIndex;
            }

            const float x =
                    firstGateX + gateSpacing * static_cast<float>(displayedInstructionIndex);

            const bool highlighted =
                    instructionIndex ==
                    snapshot.currentStepIndex;

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
                    style_.wireSpacing *
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

        if (singleQubitPlacement) {
            for (std::size_t qubit = 0; qubit < circuit.qubitCount(); ++qubit) {
                const float y =
                        firstWireY +
                        style_.wireSpacing *
                        static_cast<float>(qubit);

                const bool hovered =
                        ImGui::IsWindowHovered() &&
                        isPointInsideRect(
                            mousePosition,
                            ImVec2{
                                placementX - style_.gateHalfWidth,
                                y - style_.gateHalfHeight
                            },
                            ImVec2{
                                placementX + style_.gateHalfWidth,
                                y + style_.gateHalfHeight
                            }
                        );

                const bool clicked =
                        hovered &&
                        ImGui::IsMouseClicked(
                            ImGuiMouseButton_Left
                        );

                if (hovered) {
                    ImGui::SetMouseCursor(
                        ImGuiMouseCursor_Hand
                    );
                }

                if (clicked) {
                    // The application consumes this placement next frame and edits the circuit.
                    completedSingleQubitPlacement_ =
                            SingleQubitPlacement{
                                pendingGate.value(), qubit, pendingInsertionIndex_.value_or(instructions.size())
                            };
                }

                drawGate(
                    drawList,
                    ImVec2{placementX, y},
                    pendingGate.value(),
                    false,
                    hovered,
                    false, true
                );
            }
        }
        if (controlledPlacement) {
            for (
                std::size_t qubit = 0;
                qubit < circuit.qubitCount();
                ++qubit
            ) {
                const bool isSelectedTarget =
                        pendingTargetQubit_.has_value() &&
                        pendingTargetQubit_.value() == qubit;
                const float y =
                        firstWireY +
                        style_.wireSpacing *
                        static_cast<float>(qubit);

                const bool hovered =
                        ImGui::IsWindowHovered() &&
                        isPointInsideRect(
                            mousePosition,
                            ImVec2{
                                placementX - style_.gateHalfWidth,
                                y - style_.gateHalfHeight
                            },
                            ImVec2{
                                placementX + style_.gateHalfWidth,
                                y + style_.gateHalfHeight
                            }
                        );

                const bool clicked =
                        hovered &&
                        ImGui::IsMouseClicked(
                            ImGuiMouseButton_Left
                        );

                const bool isSelectedControl =
                        pendingControlQubit_.has_value() &&
                        pendingControlQubit_.value() == qubit;

                const bool choosingTarget =
                        pendingControlQubit_.has_value();

                const char *targetPreviewLabel = "T";

                if (pendingGate.value() == "CX") {
                    targetPreviewLabel = "X";
                } else if (pendingGate.value() == "CY") {
                    targetPreviewLabel = "Y";
                } else if (pendingGate.value() == "CZ") {
                    targetPreviewLabel = "Z";
                }

                const char *previewLabel =
                        choosingTarget
                            ? isSelectedControl
                                  ? "C"
                                  : targetPreviewLabel
                            : "C";

                if (hovered) {
                    ImGui::SetMouseCursor(
                        ImGuiMouseCursor_Hand
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
                    } else {
                        // Second click chooses the target/second qubit.
                        pendingTargetQubit_ =
                                qubit;
                    }
                }

                drawGate(
                    drawList,
                    ImVec2{placementX, y},
                    previewLabel,
                    false,
                    hovered,
                    isSelectedControl || isSelectedTarget, true
                );
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

        for (std::size_t instructionIndex = 0;
             instructionIndex < instructions.size();
             ++instructionIndex) {
            const circuit::CircuitInstructionInfo &instruction =
                    instructions[instructionIndex];

            std::size_t displayedInstructionIndex =
                    instructionIndex;

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
                    instructionIndex ==
                    snapshot.currentStepIndex;

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

            if (stepBadgeClicked) {
                // Step badges select gates without requiring a precise gate click.
                selectedInstructionIndex_ = instructionIndex;
            }

            if (stepBadgeHovered) {
                ImGui::SetMouseCursor(
                    ImGuiMouseCursor_Hand
                );

                if (instruction.angleRadians.has_value()) {
                    const std::string angleText =
                            quantum_sim::gui::notation::formatRadians(
                                instruction.angleRadians.value()
                            );

                    ImGui::SetTooltip(
                        "Step %zu of %zu\nGate: %s\nAngle: %s",
                        instructionIndex + 1,
                        instructions.size(),
                        instruction.name.c_str(),
                        angleText.c_str()
                    );
                } else {
                    ImGui::SetTooltip(
                        "Step %zu of %zu\nGate: %s",
                        instructionIndex + 1,
                        instructions.size(),
                        instruction.name.c_str()
                    );
                }
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
            // Controlled gate such as CX
            // -----------------------------------------------------

            if (instruction.controlQubit.has_value() &&
                instruction.secondaryTargetQubit.has_value()) {
                const float controlY = firstWireY + style_.wireSpacing * static_cast<float>(instruction.controlQubit.
                                           value());

                const float targetY = firstWireY + style_.wireSpacing * static_cast<float>(instruction.
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
                }

                const bool clicked =
                        !placementModeActive && hovered &&
                        ImGui::IsMouseClicked(ImGuiMouseButton_Left);

                if (clicked) {
                    gateClickedThisFrame = true;

                    if (selectedInstructionIndex_.has_value() &&
                        selectedInstructionIndex_.value() == instructionIndex) {
                        selectedInstructionIndex_.reset();
                    } else {
                        selectedInstructionIndex_ = instructionIndex;
                    }
                }

                const bool selected =
                        selectedInstructionIndex_.has_value() &&
                        selectedInstructionIndex_.value() == instructionIndex;

                const bool isSwap =
                        instruction.name == "SWAP";

                const bool isISwap =
                        instruction.name == "iSWAP";

                const bool isSwapFamily =
                        isSwap || isISwap;

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
                    const float crossRadius =
                            style_.targetRadius * 0.9F;

                    // SWAP-style gates use two crosses instead of a control dot and box.
                    const auto drawSwapCross =
                            [&](const ImVec2 &center, const ImU32 color, const float thickness) {
                        drawList->AddLine(
                            ImVec2{center.x - crossRadius, center.y - crossRadius},
                            ImVec2{center.x + crossRadius, center.y + crossRadius},
                            color,
                            thickness
                        );

                        drawList->AddLine(
                            ImVec2{center.x - crossRadius, center.y + crossRadius},
                            ImVec2{center.x + crossRadius, center.y - crossRadius},
                            color,
                            thickness
                        );
                    };

                    // One clean outline around the entire SWAP instruction.
                    // Do not draw circular endpoint outlines.
                    if (selected) {
                        const float horizontalPadding =
                                style_.selectedGateOutlinePadding + 4.0F;

                        const float verticalPadding =
                                style_.selectedGateOutlinePadding + 4.0F;

                        drawList->AddRect(
                            ImVec2{
                                x - crossRadius - horizontalPadding,
                                controlledMinY - crossRadius - verticalPadding
                            },
                            ImVec2{
                                x + crossRadius + horizontalPadding,
                                controlledMaxY + crossRadius + verticalPadding
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

                        drawList->AddLine(
                            controlCenter,
                            targetCenter,
                            glowColor,
                            style_.controlledGlowLineThickness
                        );

                        drawSwapCross(
                            controlCenter,
                            glowColor,
                            style_.controlledGlowLineThickness
                        );

                        drawSwapCross(
                            targetCenter,
                            glowColor,
                            style_.controlledGlowLineThickness
                        );
                    }

                    // Draw the connection first so the crosses stay crisp.
                    drawList->AddLine(
                        controlCenter,
                        targetCenter,
                        gateColor,
                        style_.controlledConnectionThickness
                    );

                    drawSwapCross(
                        controlCenter,
                        gateColor,
                        style_.controlledConnectionThickness
                    );

                    drawSwapCross(
                        targetCenter,
                        gateColor,
                        style_.controlledConnectionThickness
                    );

                    if (isISwap) {
                        const char *iSwapLabel = "i";

                        const float iScale = 1.8F;

                        const ImVec2 baseSize =
                                ImGui::CalcTextSize(iSwapLabel);

                        const ImVec2 midpoint{
                            controlCenter.x,
                            (
                                controlCenter.y +
                                targetCenter.y
                            ) * 0.5F
                        };

                        drawList->AddText(
                            ImGui::GetFont(),
                            ImGui::GetFontSize() * iScale,
                            ImVec2{
                                midpoint.x + 7.0F,
                                midpoint.y - (baseSize.y * iScale) * 0.5F
                            },
                            gateColor,
                            iSwapLabel
                        );
                    }

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

                const char *targetLabel = "?";

                if (instruction.name == "CX") {
                    targetLabel = "X";
                } else if (instruction.name == "CY") {
                    targetLabel = "Y";
                } else if (instruction.name == "CZ") {
                    targetLabel = "Z";
                }

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

            const float y = firstWireY + style_.wireSpacing * static_cast<float>(instruction.targetQubit.value());

            drawList->AddRectFilled(
                ImVec2{x - style_.wireGapHalfWidth, y - style_.wireGapHalfHeight},
                ImVec2{x + style_.wireGapHalfWidth, y + style_.wireGapHalfHeight},
                backgroundColor
            );

            const bool hovered =
                    ImGui::IsWindowHovered() &&
                    isPointInsideRect(
                        mousePosition,
                        ImVec2{
                            x - style_.gateHalfWidth,
                            y - style_.gateHalfHeight
                        },
                        ImVec2{
                            x + style_.gateHalfWidth,
                            y + style_.gateHalfHeight
                        }
                    );

            if (hovered) {
                ImGui::SetMouseCursor(
                    ImGuiMouseCursor_Hand
                );

                if (instruction.angleRadians.has_value()) {
                    const std::string angleText =
                            quantum_sim::gui::notation::formatRadians(
                                instruction.angleRadians.value()
                            );

                    ImGui::SetTooltip(
                        "%s\nAngle: %s",
                        instruction.name.c_str(),
                        angleText.c_str()
                    );
                }
            }

            const bool clicked =
                    hovered &&
                    ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            if (clicked) {
                gateClickedThisFrame = true;
                if (
                    selectedInstructionIndex_.has_value() &&
                    selectedInstructionIndex_.value() == instructionIndex
                ) {
                    selectedInstructionIndex_.reset();
                } else {
                    selectedInstructionIndex_ =
                            instructionIndex;
                }
            }

            const bool selected =
                    selectedInstructionIndex_.has_value() &&
                    selectedInstructionIndex_.value() == instructionIndex;

            drawGate(drawList, ImVec2{x, y}, instruction.name, highlighted, hovered, selected, false);

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
                !instructions.empty() &&
                (
                    !lastFocusedStepIndex_.has_value() ||
                    lastFocusedStepIndex_.value() != snapshot.currentStepIndex ||
                    lastFocusedInstructionCount_ != instructions.size()
                );

        if (activeStepNeedsFocus) {
            const std::size_t activeStepIndex =
                    std::min(
                        snapshot.currentStepIndex,
                        instructions.size() - 1U
                    );

            const float firstGateContentX =
                    firstGateX -
                    canvasMin.x +
                    ImGui::GetScrollX();

            const float activeStepContentX =
                    firstGateContentX +
                    gateSpacing *
                    static_cast<float>(activeStepIndex);

            const float centeredScrollX =
                    activeStepContentX -
                    ImGui::GetWindowWidth() * 0.52F;

            ImGui::SetScrollX(
                std::max(
                    centeredScrollX,
                    0.0F
                )
            );

            lastFocusedStepIndex_ =
                    snapshot.currentStepIndex;

            lastFocusedInstructionCount_ =
                    instructions.size();
        }

        if (
            ImGui::IsWindowHovered() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !gateClickedThisFrame
        ) {
            selectedInstructionIndex_.reset();
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

    void CircuitRenderer::clearSelection() noexcept {
        selectedInstructionIndex_.reset();
    }

    std::optional<ControlledPlacement> CircuitRenderer::completedControlledPlacement() const noexcept {
        if (!pendingControlledGateName_.has_value() ||
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

    void CircuitRenderer::cancelPlacement() noexcept {
        pendingControlledGateName_.reset();
        pendingControlQubit_.reset();
        pendingTargetQubit_.reset();
        pendingInsertionIndex_.reset();
        completedSingleQubitPlacement_.reset();
    }

    std::optional<std::size_t> CircuitRenderer::pendingInsertionIndex() const noexcept {
        return pendingInsertionIndex_;
    }

    void CircuitRenderer::drawGate(ImDrawList *drawList, const ImVec2 &center, const std::string &label,
                                   const bool highlighted, const bool hovered, const bool selected,
                                   bool placementPreview
    ) {
        const ImVec2 topLeft{
            center.x - style_.gateHalfWidth,
            center.y - style_.gateHalfHeight
        };
        const ImVec2 bottomRight{
            center.x + style_.gateHalfWidth,
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

        const ImVec2 textSize =
                ImGui::CalcTextSize(label.c_str());

        const ImVec2 textPosition{
            center.x - textSize.x / 2.0F,
            center.y - textSize.y / 2.0F
        };

        drawList->AddText(
            textPosition,
            style_.gateTextColor,
            label.c_str()
        );
    }
}

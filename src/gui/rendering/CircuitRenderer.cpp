#include "quantum_sim/gui/rendering/CircuitRenderer.hpp"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <optional>

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
        const debug::DebuggerSnapshot &snapshot, const std::optional<std::string> &pendingGate
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

        const bool controlledPlacement =
                placementModeActive &&
                pendingGate.value() == "CX";

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

        const float wireEndX =
                lastGateX + style_.rightPadding;

        const float firstWireY =
                origin.y + style_.topMargin;

        const float lastWireY = firstWireY + style_.wireSpacing * static_cast<float>(qubitCount - 1);

        const float pulse =
                animationPulse(style_.animationSpeed);

        const ImU32 backgroundColor =
                style_.canvasBackgroundColor;

        // ---------------------------------------------------------
        // Layer 1: subtle instruction-column guides
        // ---------------------------------------------------------

        for (std::size_t instructionIndex = 0;
             instructionIndex < instructions.size();
             ++instructionIndex) {
            const float x = firstGateX + gateSpacing * static_cast<float>(instructionIndex);

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
                ImVec2{wireEndX, y},
                style_.wireColor,
                style_.wireThickness
            );
        }

        const float placementX =
                wireEndX + style_.gateSpacing;

        const ImVec2 mousePosition =
                ImGui::GetMousePos();

        if (singleQubitPlacement) {
            for (
                std::size_t qubit = 0;
                qubit < circuit.qubitCount();
                ++qubit
            ) {
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

                if (hovered) {
                    ImGui::SetMouseCursor(
                        ImGuiMouseCursor_Hand
                    );
                }

                drawGate(
                    drawList,
                    ImVec2{placementX, y},
                    pendingGate.value(),
                    false,
                    hovered,
                    false
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

                const char *previewLabel =
                        pendingControlQubit_.has_value()
                            ? isSelectedControl
                                  ? "C"
                                  : "T"
                            : "C";

                if (hovered) {
                    ImGui::SetMouseCursor(
                        ImGuiMouseCursor_Hand
                    );
                }

                if (clicked) {
                    if (!pendingControlQubit_.has_value()) {
                        pendingControlQubit_ =
                                qubit;
                    } else if (isSelectedControl) {
                        pendingControlQubit_.reset();
                        pendingTargetQubit_.reset();
                        pendingControlQubit_.reset();
                    } else {
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
                    isSelectedControl || isSelectedTarget
                );
            }
        }

        // ---------------------------------------------------------
        // Layer 3: timeline, execution marker and instructions
        // ---------------------------------------------------------

        for (std::size_t instructionIndex = 0;
             instructionIndex < instructions.size();
             ++instructionIndex) {
            const circuit::CircuitInstructionInfo &instruction =
                    instructions[instructionIndex];

            const float x = firstGateX + gateSpacing * static_cast<float>(instructionIndex);

            const bool highlighted =
                    instructionIndex ==
                    snapshot.currentStepIndex;

            const ImU32 instructionColor =
                    highlighted
                        ? activeOrange(style_, pulse)
                        : style_.inactiveTimelineColor;

            const std::string instructionLabel =
                    std::to_string(instructionIndex);

            const ImVec2 labelSize =
                    ImGui::CalcTextSize(
                        instructionLabel.c_str()
                    );

            drawList->AddText(
                ImVec2{
                    x - labelSize.x / 2.0F,
                    origin.y + style_.timelineLabelOffsetY
                },
                instructionColor,
                instructionLabel.c_str()
            );

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
                        : style_.inactiveGateColor;

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
                        hovered &&
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

                if (!highlighted && hovered) {
                    gateColor = style_.hoveredGateOutlineColor;
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
                    direction * style_.targetRadius
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

                    drawList->AddCircle(
                        controlCenter,
                        style_.controlRadius +
                        style_.selectedGateOutlinePadding,
                        style_.selectedGateOutlineColor,
                        32,
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

                    drawList->AddCircleFilled(
                        targetCenter,
                        style_.targetRadius + style_.targetGlowPadding,
                        glowColor
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

                // Target circle.
                drawList->AddCircle(
                    targetCenter,
                    style_.targetRadius,
                    gateColor,
                    32,
                    style_.controlledConnectionThickness
                );

                // Target horizontal cross-line.
                drawList->AddLine(
                    ImVec2{
                        targetCenter.x -
                        style_.targetRadius + style_.targetCrossInset,
                        targetCenter.y
                    },
                    ImVec2{
                        targetCenter.x +
                        style_.targetRadius - style_.targetCrossInset,
                        targetCenter.y
                    },
                    gateColor,
                    style_.controlledConnectionThickness
                );

                // Target vertical cross-line.
                drawList->AddLine(
                    ImVec2{
                        targetCenter.x,
                        targetCenter.y -
                        style_.targetRadius + style_.targetCrossInset
                    },
                    ImVec2{
                        targetCenter.x,
                        targetCenter.y +
                        style_.targetRadius - style_.targetCrossInset
                    },
                    gateColor,
                    style_.controlledConnectionThickness
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

            drawGate(drawList, ImVec2{x, y}, instruction.name, highlighted, hovered, selected);
        }

        const float circuitContentWidth =
                lastGateX - origin.x + style_.rightPadding + style_.canvasPaddingX;

        ImGui::Dummy(
            ImVec2{
                circuitContentWidth,
                circuitContentHeight
            }
        );

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

    bool CircuitRenderer::hasCompletedControlledPlacement() const noexcept {
        return pendingControlQubit_.has_value()&&
            pendingTargetQubit_.has_value();
    }

    void CircuitRenderer::drawGate(ImDrawList *drawList, const ImVec2 &center, const std::string &label,
                                   const bool highlighted, const bool hovered, const bool selected
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
                          : style_.gateFillColor;

        const ImU32 outlineColor =
                highlighted
                    ? style_.activeGateOutlineColor
                    : hovered
                          ? style_.hoveredGateOutlineColor
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

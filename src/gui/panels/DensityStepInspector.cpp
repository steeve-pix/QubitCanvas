#include "quantum_sim/gui/panels/DensityStepInspector.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace quantum_sim::gui {
    void DensityStepInspector::draw(
        const debug::DebuggerSnapshot &snapshot,
        const ImVec2 &imageOrigin,
        const ImVec2 &imageSize
    ) {
        if (imageSize.x < 300.0F || imageSize.y < 130.0F) {
            return;
        }

        const auto &state =
                snapshot.afterState.get();

        const std::size_t qubitCount =
                state.qubitCount();

        const circuit::CircuitInstructionInfo *instruction =
                snapshot.instruction.has_value()
                    ? &snapshot.instruction->get()
                    : nullptr;

        std::vector<std::size_t> displayedQubits;

        const auto appendQubit =
                [&displayedQubits, qubitCount](
                    const std::optional<std::size_t> qubit
                ) {
                    if (!qubit.has_value() || qubit.value() >= qubitCount) {
                        return;
                    }

                    if (
                        std::find(
                            displayedQubits.begin(),
                            displayedQubits.end(),
                            qubit.value()
                        ) == displayedQubits.end()
                    ) {
                        displayedQubits.push_back(qubit.value());
                    }
                };

        if (instruction != nullptr) {
            appendQubit(instruction->controlQubit);
            appendQubit(instruction->targetQubit);
            appendQubit(instruction->secondaryTargetQubit);
        }

        for (
            std::size_t qubit = 0U;
            qubit < qubitCount && displayedQubits.size() < 3U;
            ++qubit
        ) {
            appendQubit(qubit);
        }

        std::sort(displayedQubits.begin(), displayedQubits.end());

        const float panelWidth =
                std::clamp(imageSize.x * 0.31F, 238.0F, 292.0F);

        const float rowHeight = 18.0F;

        const float panelHeight =
                67.0F +
                rowHeight * static_cast<float>(displayedQubits.size());

        const ImVec2 minimum{
            imageOrigin.x + 12.0F,
            imageOrigin.y + 12.0F
        };

        const ImVec2 maximum{
            minimum.x + panelWidth,
            minimum.y + panelHeight
        };

        ImDrawList *drawList =
                ImGui::GetWindowDrawList();

        drawList->AddRectFilled(
            minimum,
            maximum,
            IM_COL32(5, 12, 23, 238),
            6.0F
        );

        drawList->AddRect(
            minimum,
            maximum,
            IM_COL32(50, 101, 145, 220),
            6.0F,
            0,
            1.0F
        );

        std::ostringstream header;
        header
                << "STEP "
                << snapshot.currentStepNumber
                << "/"
                << snapshot.stepCount
                << "  "
                << (instruction == nullptr ? "I" : instruction->name);

        if (instruction == nullptr) {
            header << "  ALL QUBITS";
        } else if (instruction->controlQubit.has_value()) {
            header
                    << "  q"
                    << instruction->controlQubit.value()
                    << " -> q"
                    << instruction->secondaryTargetQubit.value_or(
                        instruction->targetQubit.value_or(0U)
                    );
        } else if (instruction->targetQubit.has_value()) {
            header
                    << "  q"
                    << instruction->targetQubit.value();
        }

        drawList->AddText(
            ImVec2{minimum.x + 11.0F, minimum.y + 9.0F},
            IM_COL32(132, 192, 230, 255),
            header.str().c_str()
        );

        std::string description =
                "Identity preserves the register.";

        if (instruction != nullptr) {
            const std::string &name =
                    instruction->name;

            if (name == "H") {
                description = "Mixes paired basis states.";
            } else if (
                name == "X" ||
                name == "Y" ||
                name == "Rx" ||
                name == "Ry"
            ) {
                description = "Transfers amplitude between pairs.";
            } else if (
                name == "Z" ||
                name == "S" ||
                name == "Sdg" ||
                name == "T" ||
                name == "Tdg" ||
                name == "Rz"
            ) {
                description = "Changes phase, not probabilities.";
            } else if (
                name == "SWAP" ||
                name == "iSWAP"
            ) {
                description = "Exchanges the selected qubits.";
            } else if (
                instruction->controlQubit.has_value()
            ) {
                description = "Correlates control and target.";
            } else {
                description = "Transforms the complete register.";
            }
        }

        drawList->AddText(
            ImVec2{minimum.x + 11.0F, minimum.y + 29.0F},
            IM_COL32(104, 125, 158, 255),
            description.c_str()
        );

        const float lineStartX =
                minimum.x + 42.0F;

        const float lineEndX =
                maximum.x - 11.0F;

        const float gateCenterX =
                minimum.x + panelWidth * 0.58F;

        const std::size_t targetQubit =
                instruction == nullptr
                    ? 0U
                    : instruction->secondaryTargetQubit.value_or(
                        instruction->targetQubit.value_or(0U)
                    );

        if (
            instruction != nullptr &&
            instruction->controlQubit.has_value() &&
            displayedQubits.size() > 1U
        ) {
            const auto controlPosition =
                    std::find(
                        displayedQubits.begin(),
                        displayedQubits.end(),
                        instruction->controlQubit.value()
                    );

            const auto targetPosition =
                    std::find(
                        displayedQubits.begin(),
                        displayedQubits.end(),
                        targetQubit
                    );

            if (
                controlPosition != displayedQubits.end() &&
                targetPosition != displayedQubits.end()
            ) {
                const float controlY =
                        minimum.y + 59.0F +
                        static_cast<float>(
                            std::distance(
                                displayedQubits.begin(),
                                controlPosition
                            )
                        ) * rowHeight;

                const float targetY =
                        minimum.y + 59.0F +
                        static_cast<float>(
                            std::distance(
                                displayedQubits.begin(),
                                targetPosition
                            )
                        ) * rowHeight;

                drawList->AddLine(
                    ImVec2{gateCenterX, controlY},
                    ImVec2{gateCenterX, targetY},
                    IM_COL32(245, 175, 59, 255),
                    1.5F
                );
            }
        }

        for (std::size_t row = 0U; row < displayedQubits.size(); ++row) {
            const std::size_t qubit =
                    displayedQubits[row];

            const float y =
                    minimum.y + 59.0F +
                    static_cast<float>(row) * rowHeight;

            const std::string qubitLabel =
                    "q" + std::to_string(qubit);

            drawList->AddText(
                ImVec2{minimum.x + 11.0F, y - 7.0F},
                IM_COL32(122, 143, 174, 255),
                qubitLabel.c_str()
            );

            drawList->AddLine(
                ImVec2{lineStartX, y},
                ImVec2{lineEndX, y},
                IM_COL32(74, 101, 132, 215),
                1.0F
            );

            const bool isAffected =
                    instruction == nullptr ||
                    instruction->targetQubit == qubit ||
                    instruction->controlQubit == qubit ||
                    instruction->secondaryTargetQubit == qubit;

            if (!isAffected) {
                continue;
            }

            const bool isControl =
                    instruction != nullptr &&
                    instruction->controlQubit == qubit &&
                    qubit != targetQubit;

            if (isControl) {
                drawList->AddCircleFilled(
                    ImVec2{gateCenterX, y},
                    4.0F,
                    IM_COL32(245, 175, 59, 255)
                );
                continue;
            }

            const ImVec2 gateMinimum{
                gateCenterX - 15.0F,
                y - 8.0F
            };

            const ImVec2 gateMaximum{
                gateCenterX + 15.0F,
                y + 8.0F
            };

            drawList->AddRectFilled(
                gateMinimum,
                gateMaximum,
                IM_COL32(7, 30, 46, 255),
                4.0F
            );

            drawList->AddRect(
                gateMinimum,
                gateMaximum,
                instruction == nullptr
                    ? IM_COL32(132, 192, 230, 255)
                    : IM_COL32(44, 197, 240, 255),
                4.0F,
                0,
                1.5F
            );

            const std::string gateLabel =
                    instruction == nullptr
                        ? "I"
                        : instruction->name;

            const ImVec2 textSize =
                    ImGui::CalcTextSize(gateLabel.c_str());

            drawList->AddText(
                ImVec2{
                    gateCenterX - textSize.x * 0.5F,
                    y - textSize.y * 0.5F
                },
                IM_COL32(218, 236, 246, 255),
                gateLabel.c_str()
            );
        }
    }
}

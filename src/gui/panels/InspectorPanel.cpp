#include "quantum_sim/gui/panels/InspectorPanel.hpp"
#include "quantum_sim/gui/rendering/QaveColorMap.hpp"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace {
    struct IndexedStateInfo {
        std::size_t index{};
        quantum_sim::quantum::StateInfo state;
    };

    [[nodiscard]] std::string toLower(std::string value) {
        for (char &character: value) {
            character =
                    static_cast<char>(
                        std::tolower(
                            static_cast<unsigned char>(character)
                        )
                    );
        }

        return value;
    }

    [[nodiscard]] bool stateMatchesFilter(
        const IndexedStateInfo &entry,
        const std::string &filter
    ) {
        if (filter.empty()) {
            return true;
        }

        const std::string label =
                toLower(entry.state.label);

        const std::string index =
                std::to_string(entry.index);

        return label.find(filter) != std::string::npos ||
               index.find(filter) != std::string::npos;
    }

    [[nodiscard]] quantum_sim::quantum::BlochVector reducedBlochVector(
        const quantum_sim::quantum::QuantumRegister &state,
        const std::size_t qubitIndex
    ) {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;

        const std::size_t qubitCount =
                state.qubitCount();

        const std::size_t bitPosition =
                qubitCount - 1 - qubitIndex;

        const std::size_t mask =
                std::size_t{1} << bitPosition;

        // Reduced Bloch vector comes from tracing out every qubit except the selected one.
        for (std::size_t stateIndex = 0; stateIndex < state.stateCount(); ++stateIndex) {
            const bool bitIsOne =
                    (stateIndex & mask) != 0;

            const double probability =
                    state.probability(stateIndex);

            z += bitIsOne
                     ? -probability
                     : probability;

            if (bitIsOne) {
                // Each zero-side basis state pairs with the same state where this qubit is one.
                continue;
            }

            const std::size_t pairedIndex =
                    stateIndex | mask;

            const auto &zeroAmplitude =
                    state.amplitude(stateIndex);

            const auto &oneAmplitude =
                    state.amplitude(pairedIndex);

            x += 2.0 * (
                zeroAmplitude.real() * oneAmplitude.real() +
                zeroAmplitude.imaginary() * oneAmplitude.imaginary()
            );

            y += 2.0 * (
                zeroAmplitude.real() * oneAmplitude.imaginary() -
                zeroAmplitude.imaginary() * oneAmplitude.real()
            );
        }

        return quantum_sim::quantum::BlochVector{x, y, z};
    }

    [[nodiscard]] ImU32 densityColor(
        const double normalizedMagnitude,
        const double phase
    ) {
        const quantum_sim::gui::qave::Color color =
                quantum_sim::gui::qave::phaseColor(
                    normalizedMagnitude,
                    phase
                );

        return IM_COL32(
            static_cast<int>(std::clamp(color.red, 0.0F, 1.0F) * 255.0F),
            static_cast<int>(std::clamp(color.green, 0.0F, 1.0F) * 255.0F),
            static_cast<int>(std::clamp(color.blue, 0.0F, 1.0F) * 255.0F),
            255
        );
    }
}

namespace quantum_sim::gui {
    bool InspectorPanel::draw(
        debug::DebuggerSession &session,
        const debug::DebuggerSnapshot &snapshot,
        const circuit::QuantumCircuit &circuit,
        std::optional<std::size_t> selectedInstructionIndex,
        const qave::DensityStack &densityStack,
        std::size_t &selectedDensityLayer,
        ImFont *headingFont
    ) {
        drawHeader(snapshot, selectedInstructionIndex, headingFont);

        const bool jumpedToInstruction =
        drawInstructionSummary(session, snapshot, circuit, selectedInstructionIndex);

        if (
            jumpedToInstruction &&
            selectedInstructionIndex.has_value()
        ) {
            selectedDensityLayer =
                    std::min(
                        selectedInstructionIndex.value() + 1U,
                        densityStack.layers.empty()
                            ? 0U
                            : densityStack.layers.size() - 1U
                    );
        }

        drawQuantumState(
            session,
            densityStack,
            selectedDensityLayer
        );

        drawDebuggerControls(session, snapshot);

        return jumpedToInstruction;
    }

    void InspectorPanel::showNavigationConfirmation(
        std::string message
    ) {
        navigationConfirmationMessage_ =
                std::move(message);

        // Confirmation messages self-expire so the panel does not need extra state cleanup.
        navigationConfirmationUntil_ =
                ImGui::GetTime() + 1.5;
    }

    void InspectorPanel::moveToPreviousInstruction(
        debug::DebuggerSession &session
    ) {
        session.movePrevious();

        showNavigationConfirmation(
            "Moved to the previous instruction."
        );
    }

    void InspectorPanel::moveToNextInstruction(
        debug::DebuggerSession &session
    ) {
        session.moveNext();

        showNavigationConfirmation(
            "Moved to the next instruction."
        );
    }

    void InspectorPanel::restartDebugger(
        debug::DebuggerSession &session
    ) {
        session.restart();

        showNavigationConfirmation(
            "Debugger restarted."
        );
    }

    void InspectorPanel::jumpToInstruction(debug::DebuggerSession &session, std::size_t instructionIndex) {
        session.moveToStep(instructionIndex);

        showNavigationConfirmation(
            "Jumped to the selected instruction."
        );
    }

    void InspectorPanel::drawNavigationConfirmation() const {
        if (
            ImGui::GetTime() >= navigationConfirmationUntil_ ||
            navigationConfirmationMessage_.empty()
        ) {
            return;
        }

        ImGui::TextColored(
            ImVec4{0.35F, 0.85F, 0.55F, 1.0F},
            "%s",
            navigationConfirmationMessage_.c_str()
        );
    }

    void InspectorPanel::drawDebuggerControls(
        debug::DebuggerSession &session,
        const debug::DebuggerSnapshot &snapshot
    ) {
        ImGui::Spacing();
        ImGui::SeparatorText("Debugger Controls");

        const float availableButtonWidth =
                ImGui::GetContentRegionAvail().x;

        const float buttonSpacing =
                ImGui::GetStyle().ItemSpacing.x;

        const float debuggerButtonWidth =
                (availableButtonWidth - buttonSpacing * 2.0F)
                / 3.0F;

        const ImGuiIO &io =
                ImGui::GetIO();

        if (
            !io.WantTextInput &&
            ImGui::IsWindowFocused(
                ImGuiFocusedFlags_RootAndChildWindows
            )
        ) {
            // Keyboard navigation only runs when the inspector has focus.
            if (
                ImGui::IsKeyPressed(ImGuiKey_LeftArrow) &&
                snapshot.canMovePrevious
            ) {
                moveToPreviousInstruction(session);
            }

            if (
                ImGui::IsKeyPressed(ImGuiKey_RightArrow) &&
                snapshot.canMoveNext
            ) {
                moveToNextInstruction(session);
            }

            if (ImGui::IsKeyPressed(ImGuiKey_R)) {
                restartDebugger(session);
            }
        }

        if (!snapshot.canMovePrevious) {
            ImGui::BeginDisabled();
        }

        if (
            ImGui::Button(
                "Previous  [←]",
                ImVec2{debuggerButtonWidth, 0.0F}
            )
        ) {
            moveToPreviousInstruction(session);
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Go to the previous instruction. Shortcut: Left Arrow"
            );
        }

        if (!snapshot.canMovePrevious) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (!snapshot.canMoveNext) {
            ImGui::BeginDisabled();
        }

        if (
            ImGui::Button(
                "Next  [→]",
                ImVec2{debuggerButtonWidth, 0.0F}
            )
        ) {
            moveToNextInstruction(session);
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Go to the next instruction. Shortcut: Right Arrow"
            );
        }

        if (!snapshot.canMoveNext) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (
            ImGui::Button(
                "Restart  [R]",
                ImVec2{debuggerButtonWidth, 0.0F}
            )
        ) {
            restartDebugger(session);
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Return to the first instruction. Shortcut: R"
            );
        }
    }

    bool InspectorPanel::drawInstructionSummary(
        debug::DebuggerSession &session,
        const debug::DebuggerSnapshot &snapshot,
        const circuit::QuantumCircuit &circuit,
        std::optional<std::size_t> selectedInstructionIndex
    ) {
        bool jumpedToInstruction = false;

        const std::size_t inspectedInstructionIndex =
                selectedInstructionIndex.value_or(
                    snapshot.currentStepIndex
                );

        // Selection mode lets the renderer and inspector inspect different steps.
        ImGui::TextDisabled(
            selectedInstructionIndex.has_value()
                ? "Selection"
                : "Execution"
        );

        ImGui::SameLine();

        ImGui::Text(
            "Instruction %zu",
            inspectedInstructionIndex
        );

        const auto instructions =
                circuit.instructionInfo();

        const circuit::CircuitInstructionInfo *inspectedInstruction =
                nullptr;

        if (            instructions.empty() ||
            !snapshot.instruction.has_value()        ) {
            ImGui::SeparatorText("Instruction");

            ImGui::TextDisabled(
                "The circuit contains no instructions."
            );

            drawNavigationConfirmation();

            return false;
        }

        if (inspectedInstructionIndex < instructions.size()) {
            // Prefer circuit metadata for selected gates.
            inspectedInstruction =
                    &instructions[inspectedInstructionIndex];
        } else {
            // Fall back to the debugger snapshot when execution is the source.
            inspectedInstruction =
                    &snapshot.instruction->get();
        }

        ImGui::SeparatorText("Instruction");

        ImGui::Text(
            "%zu — %s",
            inspectedInstructionIndex,
            inspectedInstruction->name.c_str()
        );

        if (inspectedInstruction->angleRadians.has_value()) {
            ImGui::TextColored(
                ImVec4{0.35F, 0.80F, 1.0F, 1.0F},
                "angle %.6f rad",
                inspectedInstruction->angleRadians.value()
            );
        }

        if (
            selectedInstructionIndex.has_value() &&
            selectedInstructionIndex.value() != snapshot.currentStepIndex
        ) {
            const std::size_t selectedIndex =
                    selectedInstructionIndex.value();

            if (
                ImGui::Button(
                    "Jump to instruction  [J]",
                    ImVec2{-1.0F, 0.0F}
                )
            ) {
                jumpToInstruction(
                    session,
                    selectedIndex
                );

                jumpedToInstruction = true;
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Move debugger execution to instruction %zu",
                    selectedIndex
                );
            }

            const ImGuiIO &io =
                    ImGui::GetIO();

            if (
                !io.WantTextInput &&
                ImGui::IsKeyPressed(ImGuiKey_J)
            ) {
                jumpToInstruction(
                    session,
                    selectedIndex
                );

                jumpedToInstruction = true;
            }
        }

        drawNavigationConfirmation();

        return jumpedToInstruction;
    }

    void InspectorPanel::drawQuantumState(
        debug::DebuggerSession &session,
        const qave::DensityStack &densityStack,
        std::size_t &selectedDensityLayer
    ) {
        ImGui::Spacing();
        ImGui::SeparatorText("Quantum State");

        drawLayerStack(
            session,
            densityStack,
            selectedDensityLayer
        );

        const quantum::QuantumRegister *state =
                &session.initialState();

        if (
            selectedDensityLayer > 0U &&
            selectedDensityLayer - 1U < session.stepCount()
        ) {
            state =
                    &session.stepAt(selectedDensityLayer - 1U).state;
        }

        drawProbabilities(*state);
        drawAmplitudes(*state);
        drawBlochInformation(*state);
    }

    void InspectorPanel::drawProbabilities(const quantum::QuantumRegister &state) {
        ImGui::TextDisabled("Qubit probabilities");

        if (
            ImGui::BeginTable(
                "QubitProbabilityTable",
                3,
                ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_RowBg
            )
        ) {
            ImGui::TableSetupColumn("Qubit", ImGuiTableColumnFlags_WidthFixed, 52.0F);
            ImGui::TableSetupColumn("P(1)");
            ImGui::TableSetupColumn("P(0)", ImGuiTableColumnFlags_WidthFixed, 74.0F);
            ImGui::TableHeadersRow();

            for (std::size_t qubit = 0; qubit < state.qubitCount(); ++qubit) {
                // Marginal probabilities stay compact even for large registers.
                const float oneProbability =
                        static_cast<float>(
                            state.probabilityOfQubitOne(qubit)
                        );

                const float zeroProbability =
                        1.0F - oneProbability;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("q%zu", qubit);

                ImGui::TableSetColumnIndex(1);

                char overlay[32]{};
                std::snprintf(
                    overlay,
                    sizeof(overlay),
                    "%.1f%%",
                    oneProbability * 100.0F
                );

                ImGui::ProgressBar(
                    oneProbability,
                    ImVec2{-1.0F, 0.0F},
                    overlay
                );

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.1f%%", zeroProbability * 100.0F);
            }

            ImGui::EndTable();
        }
    }

    void InspectorPanel::drawLayerStack(
        debug::DebuggerSession &session,
        const qave::DensityStack &densityStack,
        std::size_t &selectedDensityLayer
    ) {
        ImGui::Spacing();
        ImGui::TextDisabled("Layer stack");

        if (densityStack.layers.empty()) {
            ImGui::TextDisabled("No density history is available.");
            return;
        }

        selectedDensityLayer =
                std::min(
                    selectedDensityLayer,
                    densityStack.layers.size() - 1U
                );

        int layer =
                static_cast<int>(selectedDensityLayer);

        if (layer == 0) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("<##PreviousDensityLayer")) {
            --layer;
        }

        if (selectedDensityLayer == 0U) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(
            std::max(
                120.0F,
                ImGui::GetContentRegionAvail().x - 36.0F
            )
        );

        const bool sliderChanged =
                ImGui::SliderInt(
                    "##DensityLayer",
                    &layer,
                    0,
                    static_cast<int>(densityStack.layers.size() - 1U),
                    "layer %d"
                );

        ImGui::SameLine();

        if (selectedDensityLayer + 1U >= densityStack.layers.size()) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button(">##NextDensityLayer")) {
            ++layer;
        }

        if (selectedDensityLayer + 1U >= densityStack.layers.size()) {
            ImGui::EndDisabled();
        }

        if (
            sliderChanged ||
            layer != static_cast<int>(selectedDensityLayer)
        ) {
            selectedDensityLayer =
                    static_cast<std::size_t>(
                        std::clamp(
                            layer,
                            0,
                            static_cast<int>(densityStack.layers.size() - 1U)
                        )
                    );

            if (selectedDensityLayer > 0U && session.hasSteps()) {
                session.moveToStep(selectedDensityLayer - 1U);
            }
        }

        const qave::DensityLayer &densityLayer =
                densityStack.layers[selectedDensityLayer];

        const float availableWidth =
                std::max(220.0F, ImGui::GetContentRegionAvail().x);

        const std::size_t gridDimension =
                densityLayer.dimension;

        double layerMaximumMagnitude{};

        for (const qave::DensityCell &cell : densityLayer.cells) {
            layerMaximumMagnitude =
                    std::max(
                        layerMaximumMagnitude,
                        cell.magnitude
                    );
        }

        layerMaximumMagnitude =
                std::max(layerMaximumMagnitude, 1e-12);

        const float panelWidth =
                std::clamp(
                    availableWidth,
                    260.0F,
                    560.0F
                );

        constexpr float padding = 10.0F;
        constexpr float headerHeight = 34.0F;
        constexpr float legendHeight = 30.0F;

        const float gridSide =
                std::max(
                    240.0F,
                    panelWidth - padding * 2.0F
                );

        const float cellSize =
                gridSide / static_cast<float>(gridDimension);

        const float cellInset =
                gridDimension <= 32U
                    ? 1.0F
                    : 0.45F;

        const ImVec2 canvasSize{
            panelWidth,
            padding + headerHeight + gridSide + padding + legendHeight
        };

        const ImVec2 origin =
                ImGui::GetCursorScreenPos();

        ImGui::InvisibleButton(
            "DensityLayerCanvas",
            canvasSize
        );

        ImDrawList *drawList =
                ImGui::GetWindowDrawList();

        drawList->AddRectFilled(
            origin,
            ImVec2{origin.x + canvasSize.x, origin.y + canvasSize.y},
            IM_COL32(7, 11, 20, 232),
            7.0F
        );

        drawList->AddRect(
            origin,
            ImVec2{origin.x + canvasSize.x, origin.y + canvasSize.y},
            IM_COL32(55, 99, 142, 210),
            7.0F,
            0,
            1.0F
        );

        const ImVec2 headerPosition{
            origin.x + padding,
            origin.y + 7.0F
        };

        const std::string header =
                "rho - 2D - LAYER " +
                std::to_string(selectedDensityLayer) +
                "/" +
                std::to_string(densityStack.layers.size() - 1U) +
                " - " +
                std::to_string(gridDimension) +
                "x" +
                std::to_string(gridDimension);

        drawList->AddText(
            headerPosition,
            IM_COL32(139, 160, 193, 255),
            header.c_str()
        );

        if (densityLayer.bucketed) {
            const std::string bucketLabel =
                    "bucketed from " +
                    std::to_string(densityLayer.sourceStateCount) +
                    " states";

            drawList->AddText(
                ImVec2{
                    headerPosition.x,
                    headerPosition.y + 17.0F
                },
                IM_COL32(95, 117, 151, 255),
                bucketLabel.c_str()
            );
        }

        const ImVec2 gridOrigin{
            origin.x + padding,
            origin.y + padding + headerHeight
        };

        const ImVec2 gridMaximum{
            gridOrigin.x + gridSide,
            gridOrigin.y + gridSide
        };

        drawList->AddRectFilled(
            gridOrigin,
            gridMaximum,
            IM_COL32(13, 8, 34, 245),
            3.0F
        );

        for (std::size_t row = 0; row < gridDimension; ++row) {
            for (std::size_t column = 0; column < gridDimension; ++column) {
                const qave::DensityCell &cell =
                        densityLayer.cellAt(row, column);

                const double normalizedMagnitude =
                        cell.magnitude /
                        layerMaximumMagnitude;

                const ImVec2 minimum{
                    gridOrigin.x + static_cast<float>(column) * cellSize + cellInset * 0.5F,
                    gridOrigin.y + static_cast<float>(row) * cellSize + cellInset * 0.5F
                };

                const ImVec2 maximum{
                    gridOrigin.x + static_cast<float>(column + 1U) * cellSize - cellInset * 0.5F,
                    gridOrigin.y + static_cast<float>(row + 1U) * cellSize - cellInset * 0.5F
                };

                drawList->AddRectFilled(
                    minimum,
                    maximum,
                    normalizedMagnitude <= 0.002
                        ? IM_COL32(22, 19, 47, 255)
                        : densityColor(
                            normalizedMagnitude,
                            cell.phaseRadians
                        ),
                    gridDimension <= 32U ? 1.5F : 0.7F
                );
            }
        }

        const std::size_t gridLineStride =
                gridDimension <= 32U
                    ? 1U
                    : 4U;

        for (std::size_t line = 0; line <= gridDimension; line += gridLineStride) {
            const float offset =
                    static_cast<float>(line) * cellSize;

            drawList->AddLine(
                ImVec2{gridOrigin.x + offset, gridOrigin.y},
                ImVec2{gridOrigin.x + offset, gridMaximum.y},
                IM_COL32(89, 112, 154, 34),
                1.0F
            );

            drawList->AddLine(
                ImVec2{gridOrigin.x, gridOrigin.y + offset},
                ImVec2{gridMaximum.x, gridOrigin.y + offset},
                IM_COL32(89, 112, 154, 34),
                1.0F
            );
        }

        drawList->AddLine(
            gridOrigin,
            gridMaximum,
            IM_COL32(219, 147, 48, 96),
            1.25F
        );

        drawList->AddRect(
            gridOrigin,
            gridMaximum,
            IM_COL32(93, 139, 192, 155),
            3.0F,
            0,
            1.0F
        );

        constexpr int legendSteps = 42;
        const float legendWidth = 88.0F;
        const float legendHeightPixels = 8.0F;
        const ImVec2 legendMinimum{
            origin.x + canvasSize.x - padding - legendWidth,
            gridMaximum.y + padding + 5.0F
        };

        for (int step = 0; step < legendSteps; ++step) {
            const float t =
                    static_cast<float>(step) /
                    static_cast<float>(legendSteps - 1);

            const ImVec2 minimum{
                legendMinimum.x + legendWidth * t,
                legendMinimum.y
            };

            const ImVec2 maximum{
                legendMinimum.x + legendWidth * (static_cast<float>(step + 1) / static_cast<float>(legendSteps)),
                legendMinimum.y + legendHeightPixels
            };

            drawList->AddRectFilled(
                minimum,
                maximum,
                densityColor(
                    1.0,
                    -3.141592653589793 +
                    6.283185307179586 * static_cast<double>(t)
                ),
                0.0F
            );
        }

        const char *legendLabel =
                "phase -pi..+pi";

        const ImVec2 legendLabelSize =
                ImGui::CalcTextSize(legendLabel);

        drawList->AddText(
            ImVec2{
                legendMinimum.x - legendLabelSize.x - 7.0F,
                legendMinimum.y - 3.0F
            },
            IM_COL32(95, 117, 151, 255),
            legendLabel
        );

        if (ImGui::IsItemHovered()) {
            const ImVec2 mouse =
                    ImGui::GetMousePos();

            if (
                mouse.x >= gridOrigin.x &&
                mouse.x <= gridMaximum.x &&
                mouse.y >= gridOrigin.y &&
                mouse.y <= gridMaximum.y
            ) {
                const std::size_t column =
                        std::min<std::size_t>(
                            gridDimension - 1U,
                            static_cast<std::size_t>(
                                (mouse.x - gridOrigin.x) / cellSize
                            )
                        );

                const std::size_t row =
                        std::min<std::size_t>(
                            gridDimension - 1U,
                            static_cast<std::size_t>(
                                (mouse.y - gridOrigin.y) / cellSize
                            )
                        );

                const qave::DensityCell &cell =
                        densityLayer.cellAt(row, column);

                const qave::DensityBin &rowBin =
                        densityLayer.bins[row];

                const qave::DensityBin &columnBin =
                        densityLayer.bins[column];

                ImGui::BeginTooltip();
                ImGui::Text(
                    "rho[%zu][%zu]",
                    row,
                    column
                );
                ImGui::Text("row %s", rowBin.label.c_str());
                ImGui::Text("col %s", columnBin.label.c_str());

                if (densityLayer.bucketed) {
                    ImGui::Text(
                        "row states %zu-%zu",
                        rowBin.firstState,
                        rowBin.lastState - 1U
                    );
                    ImGui::Text(
                        "col states %zu-%zu",
                        columnBin.firstState,
                        columnBin.lastState - 1U
                    );
                }

                ImGui::Text("|rho|       %.8f", cell.magnitude);
                ImGui::Text("intensity   %.8f", cell.intensity);
                ImGui::Text("phase       %.6f rad", cell.phaseRadians);
                ImGui::Text("Re(rho)     %.8f", cell.real);
                ImGui::Text("Im(rho)     %.8f", cell.imaginary);
                ImGui::EndTooltip();
            }
        }
    }

    void InspectorPanel::drawAmplitudes(const quantum::QuantumRegister &state) {
        ImGui::TextDisabled("Amplitudes");

        ImGui::SetNextItemWidth(-1.0F);
        ImGui::InputText(
            "Filter",
            amplitudeFilter_.data(),
            amplitudeFilter_.size()
        );

        ImGui::Checkbox("Live only", &showOnlyLiveAmplitudes_);
        ImGui::SameLine();
        ImGui::Checkbox("Sort by p", &sortAmplitudesByProbability_);

        ImGui::SetNextItemWidth(-1.0F);
        ImGui::SliderInt(
            "Shown",
            &maximumVisibleAmplitudes_,
            16,
            256
        );

        const std::string filter =
                toLower(amplitudeFilter_.data());

        std::vector<IndexedStateInfo> entries;
        entries.reserve(state.stateCount());

        constexpr double liveThreshold = 1e-10;

        for (std::size_t stateIndex = 0; stateIndex < state.stateCount(); ++stateIndex) {
            IndexedStateInfo entry{
                stateIndex,
                state.stateInfo(stateIndex)
            };

            // The default table omits zero amplitudes so large registers stay readable.
            if (
                showOnlyLiveAmplitudes_ &&
                entry.state.probability <= liveThreshold
            ) {
                continue;
            }

            if (!stateMatchesFilter(entry, filter)) {
                continue;
            }

            entries.push_back(std::move(entry));
        }

        if (sortAmplitudesByProbability_) {
            // Put the physically important states first.
            std::sort(
                entries.begin(),
                entries.end(),
                [](const IndexedStateInfo &left, const IndexedStateInfo &right) {
                    return left.state.probability > right.state.probability;
                }
            );
        }

        const std::size_t visibleCount =
                std::min(
                    entries.size(),
                    static_cast<std::size_t>(
                        std::max(1, maximumVisibleAmplitudes_)
                    )
                );

        ImGui::TextDisabled(
            "Showing %zu of %zu states",
            visibleCount,
            entries.size()
        );

        const float tableHeight =
                std::min(
                    280.0F,
                    60.0F + static_cast<float>(visibleCount) * 24.0F
                );

        ImGui::BeginChild(
            "AmplitudeTableScroller",
            ImVec2{0.0F, tableHeight},
            true
        );

        if (
            ImGui::BeginTable(
                "AmplitudeTable",
                4,
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_ScrollY
            )
        ) {
            ImGui::TableSetupColumn("State");
            ImGui::TableSetupColumn("p");
            ImGui::TableSetupColumn("Real");
            ImGui::TableSetupColumn("Imaginary");
            ImGui::TableHeadersRow();

            for (std::size_t index = 0; index < visibleCount; ++index) {
                const quantum::StateInfo &stateInfo =
                        entries[index].state;

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(stateInfo.label.c_str());

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.5f", stateInfo.probability);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text(
                    "%.4f",
                    stateInfo.amplitude.real()
                );

                ImGui::TableSetColumnIndex(3);
                ImGui::Text(
                    "%.4f",
                    stateInfo.amplitude.imaginary()
                );
            }

            ImGui::EndTable();
        }

        ImGui::EndChild();
    }

    void InspectorPanel::drawBlochInformation(const quantum::QuantumRegister &state) {
        if (state.qubitCount() == 0) {
            return;
        }

        inspectedBlochQubit_ =
                std::clamp(
                    inspectedBlochQubit_,
                    0,
                    static_cast<int>(state.qubitCount() - 1)
                );

        ImGui::Spacing();
        ImGui::SeparatorText("Bloch Sphere");

        if (state.qubitCount() > 1) {
            const std::string preview =
                    "q" + std::to_string(inspectedBlochQubit_);

            if (ImGui::BeginCombo("Qubit", preview.c_str())) {
                for (std::size_t qubit = 0; qubit < state.qubitCount(); ++qubit) {
                    const bool selected =
                            inspectedBlochQubit_ ==
                            static_cast<int>(qubit);

                    const std::string label =
                            "q" + std::to_string(qubit);

                    if (ImGui::Selectable(label.c_str(), selected)) {
                        inspectedBlochQubit_ =
                                static_cast<int>(qubit);
                    }

                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
        }

        const quantum::BlochVector bloch =
                state.qubitCount() == 1
                    ? state.blockVector()
                    : reducedBlochVector(
                        state,
                        static_cast<std::size_t>(inspectedBlochQubit_)
                    );

        // Vector length below 1.0 indicates a mixed/reduced single-qubit view.
        const double purity =
                std::sqrt(
                    bloch.x * bloch.x +
                    bloch.y * bloch.y +
                    bloch.z * bloch.z
                );

        const double safePurity =
                std::max(purity, 1e-10);

        const double theta =
                std::acos(
                    std::clamp(
                        bloch.z / safePurity,
                        -1.0,
                        1.0
                    )
                );

        const double phi =
                std::atan2(bloch.y, bloch.x);

        ImGui::Text(
            "vector (%.4f, %.4f, %.4f)",
            bloch.x,
            bloch.y,
            bloch.z
        );

        ImGui::Text(
            "theta %.4f   phi %.4f   purity %.3f",
            theta,
            phi,
            std::clamp(purity, 0.0, 1.0)
        );

        ImGui::Spacing();
        blochSphereRenderer_.draw(bloch);

        if (state.qubitCount() > 1) {
            ImGui::TextDisabled(
                "Reduced view of q%d",
                inspectedBlochQubit_
            );
        }
    }

    void InspectorPanel::drawHeader(const debug::DebuggerSnapshot &snapshot,
                                    std::optional<std::size_t> selectedInstructionIndex, ImFont *headingFont) {
        if (headingFont != nullptr) {
            ImGui::PushFont(headingFont);
        }

        ImGui::TextUnformatted("Inspector");

        if (headingFont != nullptr) {
            ImGui::PopFont();
        }

        if (snapshot.stepCount == 0) {
            ImGui::TextUnformatted("Step 0 / 0");
        } else {
            ImGui::Text(
                "Step %d / %d",
                static_cast<int>(snapshot.currentStepIndex + 1),
                static_cast<int>(snapshot.stepCount)
            );
        }

        if (selectedInstructionIndex.has_value()) {
            ImGui::TextDisabled(
                "Showing state after instruction %zu.",
                selectedInstructionIndex.value()
            );
        } else {
            ImGui::TextDisabled(
                "Showing state after the current debugger step."
            );
        }
    }

}

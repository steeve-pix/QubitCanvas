#include "quantum_sim/gui/panels/InspectorPanel.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <numbers>
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

        for (std::size_t stateIndex = 0; stateIndex < state.stateCount(); ++stateIndex) {
            const bool bitIsOne =
                    (stateIndex & mask) != 0;

            const double probability =
                    state.probability(stateIndex);

            z += bitIsOne
                     ? -probability
                     : probability;

            if (bitIsOne) {
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

    [[nodiscard]] ImU32 probabilityColor(
        const double probability,
        const double phase
    ) {
        const float intensity =
                std::clamp(
                    static_cast<float>(std::sqrt(probability)),
                    0.0F,
                    1.0F
                );

        const float phaseT =
                static_cast<float>(
                    (phase + std::numbers::pi) /
                    (2.0 * std::numbers::pi)
                );

        const int red =
                static_cast<int>(88.0F + intensity * 180.0F);

        const int green =
                static_cast<int>(52.0F + std::sin(phaseT * 6.28318F) * 40.0F + intensity * 130.0F);

        const int blue =
                static_cast<int>(135.0F + std::cos(phaseT * 6.28318F) * 70.0F + intensity * 75.0F);

        return IM_COL32(
            std::clamp(red, 0, 255),
            std::clamp(green, 0, 255),
            std::clamp(blue, 0, 255),
            static_cast<int>(55.0F + intensity * 200.0F)
        );
    }
}

namespace quantum_sim::gui {
    bool InspectorPanel::draw(
        debug::DebuggerSession &session,
        const debug::DebuggerSnapshot &snapshot,
        const circuit::QuantumCircuit &circuit,
        std::optional<std::size_t> selectedInstructionIndex,
        ImFont *headingFont
    ) {
        drawHeader(snapshot, selectedInstructionIndex, headingFont);

        const bool jumpedToInstruction =
                drawInstructionSummary(session, snapshot, circuit, selectedInstructionIndex);

        const quantum::QuantumRegister &currentState =
                resolveInspectedState(session, snapshot, selectedInstructionIndex);

        drawQuantumState(currentState);

        drawDebuggerControls(session, snapshot);

        return jumpedToInstruction;
    }

    void InspectorPanel::showNavigationConfirmation(
        std::string message
    ) {
        navigationConfirmationMessage_ =
                std::move(message);

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
            inspectedInstruction =
                    &instructions[inspectedInstructionIndex];
        } else {
            inspectedInstruction =
                    &snapshot.instruction->get();
        }

        ImGui::SeparatorText("Instruction");

        ImGui::Text(
            "%zu — %s",
            inspectedInstructionIndex,
            inspectedInstruction->name.c_str()
        );

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

        const std::string explanation =
                debug::gateExplanation(
                    inspectedInstruction->name
                );

        ImGui::Spacing();
        ImGui::TextDisabled("Explanation");

        ImGui::TextWrapped(
            "%s",
            explanation.c_str()
        );

        return jumpedToInstruction;
    }

    void InspectorPanel::drawQuantumState(const quantum::QuantumRegister &state) {
        ImGui::Spacing();
        ImGui::SeparatorText("Quantum State");

        drawProbabilities(state);
        drawStateHeatmap(state);
        drawAmplitudes(state);
        drawBlochInformation(state);
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

    void InspectorPanel::drawStateHeatmap(const quantum::QuantumRegister &state) {
        ImGui::Spacing();
        ImGui::TextDisabled("Amplitude field");

        const float availableWidth =
                std::max(120.0F, ImGui::GetContentRegionAvail().x);

        const std::size_t stateCount =
                state.stateCount();

        const int columns =
                std::max(
                    1,
                    static_cast<int>(
                        std::ceil(
                            std::sqrt(
                                static_cast<double>(stateCount)
                            )
                        )
                    )
                );

        const float cellSize =
                std::clamp(
                    availableWidth / static_cast<float>(columns),
                    4.0F,
                    10.0F
                );

        const int rows =
                static_cast<int>(
                    (stateCount + static_cast<std::size_t>(columns) - 1) /
                    static_cast<std::size_t>(columns)
                );

        const ImVec2 canvasSize{
            availableWidth,
            static_cast<float>(rows) * cellSize
        };

        const ImVec2 origin =
                ImGui::GetCursorScreenPos();

        ImGui::InvisibleButton(
            "StateHeatmapCanvas",
            canvasSize
        );

        ImDrawList *drawList =
                ImGui::GetWindowDrawList();

        drawList->AddRectFilled(
            origin,
            ImVec2{origin.x + canvasSize.x, origin.y + canvasSize.y},
            IM_COL32(8, 13, 22, 190),
            4.0F
        );

        for (std::size_t stateIndex = 0; stateIndex < stateCount; ++stateIndex) {
            const int column =
                    static_cast<int>(stateIndex % static_cast<std::size_t>(columns));

            const int row =
                    static_cast<int>(stateIndex / static_cast<std::size_t>(columns));

            const auto &amplitude =
                    state.amplitude(stateIndex);

            const double probability =
                    state.probability(stateIndex);

            const double phase =
                    std::atan2(
                        amplitude.imaginary(),
                        amplitude.real()
                    );

            const ImVec2 minimum{
                origin.x + static_cast<float>(column) * cellSize,
                origin.y + static_cast<float>(row) * cellSize
            };

            const ImVec2 maximum{
                minimum.x + cellSize - 1.0F,
                minimum.y + cellSize - 1.0F
            };

            drawList->AddRectFilled(
                minimum,
                maximum,
                probabilityColor(probability, phase),
                1.0F
            );
        }

        if (ImGui::IsItemHovered()) {
            const ImVec2 mouse =
                    ImGui::GetMousePos();

            const int column =
                    static_cast<int>(
                        (mouse.x - origin.x) / cellSize
                    );

            const int row =
                    static_cast<int>(
                        (mouse.y - origin.y) / cellSize
                    );

            if (
                column >= 0 &&
                row >= 0
            ) {
                const std::size_t stateIndex =
                        static_cast<std::size_t>(row) *
                        static_cast<std::size_t>(columns) +
                        static_cast<std::size_t>(column);

                if (stateIndex < stateCount) {
                    const quantum::StateInfo info =
                            state.stateInfo(stateIndex);

                    ImGui::BeginTooltip();
                    ImGui::Text("%s", info.label.c_str());
                    ImGui::Text("index %zu", stateIndex);
                    ImGui::Text("p %.6f", info.probability);
                    ImGui::Text(
                        "amp %.4f%+.4fi",
                        info.amplitude.real(),
                        info.amplitude.imaginary()
                    );
                    ImGui::EndTooltip();
                }
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

    const quantum::QuantumRegister &
    InspectorPanel::resolveInspectedState(
        const debug::DebuggerSession &session,
        const debug::DebuggerSnapshot &snapshot,
        std::optional<std::size_t> selectedInstructionIndex
    ) const {
        if (
            selectedInstructionIndex.has_value() &&
            selectedInstructionIndex.value() < session.stepCount()
        ) {
            return session
                    .stepAt(selectedInstructionIndex.value())
                    .state;
        }

        return snapshot.afterState.get();
    }
}

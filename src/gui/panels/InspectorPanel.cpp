#include "quantum_sim/gui/panels/InspectorPanel.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"

#include "imgui.h"

#include <cmath>
#include <string>
#include <utility>

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
        drawAmplitudes(state);
        drawBlochInformation(state);
    }

    void InspectorPanel::drawProbabilities(const quantum::QuantumRegister &state) {
        ImGui::TextDisabled("Probabilities");

        for (const quantum::StateInfo &stateInfo: state.states()) {
            const float probability =
                    static_cast<float>(stateInfo.probability);

            ImGui::Text("%s", stateInfo.label.c_str());
            ImGui::SameLine();

            ImGui::ProgressBar(
                probability,
                ImVec2{-1.0F, 0.0F}
            );
        }
    }

    void InspectorPanel::drawAmplitudes(const quantum::QuantumRegister &state) {
        ImGui::TextDisabled("Amplitudes");

        if (ImGui::BeginTable("AmplitudeTable", 3)) {
            ImGui::TableSetupColumn("State");
            ImGui::TableSetupColumn("Real");
            ImGui::TableSetupColumn("Imaginary");
            ImGui::TableHeadersRow();

            for (const quantum::StateInfo &stateInfo: state.states()) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(stateInfo.label.c_str());

                ImGui::TableSetColumnIndex(1);
                ImGui::Text(
                    "%.4f",
                    stateInfo.amplitude.real()
                );

                ImGui::TableSetColumnIndex(2);
                ImGui::Text(
                    "%.4f",
                    stateInfo.amplitude.imaginary()
                );
            }

            ImGui::EndTable();
        }
    }

    void InspectorPanel::drawBlochInformation(const quantum::QuantumRegister &state) {
        if (state.qubitCount() == 1) {
            const quantum::BlochVector bloch =
                    state.blockVector();

            const quantum::BlochAngles angles =
                    state.blochAngles();

            ImGui::Spacing();
            ImGui::SeparatorText("Bloch Sphere");

            ImGui::TextDisabled("Vector and angles");

            ImGui::Text(
                "(%.4f, %.4f, %.4f)",
                bloch.x,
                bloch.y,
                bloch.z
            );

            ImGui::Spacing();

            ImGui::Text(
                "theta = %.4f rad",
                angles.theta
            );

            constexpr double epsilon = 1e-10;

            const bool isAtPole =
                    std::abs(bloch.x) < epsilon &&
                    std::abs(bloch.y) < epsilon;

            if (isAtPole) {
                ImGui::TextUnformatted(
                    "phi = undefined at the pole"
                );
            } else {
                ImGui::Text(
                    "phi = %.4f rad",
                    angles.phi
                );
            }

            ImGui::Spacing();
            ImGui::TextDisabled("Visualization");

            blochSphereRenderer_.draw(bloch);

            ImGui::Text(
                "Depth: y = %.4f",
                bloch.y
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

        ImGui::Text(
            "Step %d / %d",
            static_cast<int>(snapshot.currentStepIndex + 1),
            static_cast<int>(snapshot.stepCount)
        );

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

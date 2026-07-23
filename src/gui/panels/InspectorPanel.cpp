#include "quantum_sim/gui/panels/InspectorPanel.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"

#include "imgui.h"

#include <cmath>
#include <string>

namespace quantum_sim::gui {
    void InspectorPanel::draw(
        debug::DebuggerSession &session,
        const debug::DebuggerSnapshot &snapshot,
        const circuit::QuantumCircuit &circuit,
        std::optional<std::size_t> selectedInstructionIndex,
        ImFont *headingFont
    ) {
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

        if (inspectedInstructionIndex < instructions.size()) {
            inspectedInstruction =
                    &instructions[inspectedInstructionIndex];
        } else {
            inspectedInstruction =
                    &snapshot.instruction.get();
        }

        ImGui::SeparatorText("Instruction");

        ImGui::Text(
            "%zu — %s",
            inspectedInstructionIndex,
            inspectedInstruction->name.c_str()
        );

        const std::string explanation =
                debug::gateExplanation(inspectedInstruction->name);

        ImGui::Spacing();

        ImGui::TextDisabled("Explanation");

        ImGui::TextWrapped(
            "%s",
            explanation.c_str()
        );


        const quantum::QuantumRegister *inspectedState =
                &snapshot.afterState.get();

        if (
            selectedInstructionIndex.has_value() &&
            selectedInstructionIndex.value() < session.stepCount()
        ) {
            inspectedState =
                    &session
                    .stepAt(selectedInstructionIndex.value())
                    .state;
        }

        const quantum::QuantumRegister &currentState =
                *inspectedState;

        ImGui::Spacing();
        ImGui::SeparatorText("Quantum State");

        ImGui::TextDisabled("Probabilities");

        for (const quantum::StateInfo &stateInfo: currentState.states()) {
            const float probability =
                    static_cast<float>(stateInfo.probability);

            ImGui::Text("%s", stateInfo.label.c_str());
            ImGui::SameLine();

            ImGui::ProgressBar(probability, ImVec2{-1.0F, 0.0F});
        }


        ImGui::TextDisabled("Amplitudes");

        if (ImGui::BeginTable("AmplitudeTable", 3)) {
            ImGui::TableSetupColumn("State");
            ImGui::TableSetupColumn("Real");
            ImGui::TableSetupColumn("Imaginary");
            ImGui::TableHeadersRow();

            for (const quantum::StateInfo &stateInfo: currentState.states()) {
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

        if (currentState.qubitCount() == 1) {
            const quantum::BlochVector bloch =
                    currentState.blockVector();
            const quantum::BlochAngles angles =
                    currentState.blochAngles();

            ImGui::Spacing();
            ImGui::SeparatorText("Bloch Sphere");

            ImGui::TextDisabled("Vector and angles");

            ImGui::Text("(%.4f, %.4f, %.4f)", bloch.x, bloch.y, bloch.z);

            ImGui::Spacing();

            ImGui::Text("theta = %.4f rad", angles.theta);

            constexpr double epsilon = 1e-10;

            const bool isAtPole =
                    std::abs(bloch.x) < epsilon &&
                    std::abs(bloch.y) < epsilon;

            isAtPole
                ? ImGui::TextUnformatted("phi = undefined at the pole")
                : ImGui::Text("phi = %.4f rad", angles.phi);

            ImGui::Spacing();
            ImGui::TextDisabled("Visualization");

            blochSphereRenderer_.draw(bloch);
            ImGui::Text(
                "Depth: y = %.4f",
                bloch.y
            );
        }

        ImGui::Spacing();
        ImGui::SeparatorText("Debugger Controls");

        const float availableButtonWidth =
                ImGui::GetContentRegionAvail().x;

        const float buttonSpacing =
                ImGui::GetStyle().ItemSpacing.x;

        const float debuggerButtonWidth =
                (availableButtonWidth - buttonSpacing * 2.0F) / 3.0F;

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
                session.movePrevious();
            }

            if (
                ImGui::IsKeyPressed(ImGuiKey_RightArrow) &&
                snapshot.canMoveNext
            ) {
                session.moveNext();
            }

            if (ImGui::IsKeyPressed(ImGuiKey_R)) {
                session.restart();
            }
        }

        if (!snapshot.canMovePrevious) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Previous  [←]", ImVec2{debuggerButtonWidth, 0.0F})) {
            session.movePrevious();
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

        if (ImGui::Button("Next  [→]", ImVec2{debuggerButtonWidth, 0.0F})) {
            session.moveNext();
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

        if (ImGui::Button("Restart  [R]", ImVec2{debuggerButtonWidth, 0.0F})) {
            session.restart();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Return to the first instruction. Shortcut: R"
            );
        }
    }
}

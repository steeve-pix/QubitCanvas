#include "quantum_sim/gui/panels/GateLibraryPanel.hpp"

#include "imgui.h"
#include <utility>

namespace quantum_sim::gui {
    constexpr GateDescriptor singleQubitGates[] = {
        {
            "H",
            "Hadamard gate: creates or removes equal superposition."
        },
        {
            "X",
            "Pauli-X gate: flips |0> and |1>."
        },
        {
            "Y",
            "Pauli-Y gate: rotates the qubit around the Y axis."
        },
        {
            "Z",
            "Pauli-Z gate: changes the phase of |1>."
        },
        {
            "S",
            "S gate: applies a quarter-turn phase shift."
        },
        {
            "T",
            "T gate: applies an eighth-turn phase shift."
        }
    };

    constexpr GateDescriptor controlledGates[] = {
        {
            "CX",
            "Controlled-X gate: flips the target when the control is |1>."
        },
        {
            "CY",
            "Controlled-Y gate: applies the Pauli-Y gate to the target when the control is |1>."
        },
        {
            "CZ",
            "Controlled-Z gate: applies a phase flip to the target when the control is |1>."
        },
        {
            "SWAP",
            "SWAP gate: exchanges the states of two qubits."
        },
        {
            "iSWAP",
            "iSWAP gate: exchanges the |01⟩ and |10⟩ amplitudes and multiplies them by i."
        }
    };

    GateLibraryPanel::GateLibraryPanel(GateLibraryStyle style)
        : style_{std::move(style)} {
    }

    const GateLibraryStyle &GateLibraryPanel::style() const noexcept {
        return style_;
    }

    void GateLibraryPanel::setStyle(GateLibraryStyle style) {
        style_ = std::move(style);
    }

    void GateLibraryPanel::draw() {
        // Categories keep one-click gate selection compact as the palette grows.
        drawGateCategory("Single-qubit gates", singleQubitGates, std::size(singleQubitGates));

        ImGui::Spacing();

        drawGateCategory("Controlled gates", controlledGates, std::size(controlledGates));

        drawSelectionSummary();
    }

    bool GateLibraryPanel::drawGateButton(const char *label, const char *tooltip, bool selected) {
        if (selected) {
            // Push only the temporary selected colors, then pop after the button.
            ImGui::PushStyleColor(
                ImGuiCol_Button,
                style_.selectedButtonColor
            );

            ImGui::PushStyleColor(
                ImGuiCol_ButtonHovered,
                style_.selectedButtonHoveredColor
            );

            ImGui::PushStyleColor(
                ImGuiCol_ButtonActive,
                style_.selectedButtonActiveColor
            );
        }

        const bool clicked =
                ImGui::Button(
                    label,
                    ImVec2{
                        style_.gateButtonSize,
                        style_.gateButtonSize
                    }
                );

        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(
                ImGuiMouseCursor_Hand
            );

            ImGui::SetTooltip(
                "%s",
                tooltip
            );
        }

        if (selected) {
            ImGui::PopStyleColor(3);
        }

        return clicked;
    }

    bool GateLibraryPanel::isGateSelected(const char *gateName) const noexcept {
        return selectedGate_.has_value() && selectedGate_.value() == gateName;
    }

    bool GateLibraryPanel::canPlaceNextGateButton() const {
        // Predict the next button's right edge before calling SameLine().
        const float nextButtonRightEdge =
                ImGui::GetItemRectMax().x
                + ImGui::GetStyle().ItemSpacing.x
                + style_.gateButtonSize;

        const float contentRightEdge =
                ImGui::GetWindowPos().x
                + ImGui::GetWindowContentRegionMax().x;

        return nextButtonRightEdge <= contentRightEdge;
    }

    void GateLibraryPanel::drawGateCategory(const char *title, const GateDescriptor *gates, std::size_t gateCount) {
        ImGui::TextDisabled("%s", title);

        for (std::size_t index = 0; index < gateCount; ++index) {
            const GateDescriptor &gate =
                    gates[index];

            // Clicking a gate arms placement; the app consumes it after draw().
            if (drawGateButton(gate.name, gate.tooltip, isGateSelected(gate.name))) {
                selectedGate_ =
                        gate.name;
            }

            if (index + 1 < gateCount && canPlaceNextGateButton()) {
                ImGui::SameLine();
            }
        }
    }

    void GateLibraryPanel::drawSelectionSummary() {
        ImGui::Spacing();
        ImGui::SeparatorText("Selection");

        if (!selectedGate_.has_value()) {
            ImGui::TextDisabled(
                "No gate selected."
            );

            return;
        }

        ImGui::Text("Selected gate: %s",
                    selectedGate_->c_str());

        if (ImGui::Button("Clear selection")) {
            clearSelection();
        }
    }

    const std::optional<std::string> &GateLibraryPanel::selectedGate() const noexcept {
        return selectedGate_;
    }

    void GateLibraryPanel::clearSelection() noexcept {
        selectedGate_.reset();
    }

    std::optional<std::string> GateLibraryPanel::consumeSelectedGate() {
        if (!selectedGate_.has_value()) {
            return std::nullopt;
        }

        std::optional<std::string> result =
                std::move(selectedGate_);

        // Make consumeSelectedGate a one-shot event.
        selectedGate_.reset();

        return result;
    }
}

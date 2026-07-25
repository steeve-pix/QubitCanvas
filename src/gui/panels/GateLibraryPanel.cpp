#include "quantum_sim/gui/panels/GateLibraryPanel.hpp"
#include "quantum_sim/gates/QuantumGates.hpp"

#include "imgui.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <iomanip>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    [[nodiscard]] bool isRotationGate(const std::string_view gateName) noexcept {
        return gateName == "Rx" ||
               gateName == "Ry" ||
               gateName == "Rz";
    }

    [[nodiscard]] const char *gateTitle(const std::string_view gateName) {
        if (gateName == "H") {
            return "HADAMARD";
        }
        if (gateName == "X") {
            return "PAULI-X";
        }
        if (gateName == "Y") {
            return "PAULI-Y";
        }
        if (gateName == "Z") {
            return "PAULI-Z";
        }
        if (gateName == "S") {
            return "PHASE-S";
        }
        if (gateName == "T") {
            return "PHASE-T";
        }
        if (gateName == "Rx") {
            return "ROTATION-X";
        }
        if (gateName == "Ry") {
            return "ROTATION-Y";
        }
        if (gateName == "Rz") {
            return "ROTATION-Z";
        }
        if (gateName == "CX") {
            return "CONTROLLED-X";
        }
        if (gateName == "CY") {
            return "CONTROLLED-Y";
        }
        if (gateName == "CZ") {
            return "CONTROLLED-Z";
        }
        if (gateName == "SWAP") {
            return "SWAP";
        }
        if (gateName == "iSWAP") {
            return "ISWAP";
        }

        return "QUANTUM GATE";
    }

    [[nodiscard]] quantum_sim::math::ComplexMatrix gateMatrix(
        const std::string_view gateName,
        const double rotationAngleRadians
    ) {
        using namespace quantum_sim;

        if (gateName == "H") {
            return gates::hadamardGate();
        }
        if (gateName == "X") {
            return gates::xGate();
        }
        if (gateName == "Y") {
            return gates::yGate();
        }
        if (gateName == "Z") {
            return gates::zGate();
        }
        if (gateName == "S") {
            return gates::sGate();
        }
        if (gateName == "T") {
            return gates::tGate();
        }
        if (gateName == "Rx") {
            return gates::rxGate(rotationAngleRadians);
        }
        if (gateName == "Ry") {
            return gates::ryGate(rotationAngleRadians);
        }
        if (gateName == "Rz") {
            return gates::rzGate(rotationAngleRadians);
        }
        if (gateName == "CX") {
            return gates::cxGate();
        }
        if (gateName == "CY") {
            return gates::cyGate();
        }
        if (gateName == "CZ") {
            return gates::czGate();
        }
        if (gateName == "SWAP") {
            return gates::swapGate();
        }
        if (gateName == "iSWAP") {
            return gates::iSwapGate();
        }

        throw std::invalid_argument{
            "Unsupported gate matrix preview."
        };
    }

    [[nodiscard]] std::string formatScalar(const double value) {
        if (std::abs(value) < 1e-9) {
            return "0";
        }
        if (std::abs(value - 1.0) < 1e-9) {
            return "1";
        }
        if (std::abs(value + 1.0) < 1e-9) {
            return "-1";
        }

        std::ostringstream output;
        output << std::fixed << std::setprecision(3) << value;

        std::string text =
                output.str();

        while (!text.empty() && text.back() == '0') {
            text.pop_back();
        }

        if (!text.empty() && text.back() == '.') {
            text.pop_back();
        }

        return text;
    }

    [[nodiscard]] std::string formatComplex(
        const quantum_sim::math::Complex &value,
        const double displayMultiplier
    ) {
        double real =
                value.real() * displayMultiplier;

        double imaginary =
                value.imaginary() * displayMultiplier;

        if (std::abs(real) < 1e-9) {
            real = 0.0;
        }

        if (std::abs(imaginary) < 1e-9) {
            imaginary = 0.0;
        }

        if (imaginary == 0.0) {
            return formatScalar(real);
        }

        const double imaginaryMagnitude =
                std::abs(imaginary);

        const std::string imaginaryText =
                std::abs(imaginaryMagnitude - 1.0) < 1e-9
                    ? "i"
                    : formatScalar(imaginaryMagnitude) + "i";

        if (real == 0.0) {
            return imaginary < 0.0
                       ? "-" + imaginaryText
                       : imaginaryText;
        }

        return formatScalar(real) +
               (imaginary < 0.0 ? "-" : "+") +
               imaginaryText;
    }

    [[nodiscard]] std::vector<std::string> formattedMatrixRows(
        const quantum_sim::math::ComplexMatrix &matrix,
        const double displayMultiplier
    ) {
        std::vector<std::vector<std::string> > cells(
            matrix.rows(),
            std::vector<std::string>(matrix.columns())
        );

        std::size_t widestCell =
                1U;

        for (std::size_t row = 0; row < matrix.rows(); ++row) {
            for (std::size_t column = 0; column < matrix.columns(); ++column) {
                cells[row][column] =
                        formatComplex(
                            matrix.at(row, column),
                            displayMultiplier
                        );

                widestCell =
                        std::max(
                            widestCell,
                            cells[row][column].size()
                        );
            }
        }

        std::vector<std::string> rows;
        rows.reserve(matrix.rows());

        for (const auto &rowCells: cells) {
            std::string rowText =
                    "[ ";

            for (std::size_t column = 0; column < rowCells.size(); ++column) {
                rowText.append(
                    widestCell - rowCells[column].size(),
                    ' '
                );

                rowText +=
                        rowCells[column];

                if (column + 1U < rowCells.size()) {
                    rowText +=
                            "  ";
                }
            }

            rowText +=
                    " ]";

            rows.push_back(
                std::move(rowText)
            );
        }

        return rows;
    }
}

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

    constexpr GateDescriptor rotationGates[] = {
        {
            "Rx",
            "Rx gate: rotates around the Bloch X axis by the selected angle."
        },
        {
            "Ry",
            "Ry gate: rotates around the Bloch Y axis by the selected angle."
        },
        {
            "Rz",
            "Rz gate: rotates around the Bloch Z axis by the selected angle."
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

        drawRotationAngleControl();
        drawGateCategory("Rotation gates", rotationGates, std::size(rotationGates));

        ImGui::Spacing();

        drawGateCategory("Controlled gates", controlledGates, std::size(controlledGates));

        drawSelectionSummary();
    }

    bool GateLibraryPanel::drawGateButton(
        const GateDescriptor &gate,
        const bool selected
    ) {
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
                    gate.name,
                    ImVec2{
                        style_.gateButtonSize,
                        style_.gateButtonSize
                    }
                );

        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(
                ImGuiMouseCursor_Hand
            );

            drawGateMatrixTooltip(gate);
        }

        if (selected) {
            ImGui::PopStyleColor(3);
        }

        return clicked;
    }

    void GateLibraryPanel::drawGateMatrixTooltip(
        const GateDescriptor &gate
    ) const {
        const std::string_view gateName =
                gate.name;

        const math::ComplexMatrix matrix =
                gateMatrix(
                    gateName,
                    rotationAngleRadians_
                );

        const bool hadamard =
                gateName == "H";

        const double displayMultiplier =
                hadamard
                    ? std::numbers::sqrt2
                    : 1.0;

        const std::vector<std::string> matrixRows =
                formattedMatrixRows(
                    matrix,
                    displayMultiplier
                );

        const bool controlledOrSwap =
                matrix.rows() == 4U;

        const ImVec4 titleColor =
                controlledOrSwap
                    ? ImVec4{1.0F, 0.72F, 0.28F, 1.0F}
                    : ImVec4{0.27F, 0.83F, 1.0F, 1.0F};

        ImGui::PushStyleVar(
            ImGuiStyleVar_WindowPadding,
            ImVec2{13.0F, 11.0F}
        );

        ImGui::PushStyleVar(
            ImGuiStyleVar_PopupRounding,
            6.0F
        );

        ImGui::PushStyleColor(
            ImGuiCol_PopupBg,
            ImVec4{0.027F, 0.043F, 0.075F, 0.98F}
        );

        ImGui::PushStyleColor(
            ImGuiCol_Border,
            ImVec4{0.21F, 0.38F, 0.56F, 0.80F}
        );

        ImGui::SetNextWindowSizeConstraints(
            ImVec2{190.0F, 0.0F},
            ImVec2{430.0F, FLT_MAX}
        );

        ImGui::BeginTooltip();
        ImGui::TextColored(
            titleColor,
            "%s",
            gateTitle(gateName)
        );

        if (isRotationGate(gateName)) {
            ImGui::TextDisabled(
                "theta %.3f rad",
                rotationAngleRadians_
            );
        }

        if (hadamard) {
            ImGui::TextDisabled("factor 1/sqrt(2)");
        }

        ImGui::Spacing();

        for (const std::string &row: matrixRows) {
            ImGui::TextUnformatted(
                row.c_str()
            );
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::PushTextWrapPos(
            ImGui::GetCursorPosX() + 310.0F
        );
        ImGui::TextDisabled(
            "%s",
            gate.tooltip
        );
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
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
            if (drawGateButton(gate, isGateSelected(gate.name))) {
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

        const bool rotationSelected =
                selectedGate_.value() == "Rx" ||
                selectedGate_.value() == "Ry" ||
                selectedGate_.value() == "Rz";

        if (rotationSelected) {
            ImGui::Text(
                "Angle: %.3f rad",
                rotationAngleRadians_
            );
        }

        if (ImGui::Button("Clear selection")) {
            clearSelection();
        }
    }

    const std::optional<std::string> &GateLibraryPanel::selectedGate() const noexcept {
        return selectedGate_;
    }

    double GateLibraryPanel::rotationAngleRadians() const noexcept {
        return static_cast<double>(rotationAngleRadians_);
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

    void GateLibraryPanel::drawRotationAngleControl() {
        ImGui::TextDisabled("Rotation angle");
        ImGui::SetNextItemWidth(-1.0F);
        ImGui::SliderFloat(
            "##RotationAngleRadians",
            &rotationAngleRadians_,
            -std::numbers::pi_v<float>,
            std::numbers::pi_v<float>,
            "%.3f rad",
            ImGuiSliderFlags_AlwaysClamp
        );

        const float availableWidth =
                ImGui::GetContentRegionAvail().x;

        const float spacing =
                ImGui::GetStyle().ItemSpacing.x;

        const float shortcutWidth =
                (availableWidth - spacing * 2.0F) / 3.0F;

        if (ImGui::Button("pi/4", ImVec2{shortcutWidth, 0.0F})) {
            rotationAngleRadians_ =
                    std::numbers::pi_v<float> / 4.0F;
        }

        ImGui::SameLine();

        if (ImGui::Button("pi/2", ImVec2{shortcutWidth, 0.0F})) {
            rotationAngleRadians_ =
                    std::numbers::pi_v<float> / 2.0F;
        }

        ImGui::SameLine();

        if (ImGui::Button("pi", ImVec2{shortcutWidth, 0.0F})) {
            rotationAngleRadians_ =
                    std::numbers::pi_v<float>;
        }
    }
}

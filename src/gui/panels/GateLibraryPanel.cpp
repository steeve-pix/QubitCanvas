#include "quantum_sim/gui/panels/GateLibraryPanel.hpp"
#include "quantum_sim/gui/GateNotation.hpp"
#include "quantum_sim/gui/QuantumNotation.hpp"
#include "quantum_sim/gates/QuantumGates.hpp"

#include "imgui.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    [[nodiscard]] bool usesTheta(const std::string_view gateName) noexcept {
        return gateName == "Rx" ||
               gateName == "Ry" ||
               gateName == "Rz" ||
               gateName == "P" ||
               gateName == "U" ||
               gateName == "CP" ||
               gateName == "CRx" ||
               gateName == "CRy" ||
               gateName == "CRz" ||
               gateName == "RXX" ||
               gateName == "RYY" ||
               gateName == "RZZ" ||
               gateName == "fSim";
    }

    [[nodiscard]] bool usesThreeAngles(
        const std::string_view gateName
    ) noexcept {
        return gateName == "U";
    }

    [[nodiscard]] bool usesPhi(
        const std::string_view gateName
    ) noexcept {
        return gateName == "U" ||
               gateName == "fSim";
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
        if (gateName == "Sdg") {
            return "PHASE-S DAGGER";
        }
        if (gateName == "T") {
            return "PHASE-T";
        }
        if (gateName == "Tdg") {
            return "PHASE-T DAGGER";
        }
        if (gateName == "SX") {
            return "SQUARE ROOT X";
        }
        if (gateName == "SXdg") {
            return "SQUARE ROOT X DAGGER";
        }
        if (gateName == "P") {
            return "PHASE";
        }
        if (gateName == "U") {
            return "UNIVERSAL U";
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
        if (gateName == "CH") {
            return "CONTROLLED-HADAMARD";
        }
        if (gateName == "CS") {
            return "CONTROLLED-S";
        }
        if (gateName == "CSdg") {
            return "CONTROLLED-S DAGGER";
        }
        if (gateName == "CT") {
            return "CONTROLLED-T";
        }
        if (gateName == "CTdg") {
            return "CONTROLLED-T DAGGER";
        }
        if (gateName == "CP") {
            return "CONTROLLED-PHASE";
        }
        if (gateName == "CRx") {
            return "CONTROLLED ROTATION-X";
        }
        if (gateName == "CRy") {
            return "CONTROLLED ROTATION-Y";
        }
        if (gateName == "CRz") {
            return "CONTROLLED ROTATION-Z";
        }
        if (gateName == "RXX") {
            return "XX INTERACTION";
        }
        if (gateName == "RYY") {
            return "YY INTERACTION";
        }
        if (gateName == "RZZ") {
            return "ZZ INTERACTION";
        }
        if (gateName == "DCX") {
            return "DOUBLE-CNOT";
        }
        if (gateName == "ECR") {
            return "ECHOED CROSS-RESONANCE";
        }
        if (gateName == "sqrtSWAP") {
            return "SQUARE ROOT SWAP";
        }
        if (gateName == "fSim") {
            return "FERMIONIC SIMULATION";
        }
        if (gateName == "CCX") {
            return "CONTROLLED-CONTROLLED-X";
        }
        if (gateName == "CSWAP") {
            return "CONTROLLED-SWAP";
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
        const quantum_sim::gui::GateParameters &parameters
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
        if (gateName == "Sdg") {
            return gates::sDaggerGate();
        }
        if (gateName == "T") {
            return gates::tGate();
        }
        if (gateName == "Tdg") {
            return gates::tDaggerGate();
        }
        if (gateName == "SX") {
            return gates::sxGate();
        }
        if (gateName == "SXdg") {
            return gates::sxDaggerGate();
        }
        if (gateName == "P") {
            return gates::phaseGate(parameters.thetaRadians);
        }
        if (gateName == "U") {
            return gates::uGate(
                parameters.thetaRadians,
                parameters.phiRadians,
                parameters.lambdaRadians
            );
        }
        if (gateName == "Rx") {
            return gates::rxGate(parameters.thetaRadians);
        }
        if (gateName == "Ry") {
            return gates::ryGate(parameters.thetaRadians);
        }
        if (gateName == "Rz") {
            return gates::rzGate(parameters.thetaRadians);
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
        if (gateName == "CH") {
            return gates::chGate();
        }
        if (gateName == "CS") {
            return gates::csGate();
        }
        if (gateName == "CSdg") {
            return gates::csDaggerGate();
        }
        if (gateName == "CT") {
            return gates::ctGate();
        }
        if (gateName == "CTdg") {
            return gates::ctDaggerGate();
        }
        if (gateName == "CP") {
            return gates::controlledPhaseGate(
                parameters.thetaRadians
            );
        }
        if (gateName == "CRx") {
            return gates::crxGate(parameters.thetaRadians);
        }
        if (gateName == "CRy") {
            return gates::cryGate(parameters.thetaRadians);
        }
        if (gateName == "CRz") {
            return gates::crzGate(parameters.thetaRadians);
        }
        if (gateName == "RXX") {
            return gates::rxxGate(parameters.thetaRadians);
        }
        if (gateName == "RYY") {
            return gates::ryyGate(parameters.thetaRadians);
        }
        if (gateName == "RZZ") {
            return gates::rzzGate(parameters.thetaRadians);
        }
        if (gateName == "DCX") {
            return gates::dcxGate();
        }
        if (gateName == "ECR") {
            return gates::ecrGate();
        }
        if (gateName == "sqrtSWAP") {
            return gates::squareRootSwapGate();
        }
        if (gateName == "fSim") {
            return gates::fSimGate(
                parameters.thetaRadians,
                parameters.phiRadians
            );
        }
        if (gateName == "CCX") {
            return gates::ccxGate();
        }
        if (gateName == "CSWAP") {
            return gates::cSwapGate();
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

    [[nodiscard]] std::vector<std::vector<std::string> > formattedMatrixCells(
        const quantum_sim::math::ComplexMatrix &matrix,
        const double displayMultiplier
    ) {
        std::vector<std::vector<std::string> > cells(
            matrix.rows(),
            std::vector<std::string>(matrix.columns())
        );

        for (std::size_t row = 0; row < matrix.rows(); ++row) {
            for (std::size_t column = 0; column < matrix.columns(); ++column) {
                cells[row][column] =
                        quantum_sim::gui::notation::formatComplex(
                            matrix.at(row, column),
                            displayMultiplier
                        );
            }
        }

        return cells;
    }

    [[nodiscard]] bool isZero(
        const quantum_sim::math::Complex &value
    ) noexcept {
        return std::abs(value.real()) < 1e-9 &&
               std::abs(value.imaginary()) < 1e-9;
    }

}

namespace quantum_sim::gui {
    constexpr GateDescriptor coreSingleQubitGates[] = {
        {
            "H",
            "Hadamard gate: creates or removes equal superposition."
        },
        {
            "X",
            "Pauli-X gate: flips |0\xE2\x9F\xA9 and |1\xE2\x9F\xA9."
        },
        {
            "Y",
            "Pauli-Y gate: rotates the qubit around the Y axis."
        },
        {
            "Z",
            "Pauli-Z gate: changes the phase of |1\xE2\x9F\xA9."
        },
        {
            "S",
            "S gate: applies a quarter-turn phase shift."
        },
        {
            "Sdg",
            "Inverse S gate: rotates the |1\xE2\x9F\xA9 phase by -\xCF\x80/2 radians. "
            "It cancels the S gate."
        },
        {
            "T",
            "T gate: applies an eighth-turn phase shift."
        },
        {
            "Tdg",
            "Inverse T gate: rotates the |1\xE2\x9F\xA9 phase by -\xCF\x80/4 radians. "
            "It cancels T and is used in decompositions such as Toffoli."
        },
        {
            "SX",
            "Square-root X: two applications equal one Pauli-X flip."
        },
        {
            "SXdg",
            "Inverse square-root X: reverses SX and rotates in the opposite direction."
        }
    };

    constexpr GateDescriptor parameterizedSingleQubitGates[] = {
        {
            "P",
            "Phase gate: applies exp(i theta) to |1\xE2\x9F\xA9 without changing probability."
        },
        {
            "U",
            "Universal gate: represents any one-qubit unitary using theta, phi, and lambda."
        },
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

    constexpr GateDescriptor coreTwoQubitGates[] = {
        {
            "CX",
            "Controlled-X gate: flips the target when the control is |1\xE2\x9F\xA9."
        },
        {
            "CY",
            "Controlled-Y gate: applies the Pauli-Y gate to the target when the control is |1\xE2\x9F\xA9."
        },
        {
            "CZ",
            "Controlled-Z gate: applies a phase flip to the target when the control is |1\xE2\x9F\xA9."
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

    constexpr GateDescriptor parameterizedControlledGates[] = {
        {
            "CP",
            "Controlled phase: applies exp(i theta) only when both qubits are |1\xE2\x9F\xA9."
        },
        {
            "CRx",
            "Controlled Rx: rotates the target around X only when the control is |1\xE2\x9F\xA9."
        },
        {
            "CRy",
            "Controlled Ry: rotates the target around Y only when the control is |1\xE2\x9F\xA9."
        },
        {
            "CRz",
            "Controlled Rz: rotates the target around Z only when the control is |1\xE2\x9F\xA9."
        }
    };

    constexpr GateDescriptor interactionGates[] = {
        {
            "RXX",
            "XX interaction: jointly rotates two qubits through the X tensor X coupling."
        },
        {
            "RYY",
            "YY interaction: jointly rotates two qubits through the Y tensor Y coupling."
        },
        {
            "RZZ",
            "ZZ interaction: adds parity-dependent phase through the Z tensor Z coupling."
        }
    };

    constexpr GateDescriptor advancedControlledGates[] = {
        {
            "CH",
            "Controlled Hadamard: mixes the target only when the control is |1\xE2\x9F\xA9."
        },
        {
            "CS",
            "Controlled S: applies a pi/2 target phase only in the active control branch."
        },
        {
            "CSdg",
            "Controlled inverse S: reverses the controlled pi/2 phase."
        },
        {
            "CT",
            "Controlled T: applies a pi/4 target phase only in the active control branch."
        },
        {
            "CTdg",
            "Controlled inverse T: reverses the controlled pi/4 phase."
        }
    };

    constexpr GateDescriptor nativeTwoQubitGates[] = {
        {
            "DCX",
            "Double-CNOT: applies CX in both directions as one two-qubit permutation."
        },
        {
            "ECR",
            "Echoed cross-resonance: a calibrated entangling primitive used by superconducting hardware."
        },
        {
            "sqrtSWAP",
            "Square-root SWAP: a half exchange whose second application equals SWAP."
        },
        {
            "fSim",
            "Fermionic simulation gate: combines excitation exchange theta with conditional phase phi."
        }
    };

    constexpr GateDescriptor threeQubitGates[] = {
        {
            "CCX",
            "Toffoli gate: flips the target only when both controls are |1\xE2\x9F\xA9."
        },
        {
            "CSWAP",
            "Fredkin gate: exchanges two targets only when the control is |1\xE2\x9F\xA9."
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
        gatePage_ =
                std::min(
                    gatePage_,
                    std::size_t{2}
                );

        if (gatePage_ == 0U) {
            drawGateCategory(
                "Core single-qubit",
                coreSingleQubitGates,
                std::size(coreSingleQubitGates)
            );

            ImGui::Spacing();

            drawGateCategory(
                "Core two-qubit",
                coreTwoQubitGates,
                std::size(coreTwoQubitGates)
            );
        } else if (gatePage_ == 1U) {
            drawGateCategory(
                "Parameterized single-qubit",
                parameterizedSingleQubitGates,
                std::size(parameterizedSingleQubitGates)
            );

            ImGui::Spacing();

            drawGateCategory(
                "Parameterized controlled",
                parameterizedControlledGates,
                std::size(parameterizedControlledGates)
            );

            ImGui::Spacing();

            drawGateCategory(
                "Two-qubit interactions",
                interactionGates,
                std::size(interactionGates)
            );
        } else {
            drawGateCategory(
                "Advanced controlled",
                advancedControlledGates,
                std::size(advancedControlledGates)
            );

            ImGui::Spacing();

            drawGateCategory(
                "Native and exchange",
                nativeTwoQubitGates,
                std::size(nativeTwoQubitGates)
            );

            ImGui::Spacing();

            drawGateCategory(
                "Three-qubit",
                threeQubitGates,
                std::size(threeQubitGates)
            );
        }

        if (
            selectedGate_.has_value() &&
            usesTheta(selectedGate_.value())
        ) {
            ImGui::Spacing();
            drawParameterizedControls();
        }

        ImGui::Spacing();
        drawPageControls();

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
                    gate_notation::displayName(
                        gate.name
                    ).data(),
                    ImVec2{
                        style_.gateButtonSize,
                        style_.gateButtonSize
                    }
                );

        bool dragStarted = false;

        if (
            ImGui::BeginDragDropSource(
                ImGuiDragDropFlags_SourceAllowNullID
            )
        ) {
            ImGui::SetDragDropPayload(
                "QUBITCANVAS_GATE",
                gate.name,
                std::char_traits<char>::length(gate.name) + 1U,
                ImGuiCond_Once
            );

            dragStarted =
                    !selected;

            ImGui::Text(
                "Place %s",
                gate_notation::displayName(
                    gate.name
                ).data()
            );

            ImGui::EndDragDropSource();
        }

        if (
            ImGui::IsItemHovered(
                ImGuiHoveredFlags_DelayNormal
            )
        ) {
            ImGui::SetMouseCursor(
                ImGuiMouseCursor_Hand
            );

            drawGateMatrixTooltip(gate);
        }

        if (selected) {
            ImGui::PopStyleColor(3);
        }

        return clicked || dragStarted;
    }

    void GateLibraryPanel::drawGateMatrixTooltip(
        const GateDescriptor &gate
    ) const {
        const std::string_view gateName =
                gate.name;

        const math::ComplexMatrix matrix =
                gateMatrix(
                    gateName,
                    gateParameters()
                );

        const bool hadamard =
                gateName == "H";

        const bool squareRootX =
                gateName == "SX" ||
                gateName == "SXdg";

        const bool squareRootSwap =
                gateName == "sqrtSWAP";

        const bool echoedCrossResonance =
                gateName == "ECR";

        const double displayMultiplier =
                hadamard ||
                echoedCrossResonance
                    ? std::numbers::sqrt2
                    : squareRootX ||
                      squareRootSwap
                        ? 2.0
                        : 1.0;

        const std::vector<std::vector<std::string> > matrixCells =
                formattedMatrixCells(
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

        const ImGuiViewport *viewport =
                ImGui::GetMainViewport();

        const ImVec2 pointer =
                ImGui::GetMousePos();

        const float tooltipWidth =
                matrix.rows() >= 8U
                    ? 520.0F
                    : 300.0F;

        const float estimatedTooltipHeight =
                matrix.rows() >= 8U
                    ? 360.0F
                    : 220.0F;
        constexpr float pointerOffset = 14.0F;

        const float viewportRight =
                viewport->WorkPos.x + viewport->WorkSize.x;

        const float viewportBottom =
                viewport->WorkPos.y + viewport->WorkSize.y;

        ImVec2 tooltipPosition{
            pointer.x + pointerOffset,
            pointer.y + pointerOffset
        };

        if (tooltipPosition.x + tooltipWidth > viewportRight) {
            tooltipPosition.x =
                    pointer.x - tooltipWidth - pointerOffset;
        }

        if (
            tooltipPosition.y + estimatedTooltipHeight >
            viewportBottom
        ) {
            tooltipPosition.y =
                    pointer.y -
                    estimatedTooltipHeight -
                    pointerOffset;
        }

        ImGui::SetNextWindowPos(
            tooltipPosition,
            ImGuiCond_Always
        );

        ImGui::SetNextWindowSizeConstraints(
            ImVec2{tooltipWidth, 0.0F},
            ImVec2{tooltipWidth, FLT_MAX}
        );

        ImGui::BeginTooltip();
        ImGui::TextColored(
            titleColor,
            "%s",
            gateTitle(gateName)
        );

        if (usesTheta(gateName)) {
            const std::string angleText =
                    notation::formatAngleMeasurement(
                        rotationAngleRadians_
                    );

            ImGui::TextDisabled(
                "\xCE\xB8 = %s",
                angleText.c_str()
            );
        }

        if (usesPhi(gateName)) {
            const std::string phiText =
                    notation::formatAngleMeasurement(
                        phiAngleRadians_
                    );

            ImGui::TextDisabled(
                "\xCF\x86 = %s",
                phiText.c_str()
            );
        }

        if (usesThreeAngles(gateName)) {
            const std::string lambdaText =
                    notation::formatAngleMeasurement(
                        lambdaAngleRadians_
                    );

            ImGui::TextDisabled(
                "\xCE\xBB = %s",
                lambdaText.c_str()
            );
        }

        if (hadamard) {
            ImGui::TextDisabled("factor 1/\xE2\x88\x9A""2");
        }

        if (squareRootX) {
            ImGui::TextDisabled("factor 1/2");
        }

        if (squareRootSwap) {
            ImGui::TextDisabled("factor 1/2");
        }

        if (echoedCrossResonance) {
            ImGui::TextDisabled("factor 1/\xE2\x88\x9A""2");
        }

        ImGui::Spacing();

        float widestCell = 0.0F;

        for (const auto &row : matrixCells) {
            for (const std::string &cell : row) {
                widestCell =
                        std::max(
                            widestCell,
                            ImGui::CalcTextSize(cell.c_str()).x
                        );
            }
        }

        const float cellWidth =
                std::max(42.0F, widestCell + 14.0F);

        const float rowHeight =
                ImGui::GetTextLineHeightWithSpacing();

        const ImVec2 matrixOrigin =
                ImGui::GetCursorScreenPos();

        const ImVec4 zeroColor{
            0.31F,
            0.36F,
            0.44F,
            1.0F
        };

        for (std::size_t row = 0; row < matrix.rows(); ++row) {
            const float rowY =
                    matrixOrigin.y +
                    static_cast<float>(row) * rowHeight;

            ImGui::SetCursorScreenPos(
                ImVec2{matrixOrigin.x, rowY}
            );
            ImGui::TextDisabled("[");

            for (std::size_t column = 0; column < matrix.columns(); ++column) {
                ImGui::SetCursorScreenPos(
                    ImVec2{
                        matrixOrigin.x + 20.0F +
                        static_cast<float>(column) * cellWidth,
                        rowY
                    }
                );

                const std::string &cell =
                        matrixCells[row][column];

                if (isZero(matrix.at(row, column))) {
                    ImGui::TextColored(
                        zeroColor,
                        "%s",
                        cell.c_str()
                    );
                } else {
                    ImGui::TextUnformatted(cell.c_str());
                }
            }

            ImGui::SetCursorScreenPos(
                ImVec2{
                    matrixOrigin.x + 20.0F +
                    static_cast<float>(matrix.columns()) * cellWidth,
                    rowY
                }
            );
            ImGui::TextDisabled("]");
        }

        ImGui::SetCursorScreenPos(
            ImVec2{
                matrixOrigin.x,
                matrixOrigin.y +
                static_cast<float>(matrix.rows()) * rowHeight
            }
        );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("Explanation");
        ImGui::PushTextWrapPos(
            ImGui::GetCursorPosX() + 260.0F
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
                selectionChanged_ = true;
            }

            if (index + 1 < gateCount && canPlaceNextGateButton()) {
                ImGui::SameLine();
            }
        }
    }

    const std::optional<std::string> &GateLibraryPanel::selectedGate() const noexcept {
        return selectedGate_;
    }

    GateParameters GateLibraryPanel::gateParameters() const noexcept {
        return GateParameters{
            static_cast<double>(rotationAngleRadians_),
            static_cast<double>(phiAngleRadians_),
            static_cast<double>(lambdaAngleRadians_)
        };
    }

    void GateLibraryPanel::setPage(
        const std::size_t pageIndex
    ) noexcept {
        gatePage_ =
                std::min(
                    pageIndex,
                    std::size_t{2}
                );
    }

    std::size_t GateLibraryPanel::page() const noexcept {
        return gatePage_;
    }

    void GateLibraryPanel::selectGate(std::string gateName) {
        selectedGate_ =
                std::move(gateName);
        selectionChanged_ = false;
    }

    void GateLibraryPanel::clearSelection() noexcept {
        selectedGate_.reset();
        selectionChanged_ = false;
    }

    std::optional<std::string> GateLibraryPanel::consumeSelectedGate() {
        if (
            !selectionChanged_ ||
            !selectedGate_.has_value()
        ) {
            return std::nullopt;
        }

        selectionChanged_ = false;

        return selectedGate_;
    }

    void GateLibraryPanel::drawParameterizedControls() {
        const auto drawAngle =
                [](
                    const char *label,
                    const char *identifier,
                    float &angleRadians
                ) {
            ImGui::TextDisabled("%s", label);
            ImGui::SetNextItemWidth(-1.0F);

            float anglePi =
                    angleRadians /
                    std::numbers::pi_v<float>;

            if (
                ImGui::SliderFloat(
                    identifier,
                    &anglePi,
                    -1.0F,
                    1.0F,
                    "%.2f\xCF\x80",
                    ImGuiSliderFlags_AlwaysClamp
                )
            ) {
                angleRadians =
                        anglePi *
                        std::numbers::pi_v<float>;
            }

            const std::string exactAngle =
                    notation::formatAngleMeasurement(
                        angleRadians
                    );

            ImGui::TextDisabled(
                "%s = %s",
                label,
                exactAngle.c_str()
            );
        };

        drawAngle(
            "\xCE\xB8",
            "##ThetaAnglePi",
            rotationAngleRadians_
        );

        if (
            selectedGate_.has_value() &&
            usesPhi(selectedGate_.value())
        ) {
            drawAngle(
                "\xCF\x86",
                "##PhiAnglePi",
                phiAngleRadians_
            );
        }

        if (
            selectedGate_.has_value() &&
            usesThreeAngles(selectedGate_.value())
        ) {
            drawAngle(
                "\xCE\xBB",
                "##LambdaAnglePi",
                lambdaAngleRadians_
            );
        }

        const float availableWidth =
                ImGui::GetContentRegionAvail().x;

        const float spacing =
                ImGui::GetStyle().ItemSpacing.x;

        const float shortcutWidth =
                (availableWidth - spacing * 2.0F) / 3.0F;

        if (ImGui::Button("\xCF\x80/4", ImVec2{shortcutWidth, 0.0F})) {
            rotationAngleRadians_ =
                    std::numbers::pi_v<float> / 4.0F;
        }

        ImGui::SameLine();

        if (ImGui::Button("\xCF\x80/2", ImVec2{shortcutWidth, 0.0F})) {
            rotationAngleRadians_ =
                    std::numbers::pi_v<float> / 2.0F;
        }

        ImGui::SameLine();

        if (ImGui::Button("\xCF\x80", ImVec2{shortcutWidth, 0.0F})) {
            rotationAngleRadians_ =
                    std::numbers::pi_v<float>;
        }
    }

    void GateLibraryPanel::drawPageControls() {
        constexpr std::size_t pageCount = 3U;

        if (
            !ImGui::BeginTable(
                "GatePageControls",
                3,
                ImGuiTableFlags_SizingStretchSame
            )
        ) {
            return;
        }

        ImGui::TableNextColumn();

        const bool firstPage =
                gatePage_ == 0U;

        if (firstPage) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("<##PreviousGatePage", ImVec2{-1.0F, 0.0F})) {
            --gatePage_;
        }

        if (firstPage) {
            ImGui::EndDisabled();
        }

        if (
            ImGui::IsItemHovered(
                ImGuiHoveredFlags_DelayShort |
                ImGuiHoveredFlags_AllowWhenDisabled
            )
        ) {
            ImGui::SetTooltip("Previous gate page");
        }

        ImGui::TableNextColumn();

        const std::string label =
                std::to_string(gatePage_ + 1U) +
                " / " +
                std::to_string(pageCount);

        const float offset =
                std::max(
                    0.0F,
                    (
                        ImGui::GetContentRegionAvail().x -
                        ImGui::CalcTextSize(label.c_str()).x
                    ) * 0.5F
                );

        ImGui::SetCursorPosX(
            ImGui::GetCursorPosX() +
            offset
        );
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label.c_str());

        ImGui::TableNextColumn();

        const bool lastPage =
                gatePage_ + 1U >= pageCount;

        if (lastPage) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button(">##NextGatePage", ImVec2{-1.0F, 0.0F})) {
            ++gatePage_;
        }

        if (lastPage) {
            ImGui::EndDisabled();
        }

        if (
            ImGui::IsItemHovered(
                ImGuiHoveredFlags_DelayShort |
                ImGuiHoveredFlags_AllowWhenDisabled
            )
        ) {
            ImGui::SetTooltip("Next gate page");
        }

        ImGui::EndTable();
    }
}

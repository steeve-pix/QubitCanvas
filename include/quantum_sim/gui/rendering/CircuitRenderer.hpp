#pragma once
#include <string>

#include "CircuitStyle.hpp"
#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/debug/DebuggerSession.hpp"
#include <optional>
#include <cstddef>
struct ImDrawList;
struct ImVec2;

namespace quantum_sim::gui {
    class CircuitRenderer {
    public:
        void draw(const circuit::QuantumCircuit &circuit, const debug::DebuggerSnapshot &snapshot,
                  const std::optional<std::string> &
                  pendingGate);

        explicit CircuitRenderer(CircuitStyle style = {});

        [[nodiscard]] const CircuitStyle &style() const noexcept;

        void setStyle(CircuitStyle style);

        [[nodiscard]] std::optional<std::size_t> selectedInstructionIndex() const noexcept;

        void clearSelection() noexcept;

        [[nodiscard]] bool hasCompletedControlledPlacement() const noexcept;

    private:
        void drawGate(ImDrawList *drawList, const ImVec2 &center, const std::string &label, bool highlighted,
                      bool hovered, bool selected);

        std::optional<std::size_t> selectedInstructionIndex_;

        CircuitStyle style_;
        std::optional<std::size_t> pendingControlQubit_;
        std::optional<std::size_t> pendingTargetQubit_;
    };
}

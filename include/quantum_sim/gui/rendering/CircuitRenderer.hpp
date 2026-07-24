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
    struct ControlledPlacement {
        std::size_t controlQubit;
        std::size_t targetQubit;
    };

    struct SingleQubitPlacement {
        std::string gateName;
        std::size_t targetQubit;
    };

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

        [[nodiscard]] std::optional<ControlledPlacement> completedControlledPlacement() const noexcept;

        [[nodiscard]] std::optional<ControlledPlacement> consumeCompletedControlledPlacement() noexcept;

        [[nodiscard]] std::optional<SingleQubitPlacement> consumeCompletedSingleQubitPlacement();

        [[nodiscard]] bool hasPendingControlQubit() const noexcept;

        void cancelPlacement() noexcept;


    private:
        void drawGate(ImDrawList *drawList, const ImVec2 &center, const std::string &label, bool highlighted,
                      bool hovered, bool selected, bool placementPreview);

        std::optional<std::size_t> selectedInstructionIndex_;

        CircuitStyle style_;
        std::optional<std::size_t> pendingControlQubit_;
        std::optional<std::size_t> pendingTargetQubit_;
        std::optional<SingleQubitPlacement> completedSingleQubitPlacement_;
    };
}

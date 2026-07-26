#pragma once

#include "quantum_sim/debug/DebuggerSession.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace quantum_sim::gui::density_volume {
    /**
     * One row/column bucket used when a register is larger than the render grid.
     */
    struct DensityBin {
        std::size_t firstState{};
        std::size_t lastState{};
        std::size_t strongestState{};
        double probability{};
        double aggregateReal{};
        double aggregateImaginary{};
        double strongestPhase{};
        std::string label;
    };

    /**
     * Numerically derived value for one density-matrix cell.
     */
    struct DensityCell {
        std::size_t row{};
        std::size_t column{};
        double magnitude{};
        double intensity{};
        double phaseRadians{};
        double real{};
        double imaginary{};
    };

    /**
     * Density matrix produced by one state in the debugger history.
     */
    struct DensityLayer {
        std::size_t index{};
        std::size_t dimension{};
        std::size_t sourceStateCount{};
        bool bucketed{};
        std::string label;
        std::vector<DensityBin> bins;
        std::vector<DensityCell> cells;

        /**
         * Returns a matrix cell using row-major indexing.
         *
         * @throws std::out_of_range when row or column is outside dimension.
         */
        [[nodiscard]] const DensityCell &cellAt(
            std::size_t row,
            std::size_t column
        ) const;
    };

    /**
     * Initial-state matrix plus every post-instruction density matrix.
     */
    struct DensityStack {
        std::vector<DensityLayer> layers;
        double maximumMagnitude{};
        std::uint64_t fingerprint{};
    };

    /**
     * Converts debugger states into exact or explicitly bucketed density data.
     */
    class DensityModel {
    public:
        /**
         * Builds the complete debugger density history.
         *
         * Registers with at most maximumDimension states keep every exact
         * density-matrix cell. Larger registers are grouped into probability
         * preserving row/column buckets.
         *
         * @param session Debugger trace whose initial and post-step states are converted.
         * @param maximumDimension Largest square matrix dimension to generate.
         * @return Shared numerical model for both 3D and 2D Density Volume views.
         */
        [[nodiscard]] static DensityStack build(
            const debug::DebuggerSession &session,
            std::size_t maximumDimension = 16U
        );
    };
}

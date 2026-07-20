#include "quantum_sim/visualization/ConsoleVisualizer.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <string>
#include <stdexcept>

namespace quantum_sim::visualization {
    void printProbabilityBars(const quantum::QuantumRegister &state, std::ostream &output, std::size_t barWidth) {
        for (const quantum::StateInfo &info: state.states()) {
            const double clampedProbability =
                    std::clamp(info.probability, 0.0, 1.0);

            const std::size_t filledWidth =
                    static_cast<std::size_t>(clampedProbability * static_cast<double>(barWidth));

            const std::string filled(filledWidth, '#');

            const std::string empty(barWidth - filledWidth, ' ');

            output
                    << info.label
                    << " ["
                    << filled
                    << empty
                    << "] "
                    << std::fixed
                    << std::setprecision(2)
                    << clampedProbability * 100.0
                    << "%\n";
        }
    }

    void printShotBars(const quantum::QuantumRegister &state, const std::vector<std::size_t> &counts,
                       std::ostream &output, std::size_t barWidth) {
        if (counts.size() != state.stateCount()) {
            throw std::invalid_argument{"Shot count must match the register state count."};
        }

        std::size_t totalShots = 0;
        for (const std::size_t count: counts) {
            totalShots += count;
        }
        for (std::size_t stateIndex = 0;
             stateIndex < state.stateCount();
             ++stateIndex) {
            const double frequency =
                    totalShots == 0
                        ? 0.0
                        : static_cast<double>(counts[stateIndex])
                          / static_cast<double>(totalShots);

            const std::size_t filledWidth =
                    static_cast<std::size_t>(
                        frequency * static_cast<double>(barWidth)
                    );

            const std::string filled(filledWidth, '#');
            const std::string empty(
                barWidth - filledWidth,
                ' '
            );

            output
                    << state.basisStateLabel(stateIndex)
                    << " ["
                    << filled
                    << empty
                    << "] "
                    << counts[stateIndex]
                    << " ("
                    << std::fixed
                    << std::setprecision(2)
                    << frequency * 100.0
                    << "%)\n";
        }
    }
}

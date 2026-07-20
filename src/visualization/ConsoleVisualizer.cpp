#include "quantum_sim/visualization/ConsoleVisualizer.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <string>

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
}

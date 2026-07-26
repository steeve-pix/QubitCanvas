#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/gates/QuantumGates.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>
#include <utility>
#include <algorithm>
#include <limits>

namespace quantum_sim::quantum {
    QuantumRegister::QuantumRegister(std::size_t qubitCount, math::ComplexVector amplitudes)
        : qubitCount_(qubitCount), amplitudes_(amplitudes) {
        if (qubitCount == 0)
            throw std::invalid_argument{
                "A quantum register must contain at least one qubit."
            };
        if (qubitCount_ >= std::numeric_limits<std::size_t>::digits)
            throw std::invalid_argument{
                "Qubit count is too large to represent its state count."
            };

        // A register with n qubits has exactly 2^n basis amplitudes.
        const std::size_t expectedStateCount =
                std::size_t{1} << qubitCount_;

        if (amplitudes.size() != expectedStateCount)
            throw std::invalid_argument{
                "Amplitude count must equal 2 raised to the qubit count."
            };

        if (!amplitudes.isNormalized())
            throw std::invalid_argument{
                "Quantum register amplitudes must be normalized."
            };
    }

    std::size_t QuantumRegister::qubitCount() const noexcept {
        return qubitCount_;
    }

    std::size_t QuantumRegister::stateCount() const noexcept {
        return amplitudes_.size();
    }

    const math::Complex &QuantumRegister::amplitude(std::size_t stateIndex) const {
        return amplitudes_.at(stateIndex);
    }

    QuantumRegister QuantumRegister::applySingleQubitGate(const math::ComplexMatrix &gate,
                                                          std::size_t targetQubit) const {
        if (gate.rows() != 2 || gate.columns() != 2) {
            throw std::invalid_argument{"A single-qubit gate must be a 2 by 2 matrix."};
        }

        if (!gate.isUnitary()) {
            throw std::invalid_argument{"A quantum gate must be unitary."};
        }

        if (targetQubit >= qubitCount_) {
            throw std::out_of_range{"Target qubit index is outside the register."};
        }

        const std::size_t targetMask =
                std::size_t{1} << (qubitCount_ - 1U - targetQubit);

        std::vector<math::Complex> transformed(stateCount(), math::Complex{});

        // Each zero-bit index owns one independent |0>, |1> amplitude pair.
        // Applying the 2x2 matrix to those pairs avoids a 2^n by 2^n expansion.
        for (std::size_t zeroState{}; zeroState < stateCount(); ++zeroState) {
            if ((zeroState & targetMask) != 0U) {
                continue;
            }

            const std::size_t oneState =
                    zeroState | targetMask;

            const math::Complex &zeroAmplitude =
                    amplitudes_.at(zeroState);

            const math::Complex &oneAmplitude =
                    amplitudes_.at(oneState);

            transformed[zeroState] =
                    gate.at(0U, 0U) * zeroAmplitude +
                    gate.at(0U, 1U) * oneAmplitude;

            transformed[oneState] =
                    gate.at(1U, 0U) * zeroAmplitude +
                    gate.at(1U, 1U) * oneAmplitude;
        }

        return QuantumRegister{
            qubitCount_,
            math::ComplexVector{std::move(transformed)}
        };
    }

    QuantumRegister QuantumRegister::applyTwoQubitGate(
        const math::ComplexMatrix &gate,
        const std::size_t firstQubit,
        const std::size_t secondQubit
    ) const {
        if (gate.rows() != 4U || gate.columns() != 4U) {
            throw std::invalid_argument{"A two-qubit gate must be a 4 by 4 matrix."};
        }

        if (!gate.isUnitary()) {
            throw std::invalid_argument{"A quantum gate must be unitary."};
        }

        if (firstQubit >= qubitCount_ || secondQubit >= qubitCount_) {
            throw std::out_of_range{"Two-qubit gate index is outside the register."};
        }

        if (firstQubit == secondQubit) {
            throw std::invalid_argument{"A two-qubit gate requires two different qubits."};
        }

        const std::size_t firstMask =
                std::size_t{1} << (qubitCount_ - 1U - firstQubit);

        const std::size_t secondMask =
                std::size_t{1} << (qubitCount_ - 1U - secondQubit);

        std::vector<math::Complex> transformed(stateCount(), math::Complex{});

        // A base state with both selected bits clear identifies one independent
        // four-amplitude block in local |00>, |01>, |10>, |11> order.
        for (std::size_t baseState{}; baseState < stateCount(); ++baseState) {
            if ((baseState & firstMask) != 0U ||
                (baseState & secondMask) != 0U) {
                continue;
            }

            const std::size_t stateIndices[4]{
                baseState,
                baseState | secondMask,
                baseState | firstMask,
                baseState | firstMask | secondMask
            };

            for (std::size_t output = 0U; output < 4U; ++output) {
                math::Complex sum{};

                for (std::size_t input = 0U; input < 4U; ++input) {
                    sum +=
                            gate.at(output, input) *
                            amplitudes_.at(stateIndices[input]);
                }

                transformed[stateIndices[output]] = sum;
            }
        }

        return QuantumRegister{
            qubitCount_,
            math::ComplexVector{std::move(transformed)}
        };
    }

    QuantumRegister QuantumRegister::applyGate(const math::ComplexMatrix &gate) const {
        if (gate.rows() != stateCount() ||
            gate.columns() != stateCount()) {
            throw std::invalid_argument{
                "Gate dimensions must match the quantum register state count."
            };
        }

        if (!gate.isUnitary()) {
            throw std::invalid_argument{
                "A quantum gate must be unitary."
            };
        }

        const math::ComplexVector transformed =
                gate * amplitudes_;

        return QuantumRegister{
            qubitCount_,
            transformed
        };
    }

    double QuantumRegister::probability(std::size_t stateIndex) const {
        return amplitude(stateIndex).magnitudeSquared();
    }

    std::size_t QuantumRegister::measure(std::mt19937 &randomEngine) {
        std::uniform_real_distribution distribution{0.0, 1.0};
        const double sample = distribution(randomEngine);

        double cumulativeProbability = 0.0;
        std::size_t measuredState = stateCount() - 1;

        // Walk the cumulative distribution and choose the first state past the sample.
        for (std::size_t state{}; state < stateCount(); ++state) {
            cumulativeProbability += probability(state);

            if (sample < cumulativeProbability) {
                measuredState = state;
                break;
            }
        }

        // Collapse the register so only the measured basis state remains.
        std::vector collapsedValues(
            stateCount(), math::Complex{});

        collapsedValues[measuredState] = math::Complex{1.0, 0.0};
        amplitudes_ = math::ComplexVector{std::move(collapsedValues)};

        return measuredState;
    }

    bool QuantumRegister::stateHasQubitOne(std::size_t stateIndex, std::size_t qubitIndex) const noexcept {
        // q0 maps to the most-significant bit of the state index.
        const std::size_t bitPosition = qubitCount_ - 1 - qubitIndex;
        const std::size_t mask =
                std::size_t{1} << bitPosition;

        return (stateIndex & mask) != 0;
    }

    double QuantumRegister::probabilityOfQubitOne(std::size_t qubitIndex) const {
        if (qubitIndex >= qubitCount_) {
            throw std::out_of_range{"Qubit index is outside the register."};
        }

        double totalProbability = 0.0;

        // Marginal probability is the sum of every basis state where this bit is 1.
        for (std::size_t state{}; state < stateCount(); ++state) {
            if (stateHasQubitOne(state, qubitIndex)) {
                totalProbability += probability(state);
            }
        }

        return totalProbability;
    }

    double QuantumRegister::probabilityOfQubitZero(std::size_t qubitIndex) const {
        return 1.0 - probabilityOfQubitOne(qubitIndex);
    }

    MeasurementResult QuantumRegister::measureQubit(std::size_t qubitIndex, std::mt19937 &randomEngine) {
        const double zeroProbability = probabilityOfQubitZero(qubitIndex);
        const double oneProbability = probabilityOfQubitOne(qubitIndex);


        std::uniform_real_distribution<double> distribution{0.0, 1.0};
        const double sample = distribution(randomEngine);

        const MeasurementResult result =
                sample < zeroProbability
                    ? MeasurementResult::Zero
                    : MeasurementResult::One;

        const bool measuredOne = result == MeasurementResult::One;

        const double resultProbability =
                measuredOne
                    ? oneProbability
                    : zeroProbability;

        const double normalizationFactor =
                std::sqrt(resultProbability);

        std::vector<math::Complex> collapsedValues;
        collapsedValues.reserve(stateCount());

        // Keep amplitudes consistent with the sampled qubit result and zero the rest.
        for (std::size_t state{}; state < stateCount(); ++state) {
            const bool stateQubitIsOne =
                    stateHasQubitOne(state, qubitIndex);

            if (stateQubitIsOne == measuredOne) {
                collapsedValues.push_back(amplitude(state) / normalizationFactor);
            } else {
                collapsedValues.push_back(math::Complex{});
            }
        }

        amplitudes_ = math::ComplexVector{
            std::move(collapsedValues)
        };

        return result;
    }

    QuantumRegister QuantumRegister::basisState(std::size_t qubitCount, std::size_t stateIndex) {
        if (qubitCount == 0) {
            throw std::invalid_argument{"A quantum register must contain at least one qubit."};
        }
        if (qubitCount >= std::numeric_limits<std::size_t>::digits) {
            throw std::invalid_argument{"Qubit count is too large to represent its state count."};
        }

        const std::size_t stateCount =
                std::size_t{1} << qubitCount;
        if (stateIndex >= stateCount) {
            throw std::out_of_range{"Basis state index is outside the register."};
        }

        std::vector values(stateCount, math::Complex{});
        values[stateIndex] = math::Complex{1.0, 0.0};

        return QuantumRegister{qubitCount, math::ComplexVector{std::move(values)}};
    }

    std::string QuantumRegister::basisStateLabel(std::size_t stateIndex) const {
        if (stateIndex >= stateCount()) {
            throw std::out_of_range{"Basis state index is outside the register"};
        }

        std::string label{"|"};

        // Convert the numeric state index back into a q0..qn bit string.
        for (std::size_t qubit = 0; qubit < qubitCount_; ++qubit) {
            const bool isOne = stateHasQubitOne(stateIndex, qubit);

            label += isOne ? '1' : '0';
        }
        label += ">";

        return label;
    }

    StateInfo QuantumRegister::stateInfo(std::size_t stateIndex) const {
        return StateInfo{
            basisStateLabel(stateIndex),
            amplitude(stateIndex),
            probability(stateIndex)
        };
    }

    std::vector<StateInfo> QuantumRegister::states() const {
        std::vector<StateInfo> result;
        result.reserve(stateCount());

        for (std::size_t stateIndex{}; stateIndex < stateCount(); ++stateIndex) {
            result.push_back(stateInfo(stateIndex));
        }

        return result;
    }

    BlochVector QuantumRegister::blockVector() const {
        if (qubitCount_ != 1) {
            throw std::invalid_argument{"A Bloch vector can only represent a single qubit."};
        }

        const math::Complex alpha = amplitude(0);
        const math::Complex beta = amplitude(1);

        // Standard Bloch coordinates for alpha|0> + beta|1>.
        const double x = 2.0 * (alpha.real() * beta.real() + alpha.imaginary() * beta.imaginary());
        const double y = 2.0 * (alpha.real() * beta.imaginary() - alpha.imaginary() * beta.real());
        const double z = alpha.magnitudeSquared() - beta.magnitudeSquared();

        return BlochVector{x, y, z};
    }

    BlochAngles QuantumRegister::blochAngles() const {
        const BlochVector vector = blockVector();

        const double clampedZ =
                std::clamp(vector.z, -1.0, 1.0);

        const double theta =
                std::acos(clampedZ);

        const double phi =
                std::atan2(vector.y, vector.x);

        return BlochAngles{theta, phi};
    }
}

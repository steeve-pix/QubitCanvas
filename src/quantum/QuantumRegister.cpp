#include "quantum_sim/quantum/QuantumRegister.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>
#include <utility>

#include "quantum_sim/gates/QuantumGates.hpp"

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

        math::ComplexMatrix combinedGate =
                math::ComplexMatrix::identity(1);

        for (std::size_t qubit{}; qubit < qubitCount_; ++qubit) {
            math::ComplexMatrix operationForThisQubit =
                    math::ComplexMatrix::identity(2);

            if (qubit == targetQubit) {
                operationForThisQubit = gate;
            }
            combinedGate = combinedGate.tensorProduct(operationForThisQubit);
        }

        const math::ComplexVector transformedAmplitudes =
                combinedGate * amplitudes_;

        return QuantumRegister{qubitCount_, transformedAmplitudes};
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

        for (std::size_t state{}; state < stateCount(); ++state) {
            cumulativeProbability += probability(state);

            if (sample < cumulativeProbability) {
                measuredState = state;
                break;
            }
        }
        std::vector collapsedValues(
            stateCount(), math::Complex{});

        collapsedValues[measuredState] = math::Complex{1.0, 0.0};
        amplitudes_ = math::ComplexVector{std::move(collapsedValues)};

        return measuredState;
    }

    bool QuantumRegister::stateHasQubitOne(std::size_t stateIndex, std::size_t qubitIndex) const noexcept {
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
        for (std::size_t qubit = 0; qubit < qubitCount_; ++qubit) {
            const bool isOne = stateHasQubitOne(stateIndex, qubit);

            label += isOne ? '1' : '0';
        }
        label += ">";

        return label;
    }
}

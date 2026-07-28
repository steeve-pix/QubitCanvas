#include "quantum_sim/gui/rendering/DensityVolumeModel.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace quantum_sim::gui::density_volume {
    namespace {
        constexpr std::uint64_t fnvOffset = 1469598103934665603ULL;
        constexpr std::uint64_t fnvPrime = 1099511628211ULL;

        void hashValue(std::uint64_t &hash, const std::uint64_t value) noexcept {
            hash ^= value;
            hash *= fnvPrime;
        }

        [[nodiscard]] std::size_t bitReversedDisplayIndex(
            std::size_t displayIndex,
            const std::size_t dimension
        ) noexcept {
            if (!std::has_single_bit(dimension)) {
                return displayIndex;
            }

            const unsigned int bitCount =
                    std::bit_width(dimension) - 1U;

            std::size_t basisIndex{};

            for (unsigned int bit = 0U; bit < bitCount; ++bit) {
                basisIndex =
                        (basisIndex << 1U) |
                        (displayIndex & 1U);

                displayIndex >>= 1U;
            }

            return basisIndex;
        }

        [[nodiscard]] DensityBin buildBin(
            const quantum::QuantumRegister &state,
            const std::size_t binIndex,
            const std::size_t binCount
        ) {
            const std::size_t stateCount =
                    state.stateCount();

            const std::size_t firstState =
                    std::min(
                        stateCount - 1U,
                        binIndex * stateCount / binCount
                    );

            const std::size_t lastState =
                    std::min(
                        stateCount,
                        std::max<std::size_t>(
                            firstState + 1U,
                            ((binIndex + 1U) * stateCount + binCount - 1U) /
                            binCount
                        )
                    );

            DensityBin bin{
                .firstState = firstState,
                .lastState = lastState,
                .strongestState = firstState
            };

            double strongestProbability =
                    -1.0;

            for (std::size_t stateIndex = firstState; stateIndex < lastState; ++stateIndex) {
                const math::Complex &amplitude =
                        state.amplitude(stateIndex);

                const double probability =
                        amplitude.magnitudeSquared();

                bin.probability += probability;
                bin.aggregateReal += amplitude.real();
                bin.aggregateImaginary += amplitude.imaginary();

                if (probability > strongestProbability) {
                    strongestProbability = probability;
                    bin.strongestState = stateIndex;
                    bin.strongestPhase = std::atan2(
                        amplitude.imaginary(),
                        amplitude.real()
                    );
                }
            }

            const std::string firstLabel =
                    state.basisStateLabel(firstState);

            if (lastState - firstState == 1U) {
                bin.label = firstLabel;
            } else {
                bin.label =
                        firstLabel +
                        ".." +
                        state.basisStateLabel(lastState - 1U);
            }

            return bin;
        }

        [[nodiscard]] DensityLayer buildLayer(
            const quantum::QuantumRegister &state,
            const std::size_t layerIndex,
            std::string label,
            const std::size_t maximumDimension
        ) {
            const std::size_t stateCount =
                    state.stateCount();

            const std::size_t dimension =
                    std::clamp<std::size_t>(
                        stateCount,
                        2U,
                        maximumDimension
                    );

            DensityLayer layer{
                .index = layerIndex,
                .dimension = dimension,
                .sourceStateCount = stateCount,
                .bucketed = dimension < stateCount,
                .label = std::move(label)
            };

            layer.bins.reserve(dimension);

            for (
                std::size_t displayBinIndex = 0U;
                displayBinIndex < dimension;
                ++displayBinIndex
            ) {
                // The simulator remains q0-most-significant. Reordering only
                // the display axes makes q0 the fastest-changing visual axis,
                // matching the reference density-matrix convention.
                const std::size_t basisBinIndex =
                        bitReversedDisplayIndex(
                            displayBinIndex,
                            dimension
                        );

                layer.bins.push_back(
                    buildBin(
                        state,
                        basisBinIndex,
                        dimension
                    )
                );
            }

            layer.cells.reserve(dimension * dimension);

            for (std::size_t row = 0; row < dimension; ++row) {
                const DensityBin &rowBin =
                        layer.bins[row];

                for (std::size_t column = 0; column < dimension; ++column) {
                    const DensityBin &columnBin =
                            layer.bins[column];

                    // rho[row,column] = a[row] * conjugate(a[column]).
                    const double real =
                            rowBin.aggregateReal * columnBin.aggregateReal +
                            rowBin.aggregateImaginary * columnBin.aggregateImaginary;

                    const double imaginary =
                            rowBin.aggregateImaginary * columnBin.aggregateReal -
                            rowBin.aggregateReal * columnBin.aggregateImaginary;

                    const double coherentMagnitude =
                            std::hypot(real, imaginary);

                    const double magnitude =
                            std::sqrt(
                                std::max(
                                    0.0,
                                    rowBin.probability * columnBin.probability
                                )
                            );

                    const double phase =
                            coherentMagnitude > 1e-12
                                ? std::atan2(imaginary, real)
                                : rowBin.strongestPhase - columnBin.strongestPhase;

                    layer.cells.push_back(
                        DensityCell{
                            .row = row,
                            .column = column,
                            .magnitude = magnitude,
                            .intensity = magnitude * magnitude,
                            .phaseRadians = phase,
                            .real = real,
                            .imaginary = imaginary
                        }
                    );
                }
            }

            return layer;
        }

        void finalizeStackMetadata(
            DensityStack &stack
        ) noexcept {
            stack.maximumMagnitude = 0.0;
            stack.fingerprint = fnvOffset;

            for (const DensityLayer &layer : stack.layers) {
                hashValue(stack.fingerprint, layer.index);
                hashValue(stack.fingerprint, layer.dimension);

                for (const DensityCell &cell : layer.cells) {
                    stack.maximumMagnitude =
                            std::max(
                                stack.maximumMagnitude,
                                cell.magnitude
                            );

                    hashValue(
                        stack.fingerprint,
                        std::bit_cast<std::uint64_t>(cell.real)
                    );

                    hashValue(
                        stack.fingerprint,
                        std::bit_cast<std::uint64_t>(cell.imaginary)
                    );
                }
            }
        }
    }

    const DensityCell &DensityLayer::cellAt(
        const std::size_t row,
        const std::size_t column
    ) const {
        if (row >= dimension || column >= dimension) {
            throw std::out_of_range{"Density Volume density cell index is outside the layer."};
        }

        return cells.at(row * dimension + column);
    }

    DensityStack DensityModel::build(
        const debug::DebuggerSession &session,
        const std::size_t maximumDimension,
        const util::StopToken stopToken
    ) {
        if (maximumDimension < 2U) {
            throw std::invalid_argument{"Density Volume density dimension must be at least two."};
        }

        DensityStack stack;
        stack.fingerprint = fnvOffset;
        stack.layers.reserve(session.stepCount() + 1U);

        stack.layers.push_back(
            buildLayer(
                session.initialState(),
                0U,
                "Initial state",
                maximumDimension
            )
        );

        for (std::size_t stepIndex = 0; stepIndex < session.stepCount(); ++stepIndex) {
            if (stopToken.stop_requested()) {
                throw circuit::TraceBuildCancelled{};
            }

            const circuit::TraceStep &step =
                    session.stepAt(stepIndex);

            stack.layers.push_back(
                buildLayer(
                    step.state,
                    stepIndex + 1U,
                    step.description,
                    maximumDimension
                )
            );
        }

        finalizeStackMetadata(stack);

        return stack;
    }

    void DensityModel::rebuildFrom(
        DensityStack &stack,
        const debug::DebuggerSession &session,
        const std::size_t firstChangedInstruction,
        const std::size_t maximumDimension,
        const util::StopToken stopToken
    ) {
        const std::size_t preservedLayerCount =
                firstChangedInstruction + 1U;

        if (
            maximumDimension < 2U ||
            firstChangedInstruction > session.stepCount() ||
            stack.layers.size() < preservedLayerCount
        ) {
            stack =
                    build(
                        session,
                        maximumDimension,
                        stopToken
                    );
            return;
        }

        stack.layers.resize(
            preservedLayerCount
        );

        stack.layers.reserve(
            session.stepCount() + 1U
        );

        for (
            std::size_t stepIndex =
                    firstChangedInstruction;
            stepIndex < session.stepCount();
            ++stepIndex
        ) {
            if (stopToken.stop_requested()) {
                throw circuit::TraceBuildCancelled{};
            }

            const circuit::TraceStep &step =
                    session.stepAt(stepIndex);

            stack.layers.push_back(
                buildLayer(
                    step.state,
                    stepIndex + 1U,
                    step.description,
                    maximumDimension
                )
            );
        }

        finalizeStackMetadata(stack);
    }

    DensityLayer DensityModel::difference(
        const DensityLayer &selected,
        const DensityLayer &reference
    ) {
        if (
            selected.dimension != reference.dimension ||
            selected.cells.size() != reference.cells.size()
        ) {
            throw std::invalid_argument{
                "Density layers must have matching dimensions for comparison."
            };
        }

        DensityLayer differenceLayer{
            .index = selected.index,
            .dimension = selected.dimension,
            .sourceStateCount = selected.sourceStateCount,
            .bucketed = selected.bucketed || reference.bucketed,
            .label =
                "Difference from layer " +
                std::to_string(reference.index),
            .bins = selected.bins
        };

        differenceLayer.cells.reserve(
            selected.cells.size()
        );

        for (
            std::size_t cellIndex = 0U;
            cellIndex < selected.cells.size();
            ++cellIndex
        ) {
            const DensityCell &selectedCell =
                    selected.cells[cellIndex];

            const DensityCell &referenceCell =
                    reference.cells[cellIndex];

            const double real =
                    selectedCell.real -
                    referenceCell.real;

            const double imaginary =
                    selectedCell.imaginary -
                    referenceCell.imaginary;

            const double magnitude =
                    std::hypot(real, imaginary);

            differenceLayer.cells.push_back(
                DensityCell{
                    .row = selectedCell.row,
                    .column = selectedCell.column,
                    .magnitude = magnitude,
                    .intensity = magnitude * magnitude,
                    .phaseRadians =
                        magnitude > 1e-12
                            ? std::atan2(imaginary, real)
                            : 0.0,
                    .real = real,
                    .imaginary = imaginary
                }
            );
        }

        return differenceLayer;
    }
}

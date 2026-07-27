#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/gates/QuantumGates.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/math/ComplexVector.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/quantum/Qubit.hpp"
#include "quantum_sim/visualization/ConsoleVisualizer.hpp"
#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"
#include "quantum_sim/gui/QuantumNotation.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeColorMap.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeModel.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeScene.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeCameraController.hpp"

#include <algorithm>
#include <sstream>
#include <iostream>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

#include "quantum_sim/debug/DebuggerSession.hpp"

namespace {
    using quantum_sim::math::ComplexVector;
    using quantum_sim::math::ComplexMatrix;
    using quantum_sim::math::Complex;
    using quantum_sim::quantum::Qubit;
    using quantum_sim::quantum::MeasurementResult;
    using quantum_sim::quantum::QuantumRegister;
    using quantum_sim::circuit::QuantumCircuit;

    int failures = 0;

    [[nodiscard]] bool approximatelyEqual(
        double left,
        double right,
        double epsilon = 1e-9) noexcept {
        return std::abs(left - right) <= epsilon;
    }

    void check(bool condition, const char *testName) {
        if (!condition) {
            std::cerr << "FAILED: " << testName << '\n';
            ++failures;
        }
    }
} //
int main() {
    // Regression coverage for debugger snapshots on a non-trivial entangling circuit.
    const QuantumCircuit circuit =
            quantum_sim::algorithms::ghzStateCircuit();

    const QuantumRegister initialState =
            QuantumRegister::basisState(3, 0);

    quantum_sim::debug::DebuggerSession session{
        circuit,
        initialState
    };

    session.restart();

    // Restart exposes step zero without executing the first circuit instruction.
    const quantum_sim::debug::DebuggerSnapshot snapshot =
            session.snapshot();

    check(
        snapshot.currentStepNumber == 0U &&
        !snapshot.instruction.has_value() &&
        !snapshot.canMovePrevious &&
        snapshot.canMoveNext &&
        approximatelyEqual(
            snapshot.afterState.get().probability(0),
            1.0
        ) &&
        approximatelyEqual(
            snapshot.beforeState.get().probability(0),
            1.0
        ),
        "Debugger restart exposes the untouched initial register as step zero"
    );

    session.moveNext();

    const quantum_sim::debug::DebuggerSnapshot firstInstructionSnapshot =
            session.snapshot();

    check(
        firstInstructionSnapshot.currentStepNumber == 1U &&
        firstInstructionSnapshot.instruction.has_value() &&
        approximatelyEqual(
            firstInstructionSnapshot.beforeState.get().probability(0),
            1.0
        ),
        "Debugger advances from initial step zero to the first instruction"
    );

    // The GUI default relies on this script to provide a rich density-history stack.
    const QuantumCircuit qftCircuit =
            quantum_sim::algorithms::qftCircuit(4);

    check(
        qftCircuit.qubitCount() == 4,
        "QFT showcase uses the requested qubit count"
    );

    check(
        qftCircuit.instructionCount() == 44,
        "QFT showcase keeps a 44-step render history"
    );

    const QuantumRegister qftFinalState =
            qftCircuit.execute(
                QuantumRegister::basisState(4, 0)
            );

    double totalProbability =
            0.0;

    for (std::size_t stateIndex = 0; stateIndex < qftFinalState.stateCount(); ++stateIndex) {
        totalProbability +=
                qftFinalState.probability(stateIndex);
    }

    check(
        approximatelyEqual(totalProbability, 1.0),
        "QFT showcase preserves total probability"
    );

    const QuantumRegister directSingleQubitResult =
            QuantumRegister::basisState(10U, 0U).applySingleQubitGate(
                quantum_sim::gates::xGate(),
                9U
            );

    check(
        approximatelyEqual(directSingleQubitResult.probability(1U), 1.0),
        "Direct single-qubit execution respects q0-as-most-significant ordering"
    );

    const QuantumRegister reversedControlResult =
            QuantumRegister::basisState(3U, 1U).applyTwoQubitGate(
                quantum_sim::gates::cxGate(),
                2U,
                0U
            );

    check(
        approximatelyEqual(reversedControlResult.probability(5U), 1.0),
        "Compact two-qubit execution supports reversed and non-adjacent operands"
    );

    QuantumCircuit insertedTwoQubitCircuit{10U};
    insertedTwoQubitCircuit.insertTwoQubitGate(
        12U,
        "CX",
        quantum_sim::gates::cxGate(),
        0U,
        9U
    );

    const QuantumRegister insertedTwoQubitResult =
            insertedTwoQubitCircuit.execute(
                QuantumRegister::basisState(10U, 512U)
            );

    check(
        insertedTwoQubitCircuit.instructionCount() == 1U &&
        insertedTwoQubitCircuit.instructionInfo().front().kind ==
            quantum_sim::circuit::CircuitInstructionKind::TwoQubit &&
        approximatelyEqual(insertedTwoQubitResult.probability(513U), 1.0),
        "Manual ten-qubit placement inserts and executes a compact two-qubit gate"
    );

    const QuantumCircuit tenQubitQft =
            quantum_sim::algorithms::qftCircuit(10U);

    const auto tenQubitQftInstructions =
            tenQubitQft.instructionInfo();

    const std::size_t compactTwoQubitInstructionCount =
            static_cast<std::size_t>(
                std::count_if(
                    tenQubitQftInstructions.begin(),
                    tenQubitQftInstructions.end(),
                    [](const quantum_sim::circuit::CircuitInstructionInfo &instruction) {
                        return instruction.kind ==
                               quantum_sim::circuit::CircuitInstructionKind::TwoQubit;
                    }
                )
            );

    const bool tenQubitQftContainsFullRegisterGate =
            std::any_of(
                tenQubitQftInstructions.begin(),
                tenQubitQftInstructions.end(),
                [](const quantum_sim::circuit::CircuitInstructionInfo &instruction) {
                    return instruction.kind ==
                           quantum_sim::circuit::CircuitInstructionKind::FullRegister;
                }
            );

    check(
        tenQubitQft.instructionCount() == 260U &&
        compactTwoQubitInstructionCount == 95U &&
        !tenQubitQftContainsFullRegisterGate,
        "Ten-qubit QFT stores all 95 two-qubit operations as compact 4x4 gates"
    );

    quantum_sim::debug::DebuggerSession tenQubitSession{
        tenQubitQft,
        QuantumRegister::basisState(10U, 0U)
    };

    check(
        tenQubitSession.stepCount() == tenQubitQft.instructionCount() &&
        tenQubitSession.stepAt(0U).state.stateCount() == 1024U,
        "Ten-qubit QFT builds a complete debugger trace without register-sized gates"
    );

    const quantum_sim::gui::density_volume::DensityStack largeDensityStack =
            quantum_sim::gui::density_volume::DensityModel::build(
                tenQubitSession,
                16U
            );

    check(
        largeDensityStack.layers.size() == tenQubitSession.stepCount() + 1U &&
        largeDensityStack.layers.back().dimension == 16U &&
        largeDensityStack.layers.back().bucketed,
        "Ten-qubit debugger history remains bounded to a 16 by 16 density view"
    );

    const std::vector<QuantumCircuit> tenQubitPresets{
        quantum_sim::algorithms::bellStateCircuit(10U),
        quantum_sim::algorithms::ghzStateCircuit(10U),
        quantum_sim::algorithms::equalSuperpositionCircuit(10U),
        quantum_sim::algorithms::inverseQftCircuit(10U),
        quantum_sim::algorithms::groverSearchCircuit(10U),
        quantum_sim::algorithms::deutschJozsaCircuit(10U),
        quantum_sim::algorithms::bernsteinVaziraniCircuit(9U, 0b101010101U),
        quantum_sim::algorithms::toffoliDemoCircuit(10U),
        quantum_sim::algorithms::phaseKickbackCircuit(10U),
        quantum_sim::algorithms::teleportationCircuit(10U),
        quantum_sim::algorithms::scrambleCircuit(10U)
    };

    check(
        std::all_of(
            tenQubitPresets.begin(),
            tenQubitPresets.end(),
            [](const QuantumCircuit &preset) {
                const auto instructions =
                        preset.instructionInfo();

                return preset.qubitCount() == 10U &&
                       std::none_of(
                           instructions.begin(),
                           instructions.end(),
                           [](const quantum_sim::circuit::CircuitInstructionInfo &instruction) {
                               return instruction.kind ==
                                      quantum_sim::circuit::CircuitInstructionKind::FullRegister;
                           }
                       );
            }
        ),
        "Every ten-qubit preset uses bounded-memory local instructions"
    );

    check(
        quantum_sim::algorithms::bellStateCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::ghzStateCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::equalSuperpositionCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::qftCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::inverseQftCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::groverSearchCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::deutschJozsaCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::bernsteinVaziraniCircuit(3, 0b101).qubitCount() == 4U &&
        quantum_sim::algorithms::toffoliDemoCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::phaseKickbackCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::teleportationCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::scrambleCircuit(4).qubitCount() == 4U,
        "Every built-in algorithm honors the selected register size"
    );

    const QuantumRegister expandedGhzResult =
            quantum_sim::algorithms::ghzStateCircuit(4).execute(
                QuantumRegister::basisState(4, 0)
            );

    check(
        approximatelyEqual(expandedGhzResult.probability(0), 0.5) &&
        approximatelyEqual(expandedGhzResult.probability(15), 0.5),
        "Expanded GHZ entangles the complete selected register"
    );

    bool rejectedUndersizedBell = false;

    try {
        static_cast<void>(
            quantum_sim::algorithms::bellStateCircuit(1)
        );
    } catch (const std::invalid_argument &) {
        rejectedUndersizedBell = true;
    }

    check(
        rejectedUndersizedBell,
        "Bell state rejects a register below its two-qubit minimum"
    );

    QuantumCircuit densityCircuit{1};
    densityCircuit.addSingleQubitGate(
        "H",
        quantum_sim::gates::hadamardGate(),
        0
    );
    densityCircuit.addSingleQubitGate(
        "S",
        quantum_sim::gates::sGate(),
        0
    );

    quantum_sim::debug::DebuggerSession densitySession{
        densityCircuit,
        QuantumRegister::basisState(1, 0)
    };

    const quantum_sim::gui::density_volume::DensityStack densityStack =
            quantum_sim::gui::density_volume::DensityModel::build(
                densitySession,
                16U
            );

    check(
        densityStack.layers.size() == densitySession.stepCount() + 1U,
        "Density Volume density history includes the initial state and every circuit step"
    );

    check(
        densityStack.layers.front().dimension == 2U &&
        approximatelyEqual(
            densityStack.layers.front().cellAt(0, 0).magnitude,
            1.0
        ) &&
        approximatelyEqual(
            densityStack.layers.front().cellAt(0, 1).magnitude,
            0.0
        ),
        "Density Volume initial density layer preserves the exact basis-state matrix"
    );

    const auto &hadamardCoherence =
            densityStack.layers.at(1).cellAt(0, 1);

    check(
        approximatelyEqual(hadamardCoherence.magnitude, 0.5) &&
        approximatelyEqual(hadamardCoherence.real, 0.5) &&
        approximatelyEqual(hadamardCoherence.imaginary, 0.0),
        "Density Volume density cells preserve Hadamard coherence"
    );

    const auto &phaseCoherence =
            densityStack.layers.at(2).cellAt(0, 1);

    check(
        approximatelyEqual(phaseCoherence.magnitude, 0.5) &&
        approximatelyEqual(
            phaseCoherence.phaseRadians,
            -std::numbers::pi / 2.0
        ) &&
        approximatelyEqual(phaseCoherence.imaginary, -0.5),
        "Density Volume density cells preserve phase in radians and complex components"
    );

    QuantumCircuit twoQubitHadamardCircuit{2U};
    twoQubitHadamardCircuit.addSingleQubitGate(
        "H",
        quantum_sim::gates::hadamardGate(),
        0U
    );

    const quantum_sim::debug::DebuggerSession twoQubitHadamardSession{
        twoQubitHadamardCircuit,
        QuantumRegister::basisState(2U, 0U)
    };

    const auto twoQubitHadamardStack =
            quantum_sim::gui::density_volume::DensityModel::build(
                twoQubitHadamardSession,
                16U
            );

    const auto &twoQubitHadamardLayer =
            twoQubitHadamardStack.layers.at(1U);

    check(
        twoQubitHadamardLayer.bins.at(0U).label == "|00\xE2\x9F\xA9" &&
        twoQubitHadamardLayer.bins.at(1U).label == "|10\xE2\x9F\xA9" &&
        twoQubitHadamardLayer.bins.at(2U).label == "|01\xE2\x9F\xA9" &&
        twoQubitHadamardLayer.bins.at(3U).label == "|11\xE2\x9F\xA9",
        "Density Volume display axes use bit-reversed two-qubit basis order"
    );

    check(
        approximatelyEqual(
            twoQubitHadamardLayer.cellAt(0U, 0U).magnitude,
            0.5
        ) &&
        approximatelyEqual(
            twoQubitHadamardLayer.cellAt(0U, 1U).magnitude,
            0.5
        ) &&
        approximatelyEqual(
            twoQubitHadamardLayer.cellAt(1U, 0U).magnitude,
            0.5
        ) &&
        approximatelyEqual(
            twoQubitHadamardLayer.cellAt(1U, 1U).magnitude,
            0.5
        ) &&
        approximatelyEqual(
            twoQubitHadamardLayer.cellAt(0U, 2U).magnitude,
            0.0
        ),
        "Density Volume renders H(q0) as the reference top-left 2x2 block"
    );

    const auto weakInfernoColor =
            quantum_sim::gui::density_volume::magnitudeColor(
                0.0625
            );

    const auto mediumInfernoColor =
            quantum_sim::gui::density_volume::magnitudeColor(
                0.25
            );

    const auto peakInfernoColor =
            quantum_sim::gui::density_volume::magnitudeColor(
                1.0
            );

    check(
        weakInfernoColor.blue > weakInfernoColor.red &&
        weakInfernoColor.red > weakInfernoColor.green &&
        mediumInfernoColor.red > mediumInfernoColor.blue &&
        mediumInfernoColor.blue > mediumInfernoColor.green &&
        peakInfernoColor.red > 0.95F &&
        peakInfernoColor.green > 0.90F &&
        peakInfernoColor.blue > 0.75F,
        "Density Volume Inferno response preserves violet, crimson, and warm peak bands"
    );

    const quantum_sim::gui::density_volume::InstanceScene floorFieldScene =
            quantum_sim::gui::density_volume::SceneBuilder::build(
                densityStack,
                1U,
                quantum_sim::gui::density_volume::VisualizationMode::FloorField
            );

    check(
        floorFieldScene.voxels.size() == 4U &&
        floorFieldScene.pickRecords.size() == 4U &&
        std::all_of(
            floorFieldScene.pickRecords.begin(),
            floorFieldScene.pickRecords.end(),
            [](const quantum_sim::gui::density_volume::Selection &selection) {
                return selection.layer == 1U;
            }
        ),
        "Density Volume floor field creates one pickable instance per selected matrix cell"
    );

    const quantum_sim::gui::density_volume::InstanceScene historyScene =
            quantum_sim::gui::density_volume::SceneBuilder::build(
                densityStack,
                1U,
                quantum_sim::gui::density_volume::VisualizationMode::LayerStack
            );

    const auto findInstance =
            [](
                const quantum_sim::gui::density_volume::InstanceScene &scene,
                const std::vector<
                    quantum_sim::gui::density_volume::VoxelInstance
                > &instances,
                const std::size_t layer,
                const std::size_t row,
                const std::size_t column
            ) -> const quantum_sim::gui::density_volume::VoxelInstance * {
                for (
                    const auto &instance : instances
                ) {
                    const std::size_t pickIndex =
                            static_cast<std::size_t>(
                                instance.pickId
                            ) -
                            1U;

                    const auto &selection =
                            scene.pickRecords.at(pickIndex);

                    if (
                        selection.layer == layer &&
                        selection.row == row &&
                        selection.column == column
                    ) {
                        return &instance;
                    }
                }

                return nullptr;
            };

    const auto *initialPeak =
            findInstance(
                historyScene,
                historyScene.voxels,
                0U,
                0U,
                0U
            );

    const auto *floorPeak =
            findInstance(
                floorFieldScene,
                floorFieldScene.voxels,
                1U,
                0U,
                0U
            );

    check(
        floorPeak != nullptr &&
        approximatelyEqual(floorPeak->magnitude, 0.52, 1.0e-5) &&
        approximatelyEqual(floorPeak->size.y, 4.4, 1.0e-5),
        "Density Volume floor field normalizes height without clipping its Inferno colors"
    );

    const auto *hadamardPeak =
            findInstance(
                historyScene,
                historyScene.voxels,
                1U,
                0U,
                0U
            );

    const auto *dormantCell =
            findInstance(
                historyScene,
                historyScene.ghostVoxels,
                0U,
                1U,
                1U
            );

    const auto *nextLayerPeak =
            findInstance(
                historyScene,
                historyScene.voxels,
                1U,
                0U,
                0U
            );

    check(
        historyScene.voxels.size() == 9U &&
        historyScene.ghostVoxels.size() == 3U &&
        historyScene.pickRecords.size() ==
            historyScene.voxels.size() +
            historyScene.ghostVoxels.size() &&
        historyScene.layerEndInstanceCounts.size() ==
            densityStack.layers.size() &&
        historyScene.layerEndGhostCounts.size() ==
            densityStack.layers.size() &&
        historyScene.layerEndInstanceCounts.at(1U) == 5U &&
        historyScene.layerEndGhostCounts.at(1U) == 3U,
        "Density Volume history separates solid values from small-matrix edge ghosts"
    );

    check(
        initialPeak != nullptr &&
        hadamardPeak != nullptr &&
        dormantCell != nullptr &&
        nextLayerPeak != nullptr &&
        initialPeak->center.x < hadamardPeak->center.x &&
        approximatelyEqual(initialPeak->center.z, hadamardPeak->center.z) &&
        nextLayerPeak->center.x - initialPeak->center.x >
            (nextLayerPeak->size.x + initialPeak->size.x) * 0.5F &&
        initialPeak->emissive > dormantCell->emissive &&
        approximatelyEqual(
            initialPeak->size.x,
            initialPeak->size.y
        ) &&
        approximatelyEqual(
            initialPeak->size.y,
            initialPeak->size.z
        ) &&
        approximatelyEqual(
            initialPeak->size.x,
            nextLayerPeak->size.x
        ),
        "Density Volume layers advance on X with separated fixed-size cubes"
    );

    const quantum_sim::gui::density_volume::VoxelGeometry roundedCube =
            quantum_sim::gui::density_volume::VoxelGeometryBuilder::buildRoundedCube();

    check(
        roundedCube.vertices.size() == 150U &&
        roundedCube.indices.size() == 576U &&
        std::all_of(
            roundedCube.vertices.begin(),
            roundedCube.vertices.end(),
            [](const quantum_sim::gui::density_volume::VoxelVertex &vertex) {
                const float normalLength =
                        std::sqrt(
                            vertex.normal[0] * vertex.normal[0] +
                            vertex.normal[1] * vertex.normal[1] +
                            vertex.normal[2] * vertex.normal[2]
                        );

                return approximatelyEqual(
                    normalLength,
                    1.0,
                    0.0001
                );
            }
        ),
        "Density Volume reuses one indexed rounded cube with normalized lighting normals"
    );

    const quantum_sim::gui::density_volume::InstanceScene largeHistoryScene =
            quantum_sim::gui::density_volume::SceneBuilder::build(
                largeDensityStack,
                105U,
                quantum_sim::gui::density_volume::VisualizationMode::LayerStack
            );

    check(
        !largeHistoryScene.voxels.empty() &&
        largeHistoryScene.voxels.size() <=
            largeDensityStack.layers.size() * 256U &&
        largeHistoryScene.ghostVoxels.empty() &&
        largeHistoryScene.layerEndInstanceCounts.at(105U) <=
            106U * 256U &&
        roundedCube.vertices.size() < 200U,
        "Ten-qubit history omits zero geometry and reuses one shared cube"
    );

    const QuantumRegister qftSourceState =
            QuantumRegister::basisState(4, 9);

    const QuantumRegister qftTransformedState =
            qftCircuit.execute(qftSourceState);

    const QuantumCircuit inverseQftCircuit =
            quantum_sim::algorithms::inverseQftCircuit(4);

    const QuantumRegister qftRestoredState =
            inverseQftCircuit.execute(qftTransformedState);

    check(
        inverseQftCircuit.instructionCount() ==
        qftCircuit.instructionCount(),
        "Inverse QFT mirrors every showcase instruction"
    );

    check(
        approximatelyEqual(qftRestoredState.probability(9), 1.0),
        "Inverse QFT restores a state transformed by the QFT showcase"
    );

    const QuantumRegister groverResult =
            quantum_sim::algorithms::groverSearchCircuit().execute(
                QuantumRegister::basisState(2, 0)
            );

    check(
        approximatelyEqual(groverResult.probability(3), 1.0),
        "Grover search amplifies the marked state |11⟩"
    );

    const QuantumRegister expandedGroverResult =
            quantum_sim::algorithms::groverSearchCircuit(4).execute(
                QuantumRegister::basisState(4, 0)
            );

    check(
        approximatelyEqual(expandedGroverResult.probability(12), 1.0),
        "Expanded Grover preserves idle qubits while marking q0/q1 as |11⟩"
    );

    const QuantumRegister deutschJozsaResult =
            quantum_sim::algorithms::deutschJozsaCircuit().execute(
                QuantumRegister::basisState(3, 0)
            );

    check(
        approximatelyEqual(
            deutschJozsaResult.probabilityOfQubitOne(0),
            1.0
        ) &&
        approximatelyEqual(
            deutschJozsaResult.probabilityOfQubitOne(1),
            1.0
        ),
        "Deutsch-Jozsa identifies the balanced XOR oracle"
    );

    const QuantumRegister expandedDeutschJozsaResult =
            quantum_sim::algorithms::deutschJozsaCircuit(4).execute(
                QuantumRegister::basisState(4, 0)
            );

    check(
        approximatelyEqual(
            expandedDeutschJozsaResult.probabilityOfQubitOne(0),
            1.0
        ) &&
        approximatelyEqual(
            expandedDeutschJozsaResult.probabilityOfQubitOne(1),
            1.0
        ) &&
        approximatelyEqual(
            expandedDeutschJozsaResult.probabilityOfQubitOne(2),
            1.0
        ),
        "Expanded Deutsch-Jozsa uses every input qubit in its parity oracle"
    );

    const QuantumRegister bernsteinVaziraniResult =
            quantum_sim::algorithms::bernsteinVaziraniCircuit(
                3,
                0b101
            ).execute(
                QuantumRegister::basisState(4, 0)
            );

    check(
        approximatelyEqual(
            bernsteinVaziraniResult.probabilityOfQubitOne(0),
            1.0
        ) &&
        approximatelyEqual(
            bernsteinVaziraniResult.probabilityOfQubitZero(1),
            1.0
        ) &&
        approximatelyEqual(
            bernsteinVaziraniResult.probabilityOfQubitOne(2),
            1.0
        ),
        "Bernstein-Vazirani recovers the hidden string 101"
    );

    const QuantumRegister toffoliResult =
            quantum_sim::algorithms::toffoliDemoCircuit().execute(
                QuantumRegister::basisState(3, 0)
            );

    check(
        approximatelyEqual(toffoliResult.probability(7), 1.0),
        "Decomposed Toffoli flips the target when both controls are one"
    );

    const QuantumRegister kickbackResult =
            quantum_sim::algorithms::phaseKickbackCircuit().execute(
                QuantumRegister::basisState(2, 0)
            );

    check(
        approximatelyEqual(kickbackResult.probability(3), 1.0),
        "Phase kickback is exposed as the final basis state |11⟩"
    );

    const QuantumRegister teleportationResult =
            quantum_sim::algorithms::teleportationCircuit().execute(
                QuantumRegister::basisState(3, 0)
            );

    check(
        approximatelyEqual(
            teleportationResult.probabilityOfQubitOne(2),
            0.25
        ),
        "Coherent teleportation transfers the prepared state's magnitude to q2"
    );

    const double teleportedTheta =
            std::numbers::pi / 3.0;

    const double teleportedPhi =
            std::numbers::pi / 5.0;

    const double expectedTeleportedZeroReal =
            0.5 *
            std::cos(teleportedTheta * 0.5) *
            std::cos(teleportedPhi * 0.5);

    const double expectedTeleportedZeroImaginary =
            -0.5 *
            std::cos(teleportedTheta * 0.5) *
            std::sin(teleportedPhi * 0.5);

    const double expectedTeleportedOneReal =
            0.5 *
            std::sin(teleportedTheta * 0.5) *
            std::cos(teleportedPhi * 0.5);

    const double expectedTeleportedOneImaginary =
            0.5 *
            std::sin(teleportedTheta * 0.5) *
            std::sin(teleportedPhi * 0.5);

    check(
        approximatelyEqual(
            teleportationResult.amplitude(0).real(),
            expectedTeleportedZeroReal
        ) &&
        approximatelyEqual(
            teleportationResult.amplitude(0).imaginary(),
            expectedTeleportedZeroImaginary
        ) &&
        approximatelyEqual(
            teleportationResult.amplitude(1).real(),
            expectedTeleportedOneReal
        ) &&
        approximatelyEqual(
            teleportationResult.amplitude(1).imaginary(),
            expectedTeleportedOneImaginary
        ),
        "Coherent teleportation preserves the prepared state's relative phase"
    );

    const double halfTurn =
            std::numbers::pi;

    const ComplexMatrix rx =
            quantum_sim::gates::rxGate(halfTurn);

    const ComplexMatrix ry =
            quantum_sim::gates::ryGate(halfTurn);

    const ComplexMatrix rz =
            quantum_sim::gates::rzGate(halfTurn);

    const ComplexMatrix tDagger =
            quantum_sim::gates::tDaggerGate();

    const ComplexMatrix sDagger =
            quantum_sim::gates::sDaggerGate();

    check(
        approximatelyEqual(rx.at(0, 0).real(), 0.0) &&
        approximatelyEqual(rx.at(0, 1).imaginary(), -1.0) &&
        approximatelyEqual(rx.at(1, 0).imaginary(), -1.0),
        "Rx(pi) uses the expected half-angle matrix"
    );

    check(
        approximatelyEqual(ry.at(0, 1).real(), -1.0) &&
        approximatelyEqual(ry.at(1, 0).real(), 1.0) &&
        approximatelyEqual(ry.at(1, 1).real(), 0.0),
        "Ry(pi) uses the expected half-angle matrix"
    );

    check(
        approximatelyEqual(rz.at(0, 0).imaginary(), -1.0) &&
        approximatelyEqual(rz.at(1, 1).imaginary(), 1.0) &&
        approximatelyEqual(rz.at(0, 1).magnitude(), 0.0),
        "Rz(pi) applies opposite diagonal phases"
    );

    check(
        approximatelyEqual(sDagger.at(0, 0).real(), 1.0) &&
        approximatelyEqual(sDagger.at(1, 1).real(), 0.0) &&
        approximatelyEqual(sDagger.at(1, 1).imaginary(), -1.0),
        "S-dagger applies the inverse pi/2 phase"
    );

    check(
        quantum_sim::gui::notation::formatReal(
            std::numbers::sqrt2 / 2.0
        ) == "\xE2\x88\x9A""2/2" &&
        quantum_sim::gui::notation::formatReal(
            static_cast<double>(
                static_cast<float>(
                    std::numbers::sqrt2 / 2.0
                )
            )
        ) == "\xE2\x88\x9A""2/2" &&
        quantum_sim::gui::notation::formatReal(0.5) == "1/2" &&
        quantum_sim::gui::notation::formatReal(
            std::sqrt(2.0 + std::numbers::sqrt2) / 2.0
        ) == "\xE2\x88\x9A(2+\xE2\x88\x9A""2)/2",
        "Quantum notation recognizes fractions and common radical constants"
    );

    check(
        quantum_sim::gui::notation::formatComplex(
            quantum_sim::math::Complex{
                std::numbers::sqrt2 / 2.0,
                -std::numbers::sqrt2 / 2.0
            }
        ) == "\xE2\x88\x9A""2/2-\xE2\x88\x9A""2/2i" &&
        quantum_sim::gui::notation::formatRadians(
            -3.0 * std::numbers::pi / 4.0
        ) == "-3\xCF\x80/4" &&
        quantum_sim::gui::notation::formatRadians(
            static_cast<double>(
                static_cast<float>(
                    std::numbers::pi / 2.0
                )
            )
        ) == "\xCF\x80/2" &&
        quantum_sim::gui::notation::formatRadians(
            0.74 * std::numbers::pi
        ) == "0.74\xCF\x80" &&
        quantum_sim::gui::notation::formatRadians(
            std::numbers::pi / 2.0,
            3,
            true
        ) == "\xCF\x80/2 rad" &&
        quantum_sim::gui::notation::formatPolarAmplitude(
            std::numbers::sqrt2 / 2.0,
            -std::numbers::pi / 4.0
        ) == "\xE2\x88\x9A""2/2 e^(-i\xCF\x80/4)",
        "Quantum notation composes exact complex, angle, and polar forms"
    );

    check(
        quantum_sim::debug::gateExplanation("Sdg").find("-pi/2") !=
        std::string::npos,
        "S-dagger has a debugger and hover-card explanation"
    );

    check(
        approximatelyEqual(tDagger.at(0, 0).real(), 1.0) &&
        approximatelyEqual(
            tDagger.at(1, 1).real(),
            std::sqrt(0.5)
        ) &&
        approximatelyEqual(
            tDagger.at(1, 1).imaginary(),
            -std::sqrt(0.5)
        ),
        "T-dagger applies the inverse pi/4 phase"
    );

    check(
        quantum_sim::debug::gateExplanation("Tdg").find("-pi/4") !=
        std::string::npos,
        "T-dagger has a debugger and hover-card explanation"
    );

    const QuantumRegister rxResult =
            quantum_sim::algorithms::rxRotationCircuit(halfTurn).execute(
                QuantumRegister::basisState(1, 0)
            );

    const QuantumRegister ryResult =
            quantum_sim::algorithms::ryRotationCircuit(halfTurn).execute(
                QuantumRegister::basisState(1, 0)
            );

    check(
        approximatelyEqual(rxResult.probability(1), 1.0),
        "Rx(pi) rotates |0⟩ to |1⟩ up to phase"
    );

    check(
        approximatelyEqual(ryResult.probability(1), 1.0),
        "Ry(pi) rotates |0⟩ to |1⟩"
    );

    const double metadataAngle =
            std::numbers::pi / 3.0;

    const auto rotationInstructions =
            quantum_sim::algorithms::rzRotationCircuit(metadataAngle).instructionInfo();

    check(
        rotationInstructions.size() == 1U &&
        rotationInstructions.front().angleRadians.has_value() &&
        approximatelyEqual(
            rotationInstructions.front().angleRadians.value(),
            metadataAngle
        ),
        "Rotation circuits retain their angle in instruction metadata"
    );

    quantum_sim::gui::density_volume::CameraController densityCamera;
    densityCamera.frameScene(
        quantum_sim::gui::density_volume::Vector3{0.0F, 0.0F, 0.0F},
        4.0F
    );
    densityCamera.orbit(80.0F, -25.0F);
    densityCamera.pan(12.0F, -6.0F);
    densityCamera.zoom(1.0F);
    densityCamera.update(0.1F);

    const auto userCameraView =
            densityCamera.viewMatrix();

    densityCamera.updateSceneBounds(
        quantum_sim::gui::density_volume::Vector3{6.0F, 2.0F, -3.0F},
        12.0F
    );

    const auto playbackUpdatedView =
            densityCamera.viewMatrix();

    check(
        std::equal(
            userCameraView.begin(),
            userCameraView.end(),
            playbackUpdatedView.begin(),
            [](const float left, const float right) {
                return approximatelyEqual(left, right, 1e-6);
            }
        ),
        "Density scene updates preserve the user camera pose"
    );

    densityCamera.reset();

    check(
        !std::equal(
            userCameraView.begin(),
            userCameraView.end(),
            densityCamera.viewMatrix().begin(),
            [](const float left, const float right) {
                return approximatelyEqual(left, right, 1e-6);
            }
        ),
        "Density camera reset uses the latest scene bounds"
    );

    quantum_sim::gui::density_volume::CameraController automaticCamera;
    automaticCamera.frameScene(
        quantum_sim::gui::density_volume::Vector3{0.0F, 0.0F, 0.0F},
        4.0F
    );

    const auto initialAutomaticView =
            automaticCamera.viewMatrix();

    automaticCamera.updateSceneBounds(
        quantum_sim::gui::density_volume::Vector3{5.0F, 0.0F, 0.0F},
        9.0F
    );

    automaticCamera.update(0.1F);

    check(
        !std::equal(
            initialAutomaticView.begin(),
            initialAutomaticView.end(),
            automaticCamera.viewMatrix().begin(),
            [](const float left, const float right) {
                return approximatelyEqual(left, right, 1e-6);
            }
        ),
        "Untouched playback camera follows expanding scene bounds"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return failures == 0 ? 0 : 1;
}

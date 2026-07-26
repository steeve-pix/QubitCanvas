#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/gates/QuantumGates.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/math/ComplexVector.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/quantum/Qubit.hpp"
#include "quantum_sim/visualization/ConsoleVisualizer.hpp"
#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeModel.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeLayout.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeMeshBuilder.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeCameraController.hpp"

#include <algorithm>
#include <sstream>
#include <iostream>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string>

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

    // After restart, the "before" state of the first step should be the original |000>.
    const quantum_sim::debug::DebuggerSnapshot snapshot =
            session.snapshot();

    check(
        approximatelyEqual(
            snapshot.beforeState.get().probability(0),
            1.0
        ),
        "Debugger snapshot exposes the state before the current instruction"
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

    const quantum_sim::gui::density_volume::SceneLayout floorFieldLayout =
            quantum_sim::gui::density_volume::LayerStackLayout::build(
                densityStack,
                1U,
                quantum_sim::gui::density_volume::VisualizationMode::FloorField
            );

    check(
        floorFieldLayout.voxels.size() == 8U &&
        std::all_of(
            floorFieldLayout.voxels.begin(),
            floorFieldLayout.voxels.end(),
            [](const quantum_sim::gui::density_volume::PlacedVoxel &voxel) {
                return voxel.layer == 1U;
            }
        ),
        "Density Volume floor field contains only the complete selected density matrix"
    );

    const quantum_sim::gui::density_volume::SceneLayout historyLayout =
            quantum_sim::gui::density_volume::LayerStackLayout::build(
                densityStack,
                1U,
                quantum_sim::gui::density_volume::VisualizationMode::LayerStack
            );

    const auto initialPeak =
            std::find_if(
                historyLayout.voxels.begin(),
                historyLayout.voxels.end(),
                [](const quantum_sim::gui::density_volume::PlacedVoxel &voxel) {
                    return voxel.layer == 0U &&
                           voxel.row == 0U &&
                           voxel.column == 0U &&
                           voxel.magnitudeVoxel;
                }
            );

    const auto hadamardPeak =
            std::find_if(
                historyLayout.voxels.begin(),
                historyLayout.voxels.end(),
                [](const quantum_sim::gui::density_volume::PlacedVoxel &voxel) {
                    return voxel.layer == 1U &&
                           voxel.row == 0U &&
                           voxel.column == 0U &&
                           voxel.magnitudeVoxel;
                }
            );

    check(
        initialPeak != historyLayout.voxels.end() &&
        hadamardPeak != historyLayout.voxels.end() &&
        initialPeak->size.y > hadamardPeak->size.y,
        "Density Volume voxel height preserves absolute density magnitude across layers"
    );

    check(
        initialPeak != historyLayout.voxels.end() &&
        hadamardPeak != historyLayout.voxels.end() &&
        initialPeak->size.y >= initialPeak->size.x &&
        hadamardPeak->size.y >= hadamardPeak->size.x * 0.80F,
        "Visible density voxels retain a block-like silhouette"
    );

    const quantum_sim::gui::density_volume::Mesh beveledFloorMesh =
            quantum_sim::gui::density_volume::MeshBuilder::build(
                floorFieldLayout
            );

    check(
        beveledFloorMesh.vertices.size() ==
            floorFieldLayout.voxels.size() * 24U &&
        beveledFloorMesh.indices.size() ==
            floorFieldLayout.voxels.size() * 132U &&
        beveledFloorMesh.pickRecords.size() ==
            floorFieldLayout.voxels.size(),
        "Density Volume uses indexed bevel faces while preserving voxel picking"
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
        "Grover search amplifies the marked state |11>"
    );

    const QuantumRegister expandedGroverResult =
            quantum_sim::algorithms::groverSearchCircuit(4).execute(
                QuantumRegister::basisState(4, 0)
            );

    check(
        approximatelyEqual(expandedGroverResult.probability(12), 1.0),
        "Expanded Grover preserves idle qubits while marking q0/q1 as |11>"
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
        "Phase kickback is exposed as the final basis state |11>"
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
        "Rx(pi) rotates |0> to |1> up to phase"
    );

    check(
        approximatelyEqual(ryResult.probability(1), 1.0),
        "Ry(pi) rotates |0> to |1>"
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

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return 0;
}

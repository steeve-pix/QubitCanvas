#include "quantum_sim/circuit/QuantumCircuit.hpp"
#include "quantum_sim/gates/QuantumGates.hpp"
#include "quantum_sim/math/ComplexMatrix.hpp"
#include "quantum_sim/math/ComplexVector.hpp"
#include "quantum_sim/quantum/QuantumRegister.hpp"
#include "quantum_sim/quantum/Qubit.hpp"
#include "quantum_sim/project/ProjectFile.hpp"
#include "quantum_sim/project/OpenQasmFile.hpp"
#include "quantum_sim/project/ProjectWorkspace.hpp"
#include "quantum_sim/project/SubcircuitLibrary.hpp"
#include "quantum_sim/visualization/ConsoleVisualizer.hpp"
#include "quantum_sim/algorithms/QuantumAlgorithms.hpp"
#include "quantum_sim/analysis/StateMetrics.hpp"
#include "quantum_sim/debug/InteractiveCircuitDebugger.hpp"
#include "quantum_sim/gui/GateNotation.hpp"
#include "quantum_sim/gui/ExportFile.hpp"
#include "quantum_sim/gui/QuantumNotation.hpp"
#include "quantum_sim/gui/SimulationHistoryWorker.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeColorMap.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeModel.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeScene.hpp"
#include "quantum_sim/gui/rendering/DensityVolumeCameraController.hpp"
#include "quantum_sim/util/StopToken.hpp"

#include <algorithm>
#include <bit>
#include <chrono>
#include <sstream>
#include <iostream>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
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

    [[nodiscard]] double summedProbability(
        const QuantumRegister &state
    ) {
        double total{};

        for (std::size_t basisState = 0U; basisState < state.stateCount(); ++basisState) {
            total += state.probability(basisState);
        }

        return total;
    }
} //
int main() {
    {
        quantum_sim::util::StopSource source;
        const quantum_sim::util::StopToken firstToken =
                source.get_token();
        const quantum_sim::util::StopToken copiedToken =
                firstToken;
        const quantum_sim::util::StopToken emptyToken;

        check(
            !firstToken.stop_requested() &&
            !copiedToken.stop_requested() &&
            !emptyToken.stop_requested(),
            "Cancellation tokens start in the running state"
        );

        check(
            source.request_stop() &&
            !source.request_stop() &&
            firstToken.stop_requested() &&
            copiedToken.stop_requested() &&
            source.stop_requested(),
            "Cancellation requests reach every shared token once"
        );
    }

    {
        const std::filesystem::path exportRoot =
                std::filesystem::temp_directory_path() /
                "qubit_canvas_interchange_test";

        std::error_code cleanupError;
        std::filesystem::remove_all(
            exportRoot,
            cleanupError
        );
        std::filesystem::create_directories(exportRoot);

        QuantumCircuit exportedCircuit{3U};
        exportedCircuit.addSingleQubitGate(
            "H",
            quantum_sim::gates::hadamardGate(),
            0U
        );
        exportedCircuit.addSingleQubitGate(
            "Rx",
            quantum_sim::gates::rxGate(
                std::numbers::pi / 3.0
            ),
            1U,
            std::numbers::pi / 3.0
        );
        exportedCircuit.addTwoQubitGate(
            "CX",
            quantum_sim::gates::cxGate(),
            0U,
            2U
        );
        exportedCircuit.addThreeQubitGate(
            "CCX",
            quantum_sim::gates::ccxGate(),
            0U,
            1U,
            2U
        );

        const std::filesystem::path qasmPath =
                exportRoot / "circuit.qasm";

        quantum_sim::project::OpenQasmFile::save(
            qasmPath,
            exportedCircuit
        );

        const auto imported =
                quantum_sim::project::OpenQasmFile::load(
                    qasmPath
                );

        const QuantumRegister initial =
                QuantumRegister::basisState(3U, 0U);

        const QuantumRegister exportedState =
                exportedCircuit.execute(initial);

        const QuantumRegister importedState =
                imported.circuit.execute(
                    imported.initialState
                );

        bool qasmStatesMatch = true;

        for (
            std::size_t stateIndex = 0U;
            stateIndex < exportedState.stateCount();
            ++stateIndex
        ) {
            qasmStatesMatch =
                    qasmStatesMatch &&
                    approximatelyEqual(
                        exportedState.amplitude(
                            stateIndex
                        ).real(),
                        importedState.amplitude(
                            stateIndex
                        ).real()
                    ) &&
                    approximatelyEqual(
                        exportedState.amplitude(
                            stateIndex
                        ).imaginary(),
                        importedState.amplitude(
                            stateIndex
                        ).imaginary()
                    );
        }

        const std::filesystem::path svgPath =
                exportRoot / "circuit.svg";

        const std::filesystem::path stateCsvPath =
                exportRoot / "state.csv";

        const std::filesystem::path densityCsvPath =
                exportRoot / "density.csv";

        quantum_sim::gui::ExportFile::saveCircuitSvg(
            svgPath,
            exportedCircuit
        );

        quantum_sim::gui::ExportFile::saveStateCsv(
            stateCsvPath,
            exportedState
        );

        quantum_sim::debug::DebuggerSession exportSession{
            exportedCircuit,
            initial
        };

        const auto exportDensity =
                quantum_sim::gui::density_volume::DensityModel::
                    build(exportSession);

        quantum_sim::gui::ExportFile::saveDensityCsv(
            densityCsvPath,
            exportDensity.layers.back()
        );

        const auto readText =
                [](const std::filesystem::path &path) {
            std::ifstream input{path};
            return std::string{
                std::istreambuf_iterator<char>{input},
                std::istreambuf_iterator<char>{}
            };
        };

        const std::string qasmText =
                readText(qasmPath);

        const std::string svgText =
                readText(svgPath);

        const std::string stateCsvText =
                readText(stateCsvPath);

        const std::string densityCsvText =
                readText(densityCsvPath);

        check(
            imported.circuit.instructionCount() == 4U &&
            qasmStatesMatch &&
            qasmText.find("rx(pi/3)") != std::string::npos &&
            svgText.find("<svg") != std::string::npos &&
            svgText.find("JetBrains Mono") != std::string::npos &&
            stateCsvText.find("phase_radians") != std::string::npos &&
            densityCsvText.find("bucketed") != std::string::npos,
            "OpenQASM round-trips supported gates and readable exports contain their documented data"
        );

        std::filesystem::remove_all(
            exportRoot,
            cleanupError
        );
    }

    {
        QuantumCircuit bellCircuit{2U};
        bellCircuit.addSingleQubitGate(
            "H",
            quantum_sim::gates::hadamardGate(),
            0U
        );
        bellCircuit.addTwoQubitGate(
            "CX",
            quantum_sim::gates::cxGate(),
            0U,
            1U
        );

        const QuantumRegister zero =
                QuantumRegister::basisState(2U, 0U);

        const QuantumRegister bell =
                bellCircuit.execute(zero);

        const auto bellMetrics =
                quantum_sim::analysis::StateMetrics::
                    forRegister(bell);

        check(
            approximatelyEqual(
                quantum_sim::analysis::StateMetrics::fidelity(
                    zero,
                    bell
                ),
                0.5
            ) &&
            bellMetrics.size() == 2U &&
            approximatelyEqual(
                bellMetrics[0U].purity,
                0.5
            ) &&
            approximatelyEqual(
                bellMetrics[0U].entropyBits,
                1.0
            ) &&
            approximatelyEqual(
                bellMetrics[1U].purity,
                0.5
            ),
            "State metrics identify Bell-state fidelity, local mixing, and entanglement entropy"
        );

        const auto productMetrics =
                quantum_sim::analysis::StateMetrics::
                    forRegister(zero);

        check(
            approximatelyEqual(
                productMetrics[0U].purity,
                1.0
            ) &&
            approximatelyEqual(
                productMetrics[0U].entropyBits,
                0.0
            ),
            "State metrics keep product-state purity and entropy exact"
        );
    }

    {
        const std::filesystem::path libraryRoot =
                std::filesystem::temp_directory_path() /
                "qubit_canvas_subcircuit_test";

        std::error_code cleanupError;
        std::filesystem::remove_all(
            libraryRoot,
            cleanupError
        );

        QuantumCircuit source{3U};
        source.addSingleQubitGate(
            "H",
            quantum_sim::gates::hadamardGate(),
            1U
        );
        source.addTwoQubitGate(
            "CX",
            quantum_sim::gates::cxGate(),
            1U,
            2U
        );

        quantum_sim::project::SubcircuitLibrary library{
            libraryRoot
        };

        library.save(
            "Bell pair",
            source.qubitCount(),
            source.instructionSnapshots()
        );

        const auto blocks = library.loadAll();
        QuantumCircuit destination{4U};

        if (!blocks.empty()) {
            for (const auto &instruction : blocks.front().instructions) {
                destination.insertInstructionSnapshot(
                    destination.instructionCount(),
                    instruction
                );
            }
        }

        const QuantumRegister prepared =
                destination.execute(
                    QuantumRegister::basisState(4U, 0U)
                );

        check(
            blocks.size() == 1U &&
            blocks.front().name == "Bell pair" &&
            blocks.front().canInsertInto(4U) &&
            !blocks.front().canInsertInto(2U) &&
            approximatelyEqual(prepared.probability(0U), 0.5) &&
            approximatelyEqual(prepared.probability(6U), 0.5),
            "Reusable subcircuits preserve executable snapshots and validate destination registers"
        );

        std::filesystem::remove_all(
            libraryRoot,
            cleanupError
        );
    }

    {
        QuantumCircuit editable{1U};
        editable.addSingleQubitGate(
            "Rx",
            quantum_sim::gates::rxGate(
                std::numbers::pi / 4.0
            ),
            0U,
            std::numbers::pi / 4.0
        );

        auto replacement =
                editable.instructionSnapshots().front();

        replacement.angleRadians =
                std::numbers::pi;

        replacement.matrix =
                quantum_sim::gates::rxGate(
                    std::numbers::pi
                );

        const bool replaced =
                editable.replaceInstructionSnapshot(
                    0U,
                    replacement
                );

        const QuantumRegister editedState =
                editable.execute(
                    QuantumRegister::basisState(1U, 0U)
                );

        check(
            replaced &&
            approximatelyEqual(
                editedState.probability(1U),
                1.0
            ),
            "Instruction snapshots can replace a gate without weakening validation"
        );
    }

    {
        const std::filesystem::path workspaceRoot =
                std::filesystem::temp_directory_path() /
                "qubit_canvas_workspace_test";

        std::error_code cleanupError;
        std::filesystem::remove_all(
            workspaceRoot,
            cleanupError
        );

        quantum_sim::project::ProjectWorkspace firstSession{
            workspaceRoot
        };

        const bool firstWasUnclean =
                firstSession.beginSession();

        quantum_sim::project::ProjectWorkspace secondSession{
            workspaceRoot
        };

        const bool secondWasUnclean =
                secondSession.beginSession();

        const std::filesystem::path recentProject =
                workspaceRoot / "recent.qcanvas";

        QuantumCircuit recentCircuit{1U};
        const QuantumRegister recentState =
                QuantumRegister::basisState(1U, 0U);

        quantum_sim::project::ProjectFile::save(
            recentProject,
            recentCircuit,
            recentState
        );

        secondSession.recordRecentProject(recentProject);

        quantum_sim::project::ProjectFile::save(
            secondSession.autosavePath(),
            recentCircuit,
            recentState
        );

        const bool workspaceBehavesCorrectly =
                !firstWasUnclean &&
                secondWasUnclean &&
                secondSession.recoveryAvailable() &&
                secondSession.recentProjects().size() == 1U;

        secondSession.discardRecovery();
        secondSession.endSession();
        firstSession.endSession();
        std::filesystem::remove_all(
            workspaceRoot,
            cleanupError
        );

        check(
            workspaceBehavesCorrectly &&
            !cleanupError,
            "Workspace detects interrupted sessions and persists recovery and recent projects"
        );
    }

    {
        QuantumCircuit savedCircuit{3U};
        savedCircuit.addSingleQubitGate(
            "H",
            quantum_sim::gates::hadamardGate(),
            0U
        );
        savedCircuit.addSingleQubitGate(
            "Rx",
            quantum_sim::gates::rxGate(
                std::numbers::pi / 3.0
            ),
            1U,
            std::numbers::pi / 3.0
        );
        savedCircuit.addTwoQubitGate(
            "CX",
            quantum_sim::gates::cxGate(),
            0U,
            2U
        );
        savedCircuit.addThreeQubitGate(
            "CCX",
            quantum_sim::gates::ccxGate(),
            0U,
            1U,
            2U
        );

        std::vector<Complex> reflectionValues(8U);
        reflectionValues[5U] = Complex{1.0, 0.0};

        savedCircuit.addReflection(
            "Saved reflection",
            ComplexVector{std::move(reflectionValues)},
            2U
        );

        const QuantumRegister savedInitialState =
                QuantumRegister::basisState(3U, 0U);

        const std::filesystem::path projectPath =
                std::filesystem::temp_directory_path() /
                "qubit_canvas_project_round_trip.qcanvas";

        quantum_sim::project::ProjectFile::save(
            projectPath,
            savedCircuit,
            savedInitialState
        );

        const quantum_sim::project::ProjectDocument loadedProject =
                quantum_sim::project::ProjectFile::load(
                    projectPath
                );

        std::error_code removeError;
        std::filesystem::remove(projectPath, removeError);

        const QuantumRegister savedFinalState =
                savedCircuit.execute(savedInitialState);

        const QuantumRegister loadedFinalState =
                loadedProject.circuit.execute(
                    loadedProject.initialState
                );

        bool statesMatch =
                savedFinalState.stateCount() ==
                loadedFinalState.stateCount();

        for (
            std::size_t stateIndex = 0U;
            statesMatch &&
            stateIndex < savedFinalState.stateCount();
            ++stateIndex
        ) {
            statesMatch =
                    approximatelyEqual(
                        savedFinalState.amplitude(stateIndex).real(),
                        loadedFinalState.amplitude(stateIndex).real()
                    ) &&
                    approximatelyEqual(
                        savedFinalState.amplitude(stateIndex).imaginary(),
                        loadedFinalState.amplitude(stateIndex).imaginary()
                    );
        }

        const auto loadedInstructions =
                loadedProject.circuit.instructionSnapshots();

        check(
            loadedProject.circuit.qubitCount() == 3U &&
            loadedInstructions.size() == 5U &&
            loadedInstructions[1U].angleRadians.has_value() &&
            approximatelyEqual(
                loadedInstructions[1U].angleRadians.value(),
                std::numbers::pi / 3.0
            ) &&
            loadedInstructions.back().reflectionAxis.has_value() &&
            statesMatch &&
            !removeError,
            "Project files preserve initial state, gate matrices, angles, operands, and reflections"
        );
    }

    {
        QuantumCircuit cancellableCircuit{1U};
        cancellableCircuit.addSingleQubitGate(
            "H",
            quantum_sim::gates::hadamardGate(),
            0U
        );

        quantum_sim::util::StopSource cancelledBuild;
        cancelledBuild.request_stop();

        bool cancellationObserved = false;

        try {
            static_cast<void>(
                cancellableCircuit.executeWithTrace(
                    QuantumRegister::basisState(1U, 0U),
                    cancelledBuild.get_token()
                )
            );
        } catch (const quantum_sim::circuit::TraceBuildCancelled &) {
            cancellationObserved = true;
        }

        check(
            cancellationObserved,
            "Circuit traces cooperatively stop before cancelled background work executes"
        );
    }

    {
        QuantumCircuit comparisonCircuit{2U};
        comparisonCircuit.addSingleQubitGate(
            "H",
            quantum_sim::gates::hadamardGate(),
            0U
        );

        const quantum_sim::debug::DebuggerSession comparisonSession{
            comparisonCircuit,
            QuantumRegister::basisState(2U, 0U)
        };

        const auto comparisonStack =
                quantum_sim::gui::density_volume::DensityModel::build(
                    comparisonSession
                );

        const auto differenceLayer =
                quantum_sim::gui::density_volume::DensityModel::difference(
                    comparisonStack.layers[1U],
                    comparisonStack.layers[0U]
                );

        check(
            approximatelyEqual(
                differenceLayer.cellAt(0U, 0U).real,
                -0.5
            ) &&
            approximatelyEqual(
                differenceLayer.cellAt(0U, 0U).magnitude,
                0.5
            ) &&
            approximatelyEqual(
                differenceLayer.cellAt(0U, 1U).real,
                0.5
            ),
            "Density comparison preserves the complex selected-minus-reference delta"
        );

        const auto isolatedScene =
                quantum_sim::gui::density_volume::SceneBuilder::build(
                    comparisonStack,
                    1U,
                    quantum_sim::gui::density_volume::VisualizationMode::LayerStack,
                    quantum_sim::gui::density_volume::SceneViewOptions{
                        .isolateSelectedLayer = true
                    }
                );

        const auto differenceScene =
                quantum_sim::gui::density_volume::SceneBuilder::build(
                    comparisonStack,
                    1U,
                    quantum_sim::gui::density_volume::VisualizationMode::LayerStack,
                    quantum_sim::gui::density_volume::SceneViewOptions{
                        .comparisonLayer = 0U
                    }
                );

        const bool isolatedPicksSelectedLayer =
                std::all_of(
                    isolatedScene.pickRecords.begin(),
                    isolatedScene.pickRecords.end(),
                    [](const auto &selection) {
                        return selection.layer == 1U;
                    }
                );

        check(
            !isolatedScene.voxels.empty() &&
            isolatedPicksSelectedLayer &&
            isolatedScene.framingMaximum.x -
                isolatedScene.framingMinimum.x < 1.0F &&
            !differenceScene.voxels.empty(),
            "Density isolation and comparison build one focused vertical matrix scene"
        );
    }

    {
        QuantumCircuit baseCircuit{1U};
        const QuantumRegister baseState =
                QuantumRegister::basisState(1U, 0U);

        quantum_sim::debug::DebuggerSession baseSession{
            baseCircuit,
            baseState
        };

        const auto baseDensity =
                quantum_sim::gui::density_volume::DensityModel::build(
                    baseSession
                );

        quantum_sim::gui::SimulationHistoryWorker worker;

        const QuantumCircuit supersededCircuit =
                quantum_sim::algorithms::randomCircuit(
                    10U,
                    0xA51CULL
                );

        static_cast<void>(
            worker.request(
                supersededCircuit,
                QuantumRegister::basisState(10U, 0U),
                baseSession,
                baseDensity,
                std::nullopt,
                std::nullopt,
                false
            )
        );

        QuantumCircuit latestCircuit{1U};
        latestCircuit.addSingleQubitGate(
            "X",
            quantum_sim::gates::xGate(),
            0U
        );

        const std::uint64_t latestRequest =
                worker.request(
                    latestCircuit,
                    baseState,
                    baseSession,
                    baseDensity,
                    std::nullopt,
                    1U,
                    true
                );

        std::optional<quantum_sim::gui::SimulationHistoryResult> completed;
        const auto deadline =
                std::chrono::steady_clock::now() +
                std::chrono::seconds{3};

        while (
            !completed.has_value() &&
            std::chrono::steady_clock::now() < deadline
        ) {
            completed = worker.takeCompleted();

            if (!completed.has_value()) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds{2}
                );
            }
        }

        check(
            completed.has_value() &&
            completed->requestId == latestRequest &&
            completed->session.has_value() &&
            completed->session->stepCount() == 1U &&
            completed->error.empty(),
            "Background history worker publishes only the newest requested circuit"
        );
    }

    const std::vector<std::string_view> builtInGateNames{
        "H", "X", "Y", "Z", "S", "Sdg", "T", "Tdg", "SX", "SXdg",
        "P", "U", "Rx", "Ry", "Rz",
        "CX", "CY", "CZ", "SWAP", "iSWAP",
        "CP", "CRx", "CRy", "CRz",
        "RXX", "RYY", "RZZ",
        "CH", "CS", "CSdg", "CT", "CTdg",
        "DCX", "ECR", "sqrtSWAP", "fSim",
        "CCX", "CSWAP"
    };

    std::unordered_set<std::string> circuitGateLabels;

    for (const std::string_view gateName : builtInGateNames) {
        circuitGateLabels.emplace(
            quantum_sim::gui::gate_notation::circuitLabel(
                gateName
            )
        );
    }

    check(
        circuitGateLabels.size() == builtInGateNames.size(),
        "Every built-in gate has a unique circuit label"
    );

    check(
        quantum_sim::gui::gate_notation::circuitLabel("CS") == "CS" &&
        quantum_sim::gui::gate_notation::circuitLabel("CSdg") ==
            "CS\xE2\x80\xA0" &&
        quantum_sim::gui::gate_notation::circuitLabel("CT") == "CT" &&
        quantum_sim::gui::gate_notation::circuitLabel("CTdg") ==
            "CT\xE2\x80\xA0",
        "Controlled phase and dagger labels remain visually distinct"
    );

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

    QuantumCircuit reorderedCircuit{1U};
    reorderedCircuit.addSingleQubitGate(
        "H",
        quantum_sim::gates::hadamardGate(),
        0U
    );
    reorderedCircuit.addSingleQubitGate(
        "Z",
        quantum_sim::gates::zGate(),
        0U
    );

    const bool instructionMoved =
            reorderedCircuit.moveInstruction(1U, 0U);

    const auto reorderedInstructions =
            reorderedCircuit.instructionInfo();

    const QuantumRegister reorderedResult =
            reorderedCircuit.execute(
                QuantumRegister::basisState(1U, 0U)
            );

    check(
        instructionMoved &&
        reorderedInstructions.front().name == "Z" &&
        reorderedInstructions.back().name == "H" &&
        reorderedResult.amplitude(1U).real() > 0.0,
        "Circuit instruction movement preserves gate data and changes execution order"
    );

    QuantumCircuit incrementallyEditedCircuit{1U};
    incrementallyEditedCircuit.addSingleQubitGate(
        "H",
        quantum_sim::gates::hadamardGate(),
        0U
    );
    incrementallyEditedCircuit.addSingleQubitGate(
        "S",
        quantum_sim::gates::sGate(),
        0U
    );

    const QuantumRegister incrementalInitialState =
            QuantumRegister::basisState(1U, 0U);

    quantum_sim::debug::DebuggerSession incrementalSession{
        incrementallyEditedCircuit,
        incrementalInitialState
    };

    const Complex preservedPrefixAmplitude =
            incrementalSession.stepAt(0U).state.amplitude(1U);

    incrementallyEditedCircuit.insertSingleQubitGate(
        1U,
        "X",
        quantum_sim::gates::xGate(),
        0U
    );

    incrementalSession.rebuildFrom(
        incrementallyEditedCircuit,
        incrementalInitialState,
        1U
    );

    const QuantumRegister incrementalExpectedResult =
            incrementallyEditedCircuit.execute(
                incrementalInitialState
            );

    check(
        incrementalSession.stepCount() == 3U &&
        approximatelyEqual(
            incrementalSession.stepAt(0U).state.amplitude(1U).real(),
            preservedPrefixAmplitude.real()
        ) &&
        approximatelyEqual(
            incrementalSession.stepAt(0U).state.amplitude(1U).imaginary(),
            preservedPrefixAmplitude.imaginary()
        ) &&
        approximatelyEqual(
            incrementalSession.stepAt(2U).state.amplitude(0U).real(),
            incrementalExpectedResult.amplitude(0U).real()
        ) &&
        approximatelyEqual(
            incrementalSession.stepAt(2U).state.amplitude(0U).imaginary(),
            incrementalExpectedResult.amplitude(0U).imaginary()
        ) &&
        approximatelyEqual(
            incrementalSession.stepAt(2U).state.amplitude(1U).real(),
            incrementalExpectedResult.amplitude(1U).real()
        ) &&
        approximatelyEqual(
            incrementalSession.stepAt(2U).state.amplitude(1U).imaginary(),
            incrementalExpectedResult.amplitude(1U).imaginary()
        ),
        "Debugger suffix rebuild preserves the valid prefix and matches full execution"
    );

    quantum_sim::gui::density_volume::DensityStack incrementalDensityStack =
            quantum_sim::gui::density_volume::DensityModel::build(
                incrementalSession,
                16U
            );

    incrementallyEditedCircuit.addSingleQubitGate(
        "T",
        quantum_sim::gates::tGate(),
        0U
    );

    incrementalSession.rebuildFrom(
        incrementallyEditedCircuit,
        incrementalInitialState,
        3U
    );

    quantum_sim::gui::density_volume::DensityModel::rebuildFrom(
        incrementalDensityStack,
        incrementalSession,
        3U,
        16U
    );

    const auto fullyRebuiltDensityStack =
            quantum_sim::gui::density_volume::DensityModel::build(
                incrementalSession,
                16U
            );

    check(
        incrementalDensityStack.fingerprint ==
            fullyRebuiltDensityStack.fingerprint &&
        incrementalDensityStack.layers.size() ==
            fullyRebuiltDensityStack.layers.size() &&
        approximatelyEqual(
            incrementalDensityStack.layers.back().cellAt(0U, 1U).real,
            fullyRebuiltDensityStack.layers.back().cellAt(0U, 1U).real
        ) &&
        approximatelyEqual(
            incrementalDensityStack.layers.back().cellAt(0U, 1U).imaginary,
            fullyRebuiltDensityStack.layers.back().cellAt(0U, 1U).imaginary
        ),
        "Density history suffix rebuild matches a complete numerical rebuild"
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
        quantum_sim::algorithms::scrambleCircuit(10U),
        quantum_sim::algorithms::simonCircuit(10U),
        quantum_sim::algorithms::shorPeriodFindingCircuit(10U),
        quantum_sim::algorithms::quantumPhaseEstimationCircuit(10U),
        quantum_sim::algorithms::vqeAnsatzCircuit(10U),
        quantum_sim::algorithms::qaoaMaxCutCircuit(10U),
        quantum_sim::algorithms::hhlDemoCircuit(10U),
        quantum_sim::algorithms::swapTestCircuit(10U),
        quantum_sim::algorithms::quantumWalkCircuit(10U),
        quantum_sim::algorithms::bb84DemoCircuit(10U),
        quantum_sim::algorithms::superdenseCodingCircuit(10U),
        quantum_sim::algorithms::wStateCircuit(10U),
        quantum_sim::algorithms::dickeStateCircuit(10U, 2U),
        quantum_sim::algorithms::graphStateCircuit(10U),
        quantum_sim::algorithms::randomCircuit(10U, 42U),
        quantum_sim::algorithms::weightedStatePreparationCircuit(10U),
        quantum_sim::algorithms::bitFlipCodeCircuit(10U),
        quantum_sim::algorithms::steaneCodeCircuit(10U),
        quantum_sim::algorithms::shorCodeCircuit(10U),
        quantum_sim::algorithms::phaseFlipCodeCircuit(10U),
        quantum_sim::algorithms::fiveQubitCodeCircuit(10U),
        quantum_sim::algorithms::quantumCountingCircuit(10U),
        quantum_sim::algorithms::amplitudeEstimationCircuit(10U),
        quantum_sim::algorithms::rippleCarryAdderCircuit(10U),
        quantum_sim::algorithms::draperAdderCircuit(10U),
        quantum_sim::algorithms::iqpCircuit(10U),
        quantum_sim::algorithms::surfaceCodeStabilizerCircuit(10U)
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
        quantum_sim::algorithms::scrambleCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::simonCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::shorPeriodFindingCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::quantumPhaseEstimationCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::vqeAnsatzCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::qaoaMaxCutCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::hhlDemoCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::swapTestCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::quantumWalkCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::bb84DemoCircuit(4).qubitCount() == 4U &&
        quantum_sim::algorithms::superdenseCodingCircuit(4).qubitCount() == 4U,
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
        approximatelyEqual(
            quantum_sim::gui::density_volume::normalizeMagnitude(
                0.125,
                0.5
            ),
            0.25
        ) &&
        approximatelyEqual(
            quantum_sim::gui::density_volume::normalizeMagnitude(
                0.5,
                0.5
            ),
            1.0
        ) &&
        approximatelyEqual(
            quantum_sim::gui::density_volume::normalizeMagnitude(
                0.5,
                0.0
            ),
            0.0
        ),
        "Density Volume views share one stable history-wide magnitude normalization"
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
        approximatelyEqual(floorPeak->magnitude, 0.5, 1.0e-5) &&
        approximatelyEqual(floorPeak->size.y, 4.4, 1.0e-5),
        "Density Volume floor field separates relative height from shared Inferno color"
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

    const auto *nextRowPeak =
            findInstance(
                historyScene,
                historyScene.voxels,
                1U,
                1U,
                0U
            );

    const auto *nextColumnPeak =
            findInstance(
                historyScene,
                historyScene.voxels,
                1U,
                0U,
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
        floorPeak != nullptr &&
        hadamardPeak != nullptr &&
        approximatelyEqual(
            floorPeak->magnitude,
            hadamardPeak->magnitude,
            1.0e-5
        ) &&
        approximatelyEqual(
            floorPeak->color.red,
            hadamardPeak->color.red,
            1.0e-5
        ) &&
        approximatelyEqual(
            floorPeak->color.green,
            hadamardPeak->color.green,
            1.0e-5
        ) &&
        approximatelyEqual(
            floorPeak->color.blue,
            hadamardPeak->color.blue,
            1.0e-5
        ),
        "Density Volume floor and stack modes assign matching colors to the same layer cell"
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
        historyScene.layerCenters.size() ==
            densityStack.layers.size() &&
        historyScene.layerEndInstanceCounts.at(1U) == 5U &&
        historyScene.layerEndGhostCounts.at(1U) == 3U,
        "Density Volume history separates solid values from small-matrix edge ghosts"
    );

    check(
        initialPeak != nullptr &&
        hadamardPeak != nullptr &&
        dormantCell != nullptr &&
        nextRowPeak != nullptr &&
        nextColumnPeak != nullptr &&
        nextLayerPeak != nullptr &&
        initialPeak->center.x < hadamardPeak->center.x &&
        approximatelyEqual(initialPeak->center.y, hadamardPeak->center.y) &&
        approximatelyEqual(initialPeak->center.z, hadamardPeak->center.z) &&
        nextRowPeak->center.y < hadamardPeak->center.y &&
        approximatelyEqual(nextRowPeak->center.z, hadamardPeak->center.z) &&
        nextColumnPeak->center.z > hadamardPeak->center.z &&
        approximatelyEqual(nextColumnPeak->center.y, hadamardPeak->center.y) &&
        nextLayerPeak->center.x - initialPeak->center.x >
            (nextLayerPeak->size.x + initialPeak->size.x) * 0.5F &&
        approximatelyEqual(
            historyScene.layerCenters.at(1U).x -
                historyScene.layerCenters.at(0U).x,
            historyScene.layerSpacing
        ) &&
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

    check(
        initialPeak != nullptr &&
        nextLayerPeak != nullptr &&
        historyScene.framingMinimum.x <=
            initialPeak->center.x - initialPeak->size.x * 0.5F &&
        historyScene.framingMinimum.y <=
            initialPeak->center.y - initialPeak->size.y * 0.5F &&
        historyScene.framingMaximum.x >=
            nextLayerPeak->center.x + nextLayerPeak->size.x * 0.5F &&
        historyScene.framingMaximum.z >=
            nextLayerPeak->center.z + nextLayerPeak->size.z * 0.5F,
        "Density Volume exposes complete history bounds for launch framing"
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

    const quantum_sim::gui::density_volume::VoxelGeometry roundedTopColumn =
            quantum_sim::gui::density_volume::VoxelGeometryBuilder::
                buildRoundedTopColumn();

    const bool hasSquareColumnBase =
            std::any_of(
                roundedTopColumn.vertices.begin(),
                roundedTopColumn.vertices.end(),
                [](const auto &vertex) {
                    return
                            approximatelyEqual(
                                std::abs(vertex.position[0]),
                                0.5,
                                0.0001
                            ) &&
                            approximatelyEqual(
                                vertex.position[1],
                                -0.5,
                                0.0001
                            ) &&
                            approximatelyEqual(
                                std::abs(vertex.position[2]),
                                0.5,
                                0.0001
                            );
                }
            );

    const bool hasRoundedColumnTop =
            std::any_of(
                roundedTopColumn.vertices.begin(),
                roundedTopColumn.vertices.end(),
                [](const auto &vertex) {
                    return
                            vertex.position[0] > 0.39F &&
                            vertex.position[0] < 0.49F &&
                            vertex.position[1] > 0.39F &&
                            vertex.position[1] < 0.49F &&
                            vertex.position[2] > 0.39F &&
                            vertex.position[2] < 0.49F;
                }
            );

    check(
        roundedTopColumn.vertices.size() == 486U &&
        roundedTopColumn.indices.size() == 2304U &&
        hasSquareColumnBase &&
        hasRoundedColumnTop &&
        std::all_of(
            roundedTopColumn.vertices.begin(),
            roundedTopColumn.vertices.end(),
            [](const auto &vertex) {
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
        "Floor Field uses a smooth rounded cap above a square column base"
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
        expandedGroverResult.probability(15U) > 0.94,
        "Expanded Grover searches the complete register for |1111⟩"
    );

    const QuantumRegister wStateResult =
            quantum_sim::algorithms::wStateCircuit(4U).execute(
                QuantumRegister::basisState(4U, 0U)
            );

    double wStateProbability{};

    for (std::size_t state = 0U; state < wStateResult.stateCount(); ++state) {
        if (std::popcount(state) == 1) {
            wStateProbability +=
                    wStateResult.probability(state);
        }
    }

    check(
        approximatelyEqual(wStateProbability, 1.0) &&
        approximatelyEqual(wStateResult.probability(1U), 0.25),
        "W-state preparation shares one excitation uniformly"
    );

    const QuantumRegister dickeStateResult =
            quantum_sim::algorithms::dickeStateCircuit(4U, 2U).execute(
                QuantumRegister::basisState(4U, 0U)
            );

    double dickeProbability{};

    for (std::size_t state = 0U; state < dickeStateResult.stateCount(); ++state) {
        if (std::popcount(state) == 2) {
            dickeProbability +=
                    dickeStateResult.probability(state);
        }
    }

    check(
        approximatelyEqual(dickeProbability, 1.0) &&
        approximatelyEqual(dickeStateResult.probability(3U), 1.0 / 6.0),
        "Dicke-state preparation populates exactly the requested Hamming weight"
    );

    const QuantumRegister weightedStateResult =
            quantum_sim::algorithms::weightedStatePreparationCircuit(6U).execute(
                QuantumRegister::basisState(6U, 0U)
            );

    check(
        approximatelyEqual(summedProbability(weightedStateResult), 1.0) &&
        !approximatelyEqual(
            weightedStateResult.probability(0U),
            weightedStateResult.probability(1U)
        ),
        "Weighted state preparation produces normalized uneven probabilities"
    );

    const QuantumRegister randomCircuitResult =
            quantum_sim::algorithms::randomCircuit(
                6U,
                0x12345678ULL
            ).execute(
                QuantumRegister::basisState(6U, 0U)
            );

    double minimumRandomProbability = 1.0;
    double maximumRandomProbability = 0.0;

    for (std::size_t state = 0U; state < randomCircuitResult.stateCount(); ++state) {
        minimumRandomProbability =
                std::min(
                    minimumRandomProbability,
                    randomCircuitResult.probability(state)
                );

        maximumRandomProbability =
                std::max(
                    maximumRandomProbability,
                    randomCircuitResult.probability(state)
                );
    }

    check(
        approximatelyEqual(summedProbability(randomCircuitResult), 1.0) &&
        maximumRandomProbability - minimumRandomProbability > 0.01,
        "Seeded random circuits are reproducible and visibly non-uniform"
    );

    const QuantumRegister graphStateResult =
            quantum_sim::algorithms::graphStateCircuit(6U).execute(
                QuantumRegister::basisState(6U, 0U)
            );

    const QuantumRegister bitFlipCodeResult =
            quantum_sim::algorithms::bitFlipCodeCircuit().execute(
                QuantumRegister::basisState(3U, 0U)
            );

    const QuantumRegister steaneCodeResult =
            quantum_sim::algorithms::steaneCodeCircuit().execute(
                QuantumRegister::basisState(7U, 0U)
            );

    const QuantumRegister shorCodeResult =
            quantum_sim::algorithms::shorCodeCircuit().execute(
                QuantumRegister::basisState(9U, 0U)
            );

    check(
        approximatelyEqual(summedProbability(graphStateResult), 1.0) &&
        approximatelyEqual(bitFlipCodeResult.probabilityOfQubitOne(0U), 0.25) &&
        approximatelyEqual(summedProbability(steaneCodeResult), 1.0) &&
        approximatelyEqual(summedProbability(shorCodeResult), 1.0),
        "Graph and error-correction presets preserve normalized quantum states"
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

    const QuantumRegister simonResult =
            quantum_sim::algorithms::simonCircuit().execute(
                QuantumRegister::basisState(4U, 0U)
            );

    double validSimonProbability{};

    for (std::size_t basisState = 0U; basisState < simonResult.stateCount(); ++basisState) {
        const std::size_t inputMeasurement =
                basisState >> 2U;

        if (inputMeasurement == 0U || inputMeasurement == 3U) {
            validSimonProbability +=
                    simonResult.probability(basisState);
        }
    }

    check(
        approximatelyEqual(validSimonProbability, 1.0),
        "Simon emits only measurements orthogonal to hidden period 11"
    );

    const QuantumRegister shorResult =
            quantum_sim::algorithms::shorPeriodFindingCircuit().execute(
                QuantumRegister::basisState(4U, 0U)
            );

    double validShorProbability{};

    for (std::size_t basisState = 0U; basisState < shorResult.stateCount(); ++basisState) {
        const std::size_t countingMeasurement =
                basisState >> 1U;

        if (countingMeasurement == 0U || countingMeasurement == 4U) {
            validShorProbability +=
                    shorResult.probability(basisState);
        }
    }

    check(
        approximatelyEqual(validShorProbability, 1.0),
        "Compiled Shor period finding resolves period two"
    );

    const QuantumRegister phaseEstimationResult =
            quantum_sim::algorithms::quantumPhaseEstimationCircuit().execute(
                QuantumRegister::basisState(3U, 0U)
            );

    check(
        approximatelyEqual(phaseEstimationResult.probability(3U), 1.0),
        "QPE resolves eigenphase one quarter as counting result 01"
    );

    const QuantumRegister vqeResult =
            quantum_sim::algorithms::vqeAnsatzCircuit().execute(
                QuantumRegister::basisState(2U, 0U)
            );

    const QuantumRegister qaoaResult =
            quantum_sim::algorithms::qaoaMaxCutCircuit().execute(
                QuantumRegister::basisState(3U, 0U)
            );

    const QuantumRegister hhlResult =
            quantum_sim::algorithms::hhlDemoCircuit().execute(
                QuantumRegister::basisState(4U, 0U)
            );

    const QuantumRegister quantumWalkResult =
            quantum_sim::algorithms::quantumWalkCircuit().execute(
                QuantumRegister::basisState(3U, 0U)
            );

    check(
        approximatelyEqual(summedProbability(vqeResult), 1.0) &&
        approximatelyEqual(summedProbability(qaoaResult), 1.0) &&
        approximatelyEqual(summedProbability(hhlResult), 1.0) &&
        approximatelyEqual(summedProbability(quantumWalkResult), 1.0),
        "Parameterized VQE, QAOA, HHL, and quantum-walk demos remain normalized"
    );

    const QuantumRegister swapTestResult =
            quantum_sim::algorithms::swapTestCircuit().execute(
                QuantumRegister::basisState(3U, 0U)
            );

    check(
        approximatelyEqual(
            swapTestResult.probabilityOfQubitZero(0U),
            0.75
        ),
        "SWAP test reports the expected overlap between plus and one states"
    );

    const QuantumRegister bb84Result =
            quantum_sim::algorithms::bb84DemoCircuit().execute(
                QuantumRegister::basisState(2U, 0U)
            );

    check(
        approximatelyEqual(bb84Result.probabilityOfQubitOne(0U), 1.0) &&
        approximatelyEqual(bb84Result.probabilityOfQubitOne(1U), 0.5),
        "BB84 separates matching-basis certainty from mismatched-basis randomness"
    );

    const QuantumRegister superdenseResult =
            quantum_sim::algorithms::superdenseCodingCircuit().execute(
                QuantumRegister::basisState(2U, 0U)
            );

    check(
        approximatelyEqual(superdenseResult.probability(3U), 1.0),
        "Superdense coding decodes classical message 11"
    );

    bool rejectedUndersizedSimon = false;

    try {
        static_cast<void>(
            quantum_sim::algorithms::simonCircuit(3U)
        );
    } catch (const std::invalid_argument &) {
        rejectedUndersizedSimon = true;
    }

    check(
        rejectedUndersizedSimon,
        "Simon rejects registers below its four-qubit minimum"
    );

    const QuantumRegister directCcxResult =
            QuantumRegister::basisState(3U, 6U).applyThreeQubitGate(
                quantum_sim::gates::ccxGate(),
                0U,
                1U,
                2U
            );

    const QuantumRegister directCswapResult =
            QuantumRegister::basisState(3U, 5U).applyThreeQubitGate(
                quantum_sim::gates::cSwapGate(),
                0U,
                1U,
                2U
            );

    check(
        approximatelyEqual(directCcxResult.probability(7U), 1.0) &&
        approximatelyEqual(directCswapResult.probability(6U), 1.0),
        "Compact CCX and CSWAP execute with standard local basis ordering"
    );

    QuantumCircuit threeQubitCircuit{10U};
    threeQubitCircuit.insertThreeQubitGate(
        4U,
        "CCX",
        quantum_sim::gates::ccxGate(),
        9U,
        0U,
        5U
    );

    const auto threeQubitInfo =
            threeQubitCircuit.instructionInfo().front();

    const QuantumRegister nonAdjacentCcxResult =
            threeQubitCircuit.execute(
                QuantumRegister::basisState(10U, 513U)
            );

    check(
        threeQubitInfo.kind ==
            quantum_sim::circuit::CircuitInstructionKind::ThreeQubit &&
        threeQubitInfo.controlQubit == 9U &&
        threeQubitInfo.secondaryTargetQubit == 0U &&
        threeQubitInfo.tertiaryTargetQubit == 5U &&
        approximatelyEqual(
            nonAdjacentCcxResult.probability(529U),
            1.0
        ),
        "Three-qubit metadata and execution support non-adjacent ten-qubit operands"
    );

    const QuantumRegister phaseFlipResult =
            quantum_sim::algorithms::phaseFlipCodeCircuit().execute(
                QuantumRegister::basisState(3U, 0U)
            );

    check(
        approximatelyEqual(
            phaseFlipResult.probabilityOfQubitOne(0U),
            0.25
        ) &&
        approximatelyEqual(
            summedProbability(phaseFlipResult),
            1.0
        ),
        "Phase-flip code restores the prepared logical qubit after a Z error"
    );

    const QuantumRegister fiveQubitCodeResult =
            quantum_sim::algorithms::fiveQubitCodeCircuit().execute(
                QuantumRegister::basisState(5U, 0U)
            );

    std::size_t fiveQubitPopulatedStates{};

    for (
        std::size_t state = 0U;
        state < fiveQubitCodeResult.stateCount();
        ++state
    ) {
        if (fiveQubitCodeResult.probability(state) > 1.0e-10) {
            ++fiveQubitPopulatedStates;
        }
    }

    check(
        fiveQubitPopulatedStates == 16U &&
        approximatelyEqual(
            fiveQubitCodeResult.probability(0U),
            1.0 / 16.0
        ) &&
        approximatelyEqual(
            summedProbability(fiveQubitCodeResult),
            1.0
        ),
        "Five-qubit perfect code prepares its exact normalized sixteen-term codeword"
    );

    const QuantumRegister rippleAdderResult =
            quantum_sim::algorithms::rippleCarryAdderCircuit().execute(
                QuantumRegister::basisState(4U, 0U)
            );

    const QuantumRegister draperAdderResult =
            quantum_sim::algorithms::draperAdderCircuit().execute(
                QuantumRegister::basisState(4U, 0U)
            );

    check(
        approximatelyEqual(rippleAdderResult.probability(7U), 1.0) &&
        approximatelyEqual(draperAdderResult.probability(7U), 1.0),
        "Ripple-carry and Draper adders both produce the deterministic result 1 + 2 = 3"
    );

    const QuantumRegister quantumCountingResult =
            quantum_sim::algorithms::quantumCountingCircuit().execute(
                QuantumRegister::basisState(5U, 0U)
            );

    const QuantumRegister amplitudeEstimationResult =
            quantum_sim::algorithms::amplitudeEstimationCircuit().execute(
                QuantumRegister::basisState(4U, 0U)
            );

    const QuantumRegister iqpResult =
            quantum_sim::algorithms::iqpCircuit(5U).execute(
                QuantumRegister::basisState(5U, 0U)
            );

    const QuantumRegister surfaceCodeResult =
            quantum_sim::algorithms::surfaceCodeStabilizerCircuit().execute(
                QuantumRegister::basisState(10U, 0U)
            );

    double iqpMinimumProbability = 1.0;
    double iqpMaximumProbability = 0.0;

    for (std::size_t state = 0U; state < iqpResult.stateCount(); ++state) {
        iqpMinimumProbability =
                std::min(
                    iqpMinimumProbability,
                    iqpResult.probability(state)
                );

        iqpMaximumProbability =
                std::max(
                    iqpMaximumProbability,
                    iqpResult.probability(state)
                );
    }

    check(
        approximatelyEqual(summedProbability(quantumCountingResult), 1.0) &&
        approximatelyEqual(summedProbability(amplitudeEstimationResult), 1.0) &&
        approximatelyEqual(summedProbability(iqpResult), 1.0) &&
        approximatelyEqual(summedProbability(surfaceCodeResult), 1.0) &&
        iqpMaximumProbability - iqpMinimumProbability > 1.0e-3,
        "Counting, estimation, IQP, and surface-code presets stay normalized and IQP is uneven"
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

    const std::vector<ComplexMatrix> extendedGateBatch{
        quantum_sim::gates::sxGate(),
        quantum_sim::gates::sxDaggerGate(),
        quantum_sim::gates::phaseGate(std::numbers::pi / 3.0),
        quantum_sim::gates::uGate(
            std::numbers::pi / 3.0,
            std::numbers::pi / 5.0,
            -std::numbers::pi / 7.0
        ),
        quantum_sim::gates::controlledPhaseGate(std::numbers::pi / 3.0),
        quantum_sim::gates::crxGate(std::numbers::pi / 3.0),
        quantum_sim::gates::cryGate(std::numbers::pi / 3.0),
        quantum_sim::gates::crzGate(std::numbers::pi / 3.0),
        quantum_sim::gates::rxxGate(std::numbers::pi / 3.0),
        quantum_sim::gates::ryyGate(std::numbers::pi / 3.0),
        quantum_sim::gates::rzzGate(std::numbers::pi / 3.0),
        quantum_sim::gates::chGate(),
        quantum_sim::gates::csGate(),
        quantum_sim::gates::csDaggerGate(),
        quantum_sim::gates::ctGate(),
        quantum_sim::gates::ctDaggerGate(),
        quantum_sim::gates::dcxGate(),
        quantum_sim::gates::ecrGate(),
        quantum_sim::gates::squareRootSwapGate(),
        quantum_sim::gates::fSimGate(
            std::numbers::pi / 3.0,
            std::numbers::pi / 5.0
        ),
        quantum_sim::gates::ccxGate(),
        quantum_sim::gates::cSwapGate()
    };

    check(
        std::all_of(
            extendedGateBatch.begin(),
            extendedGateBatch.end(),
            [](const ComplexMatrix &gate) {
                return gate.isUnitary();
            }
        ),
        "Every gate in the extended library is unitary"
    );

    QuantumCircuit parameterizedTwoQubitCircuit{2U};
    parameterizedTwoQubitCircuit.addTwoQubitGate(
        "RXX",
        quantum_sim::gates::rxxGate(std::numbers::pi / 3.0),
        0U,
        1U,
        std::numbers::pi / 3.0
    );

    check(
        parameterizedTwoQubitCircuit.instructionInfo().front().angleRadians.has_value() &&
        approximatelyEqual(
            parameterizedTwoQubitCircuit.instructionInfo().front().angleRadians.value(),
            std::numbers::pi / 3.0
        ),
        "Parameterized two-qubit gates retain their timeline angle"
    );

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
        quantum_sim::gui::notation::formatAngleMeasurement(
            std::numbers::pi / 2.0
        ) == "\xCF\x80/2 (1.571 rad)" &&
        quantum_sim::gui::notation::formatAngleMeasurement(
            0.74 * std::numbers::pi
        ) == "0.74\xCF\x80 (2.325 rad)" &&
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
        quantum_sim::gui::density_volume::Vector3{-4.0F, -0.5F, -4.0F},
        quantum_sim::gui::density_volume::Vector3{4.0F, 8.5F, 4.0F},
        16.0F / 9.0F
    );
    densityCamera.orbit(80.0F, -25.0F);
    densityCamera.pan(12.0F, -6.0F);
    densityCamera.zoom(1.0F);
    densityCamera.update(0.1F);

    const auto userCameraView =
            densityCamera.viewMatrix();

    densityCamera.updateSceneBounds(
        quantum_sim::gui::density_volume::Vector3{-5.0F, -1.0F, -5.0F},
        quantum_sim::gui::density_volume::Vector3{7.0F, 15.0F, 5.0F},
        16.0F / 9.0F
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
        quantum_sim::gui::density_volume::Vector3{-4.0F, -0.5F, -4.0F},
        quantum_sim::gui::density_volume::Vector3{4.0F, 20.5F, 4.0F},
        16.0F / 9.0F
    );

    const auto initialAutomaticView =
            automaticCamera.viewMatrix();

    const float initialAutomaticDistance =
            automaticCamera.orbitDistance();

    automaticCamera.updateSceneBounds(
        quantum_sim::gui::density_volume::Vector3{-4.0F, -0.5F, -4.0F},
        quantum_sim::gui::density_volume::Vector3{4.0F, 20.5F, 4.0F},
        16.0F / 9.0F
    );

    automaticCamera.update(0.1F);

    check(
        std::equal(
            initialAutomaticView.begin(),
            initialAutomaticView.end(),
            automaticCamera.viewMatrix().begin(),
            [](const float left, const float right) {
                return approximatelyEqual(left, right, 1e-6);
            }
        ) &&
        approximatelyEqual(
            automaticCamera.orbitDistance(),
            initialAutomaticDistance,
            1e-6
        ),
        "Untouched playback keeps the complete planned history frame stable"
    );

    quantum_sim::gui::density_volume::CameraController wideViewportCamera;
    wideViewportCamera.frameScene(
        quantum_sim::gui::density_volume::Vector3{-20.0F, -2.0F, -4.0F},
        quantum_sim::gui::density_volume::Vector3{20.0F, 2.0F, 4.0F},
        16.0F / 9.0F
    );

    quantum_sim::gui::density_volume::CameraController narrowViewportCamera;
    narrowViewportCamera.frameScene(
        quantum_sim::gui::density_volume::Vector3{-20.0F, -2.0F, -4.0F},
        quantum_sim::gui::density_volume::Vector3{20.0F, 2.0F, 4.0F},
        0.75F
    );

    check(
        narrowViewportCamera.orbitDistance() >
            wideViewportCamera.orbitDistance(),
        "Density camera fit accounts for narrow viewport aspect"
    );

    if (failures == 0) {
        std::cout << "All tests passed.\n";
    }

    return failures == 0 ? 0 : 1;
}

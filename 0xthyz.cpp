/*
    QubitCanvas ownership guide
    ===========================

    Why this file exists
    --------------------
    This is the front door to the codebase. It is intentionally valid C++ but
    is not added to CMake: its job is to explain where behavior lives before a
    maintainer starts changing it. Headers remain the exact API reference and
    source files remain the implementation; this file supplies the map between
    them.

    Read these rules first
    ----------------------
    1. q0 is the most-significant bit in QuantumRegister. UI density axes may
       bit-reverse display order, but simulation indices never change.
    2. QuantumCircuit stores compact 2x2, 4x4, and 8x8 matrices whenever
       possible. Do not expand a local gate to a 2^n by 2^n matrix.
    3. Density data is calculated on the CPU, converted to GPU instances, then
       rendered by OpenGL into a texture. ImGui only displays that texture.
    4. The render thread never waits for a long trace rebuild.
       SimulationHistoryWorker publishes complete generations atomically.
    5. ProjectFile is the lossless native format. OpenQasmFile is an
       intentionally smaller interchange format and rejects lossy exports.
    6. JetBrains Mono is loaded once by GuiApplication and is the default ImGui
       font. Keep new UI labels compact and technical.

    Runtime in one page
    -------------------
    src/main.cpp
        parseCommandLine() converts capture/testing flags into GuiLaunchOptions.
        main() chooses an initial built-in circuit and starts GuiApplication.

    GuiApplication::run()
        initializes GLFW -> OpenGL 3.3 Core -> GLAD -> DensityVolumeRenderer ->
        Dear ImGui -> JetBrains Mono. Each frame then:

        OS events
          -> queued project/QASM/circuit edits
          -> completed background simulation adoption
          -> debugger playback and layer synchronization
          -> top bar, circuit, density viewport, inspector, gate library
          -> ImGui render
          -> buffer swap

    Circuit edit
        GuiApplication records one EditorSnapshot, mutates QuantumCircuit, and
        asks SimulationHistoryWorker to rebuild the affected suffix.

    Background result
        SimulationHistoryWorker builds DebuggerSession + DensityStack. The GUI
        accepts only the newest request id, so stale work cannot replace a newer
        edit.

    Density render
        DensityModel -> DensityStack -> SceneBuilder -> InstanceScene ->
        Renderer::updateScene() -> instanced VAO/VBO/EBO draws -> HDR/bloom
        framebuffer -> Renderer::colorTexture() -> ImGui::Image().

    Repository root
    ---------------
    CMakeLists.txt
        Declares the C++20 core, GUI, executable, and test targets. Add new
        simulator/persistence sources to qubit_canvas_core. Add ImGui/OpenGL
        sources to qubit_canvas_gui. There must be only one GLAD target.

    README.md
        User-facing behavior, controls, build steps, capture flags, and feature
        overview. Update it when the visible workflow changes.

    tests/QuantumSimTests.cpp
        One executable containing deterministic regression checks. New math,
        serialization, algorithms, camera behavior, or scene layout needs a
        focused check here.

    assets/
        Runtime assets copied beside the executable after build. The application
        expects assets/fonts/JetBrainsMono-Regular.ttf.

    third_party/
        Vendored GLFW, GLAD, and Dear ImGui. Product behavior should not be
        implemented here.

    cmake-build-* and build/
        Generated build trees. Never place source-of-truth code here.

    Math layer: include/quantum_sim/math + src/math
    ------------------------------------------------
    Complex.hpp / Complex.cpp
        Complex stores real and imaginary components.
        real(), imaginary() expose components.
        conjugate(), magnitudeSquared(), magnitude() provide core operations.
        operator+(), operator*(), operator/(), operator+=() implement arithmetic.
        Change this only for primitive complex-number behavior.

    ComplexVector.hpp / ComplexVector.cpp
        ComplexVector owns a checked vector of Complex values.
        size(), at() read it.
        magnitudeSquared(), isNormalized(), normalized() enforce state norms.
        innerProduct() supplies overlaps.
        operator+(), scalar operator*(), tensorProduct() compose vectors.
        QuantumRegister owns one ComplexVector.

    ComplexMatrix.hpp / ComplexMatrix.cpp
        ComplexMatrix owns row-major matrix values.
        rows(), columns(), at() inspect dimensions/data.
        conjugateTranspose(), identity(), isUnitary() validate gates.
        tensorProduct(), matrix-vector operator*(), matrix-matrix operator*()
        implement algebra. Gate construction and circuit validation depend on
        this class.

    Gates: include/quantum_sim/gates/QuantumGates.hpp
           src/gates/QuantumGates.cpp
    --------------------------------------------------
    Every function returns an exact unitary matrix and has no GUI responsibility.

    Fixed one-qubit factories:
        xGate, yGate, zGate, hadamardGate, sGate, sDaggerGate, tGate,
        tDaggerGate, sxGate, sxDaggerGate.

    Parameterized one-qubit factories:
        phaseGate, rxGate, ryGate, rzGate, uGate.

    Compact two-qubit factories:
        cxGate(), cyGate(), czGate(), chGate(), csGate(), csDaggerGate(),
        ctGate(), ctDaggerGate(), controlledPhaseGate(), crxGate(), cryGate(),
        crzGate(), rxxGate(), ryyGate(), rzzGate(), swapGate(), iSwapGate(),
        dcxGate(), ecrGate(), squareRootSwapGate(), fSimGate().

    Compact three-qubit factories:
        ccxGate(), cSwapGate().

    Legacy/full-register factories:
        cxGate(n,...), cyGate(n,...), czGate(n,...), swapGate(n,...), and
        iSwapGate(n,...). Prefer compact matrices for new circuit instructions.

    To add a gate completely:
        1. Add its matrix factory here and a numerical test.
        2. Add its descriptor/matrix tooltip in GateLibraryPanel.cpp.
        3. Add matrix selection in GuiApplication's gate factory helpers.
        4. Add a unique circuit name in GateNotation.cpp/CircuitRenderer.cpp.
        5. Add OpenQASM mapping only if the mapping is lossless and standard.

    Quantum state: include/quantum_sim/quantum + src/quantum
    --------------------------------------------------------
    Qubit.hpp / Qubit.cpp
        MeasurementResult is Zero or One.
        Qubit stores alpha/beta for a single normalized qubit.
        zeroAmplitude(), oneAmplitude(), apply(), probabilityOfZero(),
        probabilityOfOne(), and measure() form its complete behavior.

    QuantumRegister.hpp / QuantumRegister.cpp
        QuantumRegister is the production state-vector type.
        Constructor validates 2^n normalized amplitudes.
        qubitCount(), stateCount(), amplitude(), probability() inspect state.
        applySingleQubitGate(), applyTwoQubitGate(), applyThreeQubitGate()
        walk compact amplitude blocks without full matrix expansion.
        applyGate() handles genuinely register-wide matrices.
        applyReflection() applies I - 2|u><u| in O(2^n).
        measure() collapses the full register.
        measureQubit() collapses one qubit.
        probabilityOfQubitZero/One() calculate marginals.
        basisState() constructs a computational basis register.
        basisStateLabel(), stateInfo(), states() produce display data.
        blockVector() and blochAngles() are single-qubit helpers.
        stateHasQubitOne() is the private MSB-order bit test.

    Analysis: include/quantum_sim/analysis + src/analysis
    -----------------------------------------------------
    StateMetrics.hpp / StateMetrics.cpp
        QubitMetrics holds p(0), p(1), coherence, purity, entropy, and Bloch
        length for one reduced 2x2 state.
        StateMetrics::fidelity() computes |<a|b>|^2.
        StateMetrics::forQubit() traces out all other qubits.
        StateMetrics::forRegister() repeats the reduction for every qubit.
        For a pure full register, entropyBits is q-versus-rest entanglement
        entropy. InspectorPanel displays these exact state-vector metrics.

    Circuit model: include/quantum_sim/circuit + src/circuit
    --------------------------------------------------------
    QuantumCircuit.hpp / QuantumCircuit.cpp
        TraceBuildCancelled lets cooperative background work stop quietly.
        TraceStep pairs one instruction description with its post-gate state.
        CircuitInstructionKind identifies single, two, three, full, reflection.
        CircuitInstructionInfo is lightweight UI/debugger metadata.
        CircuitInstructionSnapshot is the lossless public editor/file copy.

        QuantumCircuit owns a fixed qubit count and private instruction variant.
        addSingleQubitGate(), addTwoQubitGate(), addThreeQubitGate(),
        addFullRegisterGate(), addControlledGate(), addReflection() append.
        insertSingleQubitGate(), insertTwoQubitGate(),
        insertThreeQubitGate(), insertControlledGate() insert at a timeline slot.
        insertInstructionSnapshot() validates and inserts a lossless snapshot.
        replaceInstructionSnapshot() powers safe inline parameter edits.
        removeLastInstruction(), removeInstruction(), moveInstruction() edit.
        instructionCount(), instructionInfo(), instructionSnapshots() inspect.
        execute() returns the final state.
        runShots() samples repeated final states.
        executeWithTrace() builds all post-gate states.
        executeWithTraceFrom() rebuilds only an edited suffix.

        SingleQubitInstruction, TwoQubitInstruction, ThreeQubitInstruction,
        FullRegisterInstruction, and ReflectionInstruction are private storage
        records. Add a new instruction category only when these, execution,
        snapshots, ProjectFile, render metadata, and tests are all updated.

    Algorithms: include/quantum_sim/algorithms + src/algorithms
    -----------------------------------------------------------
    QuantumAlgorithms.hpp / QuantumAlgorithms.cpp
        Each public function returns a ready-to-run QuantumCircuit. Shared
        private helpers in the .cpp build QFT fragments, controlled phases,
        reflections, error-code blocks, and fixed demonstrations.

        Core demos:
        bellStateCircuit, ghzStateCircuit, plusRegisterCircuit,
        quantumFourierTransformCircuit, inverseQuantumFourierTransformCircuit,
        groverCircuit, deutschJozsaCircuit, bernsteinVaziraniCircuit,
        toffoliDemoCircuit, phaseKickbackCircuit, teleportationCircuit,
        scrambleCircuit.

        Algorithm demonstrations:
        simonCircuit, shorPeriodFindingCircuit,
        quantumPhaseEstimationCircuit, vqeAnsatzCircuit, qaoaMaxCutCircuit,
        hhlDemoCircuit, swapTestCircuit, quantumWalkCircuit, bb84DemoCircuit,
        superdenseCodingCircuit.

        State preparation and randomized circuits:
        wStateCircuit, dickeStateCircuit, graphStateCircuit, randomCircuit,
        weightedStatePreparationCircuit.

        Error correction:
        bitFlipCodeCircuit, phaseFlipCodeCircuit, steaneCodeCircuit,
        shorCodeCircuit, fiveQubitCodeCircuit, surfaceCodeStabilizerCircuit.

        Estimation/arithmetic/sampling:
        quantumCountingCircuit, amplitudeEstimationCircuit,
        rippleCarryAdderCircuit, draperAdderCircuit, iqpCircuit.

        Focused rotation demos:
        rxRotationCircuit, ryRotationCircuit, rzRotationCircuit.

        Add a built-in circuit here, add its CircuitPreset mapping and minimum
        qubit rule in GuiApplication, add the page button, command-line capture
        alias when useful, and add a behavior test.

    Debugger: include/quantum_sim/debug + src/debug
    ------------------------------------------------
    DebuggerSnapshot.hpp
        DebuggerSnapshot is a non-owning current-frame view: step numbers,
        optional instruction, before/after states, navigation flags.

    DebuggerSession.hpp / DebuggerSession.cpp
        Owns initial state, instruction metadata, execution trace, current step.
        stepCount(), currentStepNumber(), isAtInitialState(), hasSteps() query.
        currentStep(), currentInstruction(), stateBeforeCurrentStep(),
        initialState(), stepAt(), snapshot() read exact state/history.
        moveNext(), movePrevious(), moveToStep(), moveToStepNumber(), restart()
        navigate without rebuilding.
        rebuild() replaces the complete trace.
        rebuildFrom() preserves an unaffected prefix after a circuit edit.

    InteractiveCircuitDebugger.hpp / .cpp
        gateExplanation() provides console explanations.
        runInteractiveDebugger() is the terminal debugger loop, independent of
        the desktop GUI.

    Persistence and interchange: include/quantum_sim/project + src/project
    ------------------------------------------------------------------------
    ProjectFile.hpp / ProjectFile.cpp
        ProjectDocument owns QuantumCircuit + initial QuantumRegister.
        ProjectFile::save() writes versioned lossless .qcanvas data.
        ProjectFile::load() bounds-checks and reconstructs every value.
        Change projectVersion only with a deliberate migration strategy.

    ProjectWorkspace.hpp / ProjectWorkspace.cpp
        ProjectWorkspace chooses LOCALAPPDATA/QubitCanvas.
        beginSession()/endSession() maintain the crash marker.
        autosavePath(), recoveryAvailable(), discardRecovery() manage recovery.
        recentProjects(), recordRecentProject() maintain the bounded recent list.
        defaultRoot() is the platform path policy.
        It never interprets quantum data; ProjectFile does that.

    SubcircuitLibrary.hpp / SubcircuitLibrary.cpp
        StoredSubcircuit owns name, source size, lossless snapshots.
        canInsertInto() protects operands and register-wide semantics.
        SubcircuitLibrary::save() writes a selected range as .qcanvas.
        loadAll() returns valid blocks in name order and skips damaged entries.
        safeFilename() handles platform-invalid filename characters.

    OpenQasmFile.hpp / OpenQasmFile.cpp
        OpenQasmFile::save() emits OpenQASM 3 + stdgates and rejects loss.
        OpenQasmFile::load() tokenizes semicolon statements, parses pi angles,
        validates operands, constructs exact supported matrices, and returns a
        zero-initialized ProjectDocument.
        Private parser helpers trim/lowercase text, parse numbers/angles,
        split parameters/operands, enforce arity, map gate names, and format
        rational pi expressions.

    GUI shell: include/quantum_sim/gui + src/gui
    ------------------------------------------------
    GuiApplication.hpp / GuiApplication.cpp
        GuiLaunchOptions controls hidden visual regression starts.
        CircuitPreset is the complete built-in catalog.
        CanvasMode chooses Layer Stack or Floor Field.
        EditorSnapshot is one undo/redo checkpoint.
        InstructionAngleEdit is a deferred safe gate replacement; recordUndo
        marks the first live preview in a slider gesture as its one checkpoint.

        GuiApplication owns every long-lived subsystem and shared UI state.
        run() owns initialization, frame order, cleanup, and hidden captures.
        createSingleQubitGateMatrix(), createTwoQubitGateMatrix(),
        createThreeQubitGateMatrix() translate palette names to gate factories.
        applyQueuedCircuitEdits() is the single circuit-mutation funnel.
        undoLastCircuitEdit()/redoLastCircuitEdit() restore EditorSnapshots.

        Simulation:
        rebuildDebuggerAfterCircuitEdit(), rebuildDensityVolume(), and
        adoptCompletedSimulationHistory() schedule/adopt worker generations.

        Files:
        saveProject(), applyQueuedProjectOpen(), applyQueuedQasmOpen(),
        autosaveProjectIfDue(), drawRecoveryPrompt(), drawExchangeMenu().

        Shared selection:
        synchronizeDensityLayer(), selectDensityLayer(),
        recordEditorForUndo(), resetEditorTransientState().

        Frame/UI:
        configureStyle(), pushApplicationFont(), popApplicationFont(),
        handleGlobalShortcuts(), drawBackdrop(),
        drawSelectedGateAngleEditor(), drawDensityVolumeViewport(), drawTopBar(),
        drawAlgorithmScripts(), drawReusableSubcircuits(), drawBottomStatus(),
        applyPlayback(). The angle editor is a gate-anchored window outside the
        circuit layout; it previews at 25 Hz and never shifts gate hit regions.

        Circuit creation:
        loadPreset(), applyQueuedPreset(), createBlankCircuit(),
        clearCircuitInstructions(), createPresetCircuit(),
        minimumQubitCount(), sampleCurrentState().

        Placement:
        armGatePlacement() and cancelGatePlacement() keep palette, circuit
        previews, and Escape behavior synchronized.

        Keep GuiApplication as orchestration. Numerical algorithms belong in
        core modules; OpenGL implementation belongs in Renderer; detailed panel
        layouts belong in panel classes.

    SimulationHistoryWorker.hpp / .cpp
        SimulationHistoryResult is one publishable generation.
        SimulationHistoryWorker starts one persistent jthread.
        request() cancels/replaces pending work and returns a generation id.
        takeCompleted() transfers the newest finished result.
        busy() supports UI disabling/status.
        cancel() stops current and queued work.
        run() waits, builds a full or suffix trace/density history, and publishes
        only if its request is still newest.

    NativeFileDialog.hpp / .cpp
        openProject(), saveProject(), openQasm(), saveQasm(),
        saveCircuitSvg(), saveStateCsv(), saveDensityCsv() are platform adapters.
        Windows uses GetOpenFileNameW/GetSaveFileNameW. Other platforms return
        nullopt until an adapter is implemented.

    ExportFile.hpp / ExportFile.cpp
        saveCircuitSvg() writes a standalone dark JetBrains-Mono diagram.
        saveStateCsv() exports every exact amplitude/probability/phase.
        saveDensityCsv() exports displayed density cells and bucket metadata.

    QuantumNotation.hpp / .cpp
        formatReal(), formatComplex(), formatRadians(),
        formatAngleMeasurement(), formatPolarAmplitude() are the canonical
        numerical display formatters. Do not duplicate pi/radical formatting.

    GateNotation.hpp / .cpp
        displayName() owns typographic names (dagger/root symbols).
        exchangeBadge() owns SWAP-family center badges.
        circuitLabel() chooses the unique compact circuit identity.

    Panels: include/quantum_sim/gui/panels + src/gui/panels
    ------------------------------------------------------
    GateLibraryStyle.hpp
        GateLibraryStyle contains palette dimensions/colors only.

    GateLibraryPanel.hpp / GateLibraryPanel.cpp
        GateDescriptor pairs internal name and explanation.
        GateParameters holds theta/phi/lambda placement values.
        draw() lays out the current gate page and parameter controls.
        selectedGate(), consumeSelectedGate(), selectGate(), clearSelection()
        implement one-way palette selection.
        gateParameters(), setPage(), page(), style(), setStyle() expose state.
        drawGateButton() provides drag source + tooltip.
        drawGateMatrixTooltip() draws the documented matrix.
        drawGateCategory(), drawParameterizedControls(), drawPageControls()
        keep the palette compact.

    InspectorPanel.hpp / InspectorPanel.cpp
        draw() composes the right panel and focusQubit() synchronizes sections.
        drawQuantumState() chooses the exact selected debugger state.
        drawLayerStack() owns the synchronized 2D density heatmap and hover.
        drawProbabilities(), drawAmplitudes(), drawBlochInformation() inspect it.
        drawStateMetrics() shows fidelity, local purity, and entanglement entropy.
        drawDebuggerControls(), moveToPreviousInstruction(),
        moveToNextInstruction(), restartDebugger(), navigation helpers own
        Inspector-local navigation feedback.
        The density layer and probabilities stay immediately visible. Analysis,
        amplitudes, Bloch data, and navigation use collapsing headers, and none
        creates a nested scrolling child; the Inspector window owns scrolling.
        focusQubit() updates only the Bloch target and deliberately preserves
        the user's current Inspector scroll and expansion state.

    Rendering: include/quantum_sim/gui/rendering + src/gui/rendering
    ----------------------------------------------------------------
    CircuitStyle.hpp
        CircuitStyle is the centralized circuit geometry/color tuning table.

    CircuitRenderer.hpp / CircuitRenderer.cpp
        Placement records: SingleQubitPlacement, ControlledPlacement,
        ThreeQubitPlacement, InstructionMove. InstructionScreenAnchor is the
        screen-space attachment point for controls that belong to one gate.
        draw() renders wires, gates, step zero, previews, selections, tooltips,
        drag/drop, hidden horizontal scrolling, and queued interactions.
        Selection API: selectedInstructionIndex/Indices(), selectInstruction(),
        selectInstructions(), clearSelection(), private selection helpers.
        selectedInstructionScreenAnchor() exposes the selected gate center from
        the latest render without exposing ImGui types to GuiApplication state.
        draw() publishes that anchor after click/drag handling so a new
        selection can open its attached editor during the same frame.
        Placement API: consumeCompleted*(), completedControlledPlacement(),
        hasPendingControlQubit(), placementOperandCount(), cancelPlacement(),
        pendingInsertionIndex(), continuePlacementAfter().
        Navigation/edit API: consumeStepJumpRequest(),
        consumeInstructionMoveRequest(), requestFocusStep().
        View API: zoomIn(), zoomOut(), fitToView(), viewZoom(),
        isFittingToView(), style(), setStyle().
        drawGate() is the common boxed-gate primitive. Multi-wire/exchange
        geometry is handled in draw() because it depends on all wire positions.

    BlochSphereStyle.hpp
        BlochSphereStyle contains the ImGui sphere geometry/colors.

    BlochSphereRenderer.hpp / BlochSphereRenderer.cpp
        draw() owns the widget.
        drawSphereGeometry() draws grid, axes, and projected vector.
        drawPinnedDetails(), drawHoverTooltip(), drawCoordinates() report data.
        handleCanvasInteraction() toggles pinned details.
        calculateCanvasSize/HorizontalOffset/Radius/SphereSegments() size it.
        chooseSphereOutlineColor(), style(), setStyle() control presentation.

    DensityVolumeMath.hpp / .cpp
        Vector3 and Matrix4 are the renderer's dependency-free math types.
        vector +, -, scalar *, dot(), cross(), length(), normalize() operate.
        identityMatrix(), lookAtMatrix(), perspectiveMatrix() build matrices.

    DensityVolumeColorMap.hpp / .cpp
        Color is RGB.
        magnitudeColor() is the Inferno magnitude palette used by 2D and 3D.
        phaseColor() is the optional phase wheel. Change shared density colors
        here first so heatmap and voxels continue to correspond.

    DensityVolumeModel.hpp / .cpp
        DensityBin describes one large-register bucket.
        DensityCell stores row/column, complex value, magnitude, intensity,
        phase.
        DensityLayer stores one exact or bucketed matrix; cellAt() reads it.
        DensityStack stores initial + post-gate layers and a fingerprint.
        DensityModel::build() creates complete history.
        DensityModel::rebuildFrom() preserves an unaffected prefix.
        DensityModel::difference() computes selected-reference complex deltas.

    DensityVolumeScene.hpp / .cpp
        VisualizationMode chooses stack/floor.
        SceneViewOptions carries isolate/comparison filters.
        Selection is a stable picked cell id.
        VoxelVertex/VoxelGeometry define shared indexed meshes.
        VoxelInstance is one compact GPU instance.
        InstanceScene owns instances, picking records, layer ranges, bounds.
        VoxelGeometryBuilder::buildRoundedCube() creates Layer Stack cubes.
        buildRoundedTopColumn() creates Floor Field bars.
        SceneBuilder::build() lays matrices out in 3D without changing data.
        Change cube proportions/layout here, not in density numerical code.

    DensityVolumeCameraController.hpp / .cpp
        CameraController implements Blender-style orbit/pan/zoom.
        frameScene() chooses the initial three-quarter fit.
        updateSceneBounds() follows new horizontal history without discarding a
        user camera.
        update() applies damping, orbit(), pan(), zoom() handle input,
        reset() returns to fitted view.
        viewMatrix(), projectionMatrix() feed OpenGL.
        isFramed(), orbitDistance() expose camera status.

    DensityVolumeRenderer.hpp / .cpp
        Renderer owns all OpenGL objects and no quantum simulation rules.
        initialize() compiles shaders and creates VAO/VBO/EBO/FBO resources.
        updateScene() builds/uploads instances only when model identity changes.
        render() performs depth-tested instanced scene, picking, bloom, composite.
        pick() reads integer cell ids.
        colorTexture() is displayed by ImGui.
        framingMinimum/Maximum() feed camera fitting.
        shutdown() releases GL resources while context exists.
        createScenePrograms(), createPostProcessPrograms(),
        createVoxelBuffers(), createGhostBuffers(), createGridBuffers(),
        createPostProcessVertexArray(), uploadInstances(), resizeFramebuffer(),
        renderPostProcess() split GPU setup and passes.

    Console output: include/quantum_sim/visualization + src/visualization
    --------------------------------------------------------------------
    ConsoleVisualizer.hpp / ConsoleVisualizer.cpp
        printProbabilityBars(), printShotBars(), printExecutionTrace(),
        printCircuitDiagram(), printAmplitudes(), printStateComparison(),
        printBlochVector(), printAsciiBlochSphere() provide terminal output.
        They consume core models and must not affect GUI state.

    Practical change routes
    -----------------------
    "Change the Inferno colors"
        DensityVolumeColorMap.cpp. Verify both Inspector heatmap and Renderer.

    "Change cube roundness or Floor Field tops"
        DensityVolumeScene.cpp geometry builders, then renderer shader only if
        normal/material behavior must change.

    "Change Layer Stack spacing or growth axis"
        SceneBuilder::build() and camera framing tests.

    "Add an Inspector analysis"
        Put exact math in analysis/, test it, then render it in InspectorPanel.

    "Add a project-level editor feature"
        Queue intent in GuiApplication, mutate only in applyQueuedCircuitEdits(),
        record undo first, then request a suffix rebuild.

    "Change save data"
        CircuitInstructionSnapshot + ProjectFile + version/round-trip tests.

    "Add an export"
        ExportFile or OpenQasmFile for format logic, NativeFileDialog for a
        destination, drawExchangeMenu for discovery, and a content test.

    "Diagnose lag after editing or changing algorithms"
        Inspect SimulationHistoryWorker cancellation, circuit trace complexity,
        DensityModel bucketing, SceneBuilder instance counts, and whether
        Renderer::updateScene is re-uploading despite an unchanged fingerprint.

    "Diagnose a wrong matrix pattern"
        Verify QuantumRegister's MSB convention, DensityModel's outer product,
        and the documented bit-reversed display ordering before changing UI.
*/

namespace quantum_sim::architecture_guide {
    // A compiled reference would encourage dependencies on documentation.
    // Keeping one harmless symbol makes this a valid translation unit while
    // CMake deliberately leaves it outside every target.
    [[maybe_unused]] constexpr const char *purpose =
            "Read 0xthyz.cpp before choosing an ownership boundary.";
}

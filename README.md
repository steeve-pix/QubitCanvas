# QubitCanvas

QubitCanvas is an interactive quantum-circuit simulator and debugger written
from scratch in C++20. It combines a state-vector simulation core with a
JetBrains Mono desktop interface for building circuits, stepping through their
execution, and inspecting how the quantum state changes after every gate.

The simulator does not depend on an external quantum-computing framework.
GLFW, Dear ImGui, GLAD, and OpenGL provide the desktop and rendering layers.

## Highlights

- Editable multi-qubit circuit with blank-register creation, repeated
  placement, palette drag-and-drop, insertion, drag reordering,
  multi-selection, clipboard duplication, whole-circuit Clear, undo, redo,
  and batch deletion.
- Step-by-step debugger with play, pause, restart, scrub, and sampling controls.
- Synchronized circuit, density-matrix, probability, and Bloch-sphere views.
- Hover documentation with matrices for every gate in the gate library.
- Native `.qcanvas` project save/open with `Ctrl+S` and `Ctrl+O`.
- Timed recovery copies, interrupted-session recovery, and recent projects.
- Gate-anchored live angle editing and persistent reusable circuit blocks.
- Fidelity, reduced purity, and qubit-versus-rest entanglement entropy.
- OpenQASM 3 import/export, standalone circuit SVG, and state/density CSV export.
- Exact quantum notation for familiar fractions, radicals, complex values, and
  rational multiples of `π`, with compact decimal fallback.
- JetBrains Mono typography throughout the interface.
- Raw OpenGL 3.3 Core density visualization rendered through an off-screen
  linear-HDR framebuffer, restrained bloom composite, and Dear ImGui texture.
- Automated regression coverage for simulation, algorithms, debugger state,
  and density-volume conversion.

## Density Volume

The Density Volume panel converts each debugger state into the projector
`ρ = |ψ⟩⟨ψ|`. Numerically visible cells become opaque instances of one indexed,
smoothly rounded cube with a soft density-core material. The roundover keeps a
constant world-space radius when Floor Field instances grow vertically, so
tall values remain cuboids instead of stretching into capsules. Near-zero cells
are omitted from the solid pass. Exact matrices up to 16x16 retain a separate
faint edge-only ghost pass so every layer stays recognizable; bucketed
large-register matrices skip those ghosts to preserve clarity and performance.

Magnitude follows a tone-shaped Inferno ramp. Colors are converted from sRGB
to linear space before amplitude-dependent glow is applied, then a half-size
bloom pass and linear-to-sRGB output preserve the warm gold, orange, magenta,
and violet range without heavy tone-map compression. Restrained face lighting,
perspective framing, depth testing, and a procedural ground grid keep cube
tops, sides, matrix depth, and history separation readable.
Floor Field uses a dedicated higher-detail column mesh with a square base,
straight walls, and a smooth fixed-radius rounded top.
Layer Stack reserves a tight frame for the algorithm's complete density
history as soon as the preset loads. Its projected bounds, camera angle,
perspective field of view, and viewport aspect determine the orbit distance,
so playback fills the prepared view without camera drift or incremental
zooming. Resizing recalculates the fit, while manual orbit, pan, or zoom keeps
ownership of the live camera until reset.

Two synchronized layouts are available:

- **Layer Stack** places complete vertical Y-Z density matrices along the X
  axis, growing the history horizontally while revealing pre-uploaded solid
  and ghost instance ranges through the selected debugger step.
- **Floor Field** shows the selected density matrix as one square X-Z grid and
  maps `|ρ|` linearly to voxel height.

The inspector heatmap always follows the selected 3D layer and outlines the
cell under the pointer. Hovering either view reports row, column, magnitude,
intensity, phase in radians, real component, and imaginary component.
`Isolate layer` removes neighboring history from the 3D scene without changing
the selected state. `Compare` renders the complex cell-wise difference
`Δρ = ρ(selected) - ρ(reference)` and adds delta magnitude, real, and imaginary
values to voxel hover details. The Inspector continues to show the unchanged
selected density matrix.

| Input | Action |
| --- | --- |
| Left drag | Orbit |
| Right drag | Pan |
| Shift + left drag | Pan |
| Mouse wheel | Zoom |
| `R` | Reset the camera |
| Click a voxel | Select its density layer |
| Double-click a circuit step or gate | Jump the debugger to that step |

Circuit step `0` shows an identity gate on every qubit and represents the
untouched initial register before any instruction executes.
The application opens there, and every built-in circuit selection returns
there in a paused state before playback begins.

## Gate Library

| Category | Gates |
| --- | --- |
| Core single-qubit | `H`, `X`, `Y`, `Z`, `S`, `Sdg`, `T`, `Tdg`, `SX`, `SXdg` |
| Parameterized single-qubit | `P`, `U`, `Rx`, `Ry`, `Rz` |
| Core two-qubit | `CX`, `CY`, `CZ`, `SWAP`, `iSWAP` |
| Parameterized controlled | `CP`, `CRx`, `CRy`, `CRz` |
| Interaction rotations | `RXX`, `RYY`, `RZZ` |
| Advanced controlled | `CH`, `CS`, `CSdg`, `CT`, `CTdg` |
| Native and exchange | `DCX`, `ECR`, `sqrtSWAP`, `fSim` |
| Three-qubit | `CCX`, `CSWAP` |

The gate catalog uses compact previous/page/next controls. Core operations stay
on page one, parameterized and interaction gates stay on page two, and
advanced hardware and three-qubit operations stay on page three.
Angles are stored in radians and displayed as exact or decimal multiples of
`π`. The universal `U` gate exposes independent `θ`, `φ`, and `λ` controls.
`fSim` exposes independent `θ` exchange and `φ` conditional-phase controls.
Detailed controls and hover readouts place the decimal radian value beside the
π notation, for example `π/2 (1.571 rad)`. Hover a gate button to see its name,
purpose, and unitary matrix; zero-valued entries are intentionally subdued so
the matrix structure scans quickly.

`Escape` cancels an active gate placement. `Space` toggles playback whenever a
text field is not accepting input.

Gate placement stays armed after a successful insertion so the same operation
can be applied repeatedly without returning to the library. The `H`, `X`, `Y`,
`Z`, `S`, and `T` keys arm their matching gates directly. Single-qubit targets
and both endpoints of controlled gates receive an on-canvas preview before the
operation is committed.
Gate buttons can also be dragged directly from the library to a circuit wire.
Dropping a single-qubit gate commits it immediately; dropping a multi-qubit
gate chooses its first operand and leaves the remaining operand previews active.

The circuit toolbar provides fit and zoom controls, an authoring-focused layout
that temporarily hides the visualizers, and an optional Follow edits mode that
shows the state after each manual edit. Existing gates can be dragged to a new
timeline position or moved one step with the adjacent arrow controls. Manual
edits rebuild only the affected debugger and density-history suffix.
Trace and density reconstruction run on a persistent background worker.
Choosing another preset or editing again cancels superseded work between
instructions and density layers; only the newest completed generation can
replace the visible debugger state. Playback pauses while a build is active,
and the last complete visualization remains stable until the replacement is
ready.
Circuit gate boxes widen within their timeline slot for long operation names,
and hovering a placed gate always reveals its complete name.
Plain click selects one gate, `Ctrl`+click toggles independent gates, and
`Shift`+click selects a contiguous timeline range. The selection row provides
Copy, Paste, and Duplicate commands with `Ctrl+C`, `Ctrl+V`, and `Ctrl+D`;
Delete removes the entire selection as one undoable edit.
In the normal workspace, the circuit and Density Volume always divide the
center column equally. Larger registers scroll inside the circuit panel instead
of reducing the 3D viewport; Focus editor remains available for a full-height
circuit view.

The step scrubber above the canvas provides direct navigation through long
circuits. `Ctrl` + mouse wheel changes timeline zoom, while `Shift` + mouse
wheel pans the timeline horizontally. Placement accepts the complete visible
wire band rather than requiring a click on one small marker. Ten-qubit layouts
compress wire spacing to keep the full register readable, and the inspector
scrolls its probability table to the qubit most recently selected or edited.

`New blank circuit` creates an empty register using the qubit slider. The
`Clear circuit` button removes all current gates without changing the register.
Both actions, algorithm replacement, and register-size changes store complete
Undo snapshots. Loading a preset asks for confirmation when the current circuit
contains custom edits.

The top bar opens and saves versioned `.qcanvas` project documents through the
native file picker. Projects retain the complete normalized initial register,
compact gate matrices, operands, exact stored angles, full-register operations,
and reflection axes. The current project name carries an asterisk while edits
have not been saved.

Unsaved edits are periodically written to a separate recovery document in the
local QubitCanvas workspace. After an interrupted or deliberately unsaved
session, the next launch offers to recover that document as an unnamed dirty
project; named files are never overwritten by recovery. The compact `I/O` menu
contains recent projects, OpenQASM 3 import/export, a standalone dark-theme SVG
circuit export, and exact state-vector or selected-density-layer CSV exports.
OpenQASM export fails clearly when a custom full-register operation cannot be
represented without changing its meaning.

A selected parameterized gate with one retained angle exposes a stationary
theta editor above that gate. Slider changes preview live without an Apply step,
and one complete drag remains one undoable operation. The editor is an overlay,
so selecting it never moves the circuit or interrupts gate double-click
navigation. Any selected gate range can be named with `Save block`; persistent
blocks appear in the compact Reusable blocks chooser and can be inserted into
compatible registers.

The Inspector State analysis section computes directly from the exact complex
state vector. It reports fidelity to the preceding step, mean single-qubit
purity, and base-2 von Neumann entropy for every qubit. For the simulator's pure
global states, `S(q:rest)` is the entanglement entropy between that qubit and
the remainder of the register.

The Inspector uses one outer scroll surface. Its synchronized density layer and
qubit probabilities are immediately visible; analysis, amplitudes, Bloch
sphere, and navigation expand only when needed.

`SWAP` is drawn as two crossed exchange paths rather than endpoint crosses.
Compact `SW`, `iSW`, and `√SW` badges distinguish exchange variants without
obscuring their crossed paths. Long exchange routes open clean overpass gaps
at unrelated wires, while `CSWAP` clears its complete target ports and stops
the control stem at the center badge. Placed-gate hover cards identify every
operand and explain the behavior of controlled, exchange, and interaction gates.
One canonical notation table keeps every built-in circuit identity unique.
Controlled targets retain their complete names (`CX`, `CRx`, `CS†`, `CT†`),
interaction boxes use `RXX`/`RYY`/`RZZ`, and three-qubit operations use
`CCX`/`CSW`. The same notation is used during placement and execution.
`CCX` and `CSWAP` use a three-click placement flow and render as genuine
three-wire circuit operations. Their compact 8x8 matrices execute directly
against eight-amplitude blocks without expansion to the full register.

## Built-in Circuits

- Bell state
- GHZ state
- Register-wide `|+⟩ⁿ` state
- Quantum Fourier Transform
- Inverse Quantum Fourier Transform
- Grover search
- Deutsch-Jozsa
- Bernstein-Vazirani
- Decomposed Toffoli
- Phase kickback
- Coherent teleportation
- Mixed-gate scramble
- Simon hidden-period demonstration
- Compiled Shor order finding for `a = 4 mod 15`
- Quantum Phase Estimation
- Fixed VQE ansatz
- One-layer Max-Cut QAOA
- Fixed 2x2 HHL demonstration
- SWAP test
- Coined quantum walk
- BB84 basis demonstration
- Superdense coding
- W state
- Dicke state with two excitations
- Linear graph/cluster state
- Reproducible random circuit with uneven output probabilities
- Weighted state preparation
- Three-qubit bit-flip error correction
- Seven-qubit Steane logical-zero encoding
- Nine-qubit Shor-code encoding
- Three-qubit phase-flip error correction
- Five-qubit perfect-code logical-zero preparation
- Quantum counting with a three-qubit counting register
- Quantum amplitude estimation for a fixed 30% probability
- Two-bit ripple-carry addition
- Two-bit Draper Fourier addition
- Register-wide IQP sampling
- Ten-qubit surface-code stabilizer demonstration

The register slider controls the size of the next blank circuit or preset from
1 to 10 qubits; it does not silently resize the circuit currently being edited.
Presets below their minimum working-register size are disabled.
The fixed-height catalog uses previous and next page controls, so the expanded
algorithm collection does not make the Gate Library taller. Fixed
demonstrations use their leading qubits and leave any additional register
qubits in the initial state.
Grover is register-wide: it marks the all-one basis state and uses the
near-optimal number of oracle/diffusion iterations for the selected register.

## Code Ownership

Start with [`0xthyz.cpp`](0xthyz.cpp) before modifying an unfamiliar subsystem.
It maps every production file, its classes and function families, the runtime
data flow, numerical conventions, rendering stages, and practical "change this
when..." routes. Header comments remain the precise API contract; the guide
explains how those contracts fit together.

## Build

### Requirements

- CMake 3.20 or newer
- A C++20 compiler
- Git
- OpenGL 3.3 Core support

Initialize the GLFW and Dear ImGui submodules after cloning:

```powershell
git submodule update --init --recursive
```

Configure and build:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --target qubit_canvas qubit_canvas_tests
```

Run the automated tests:

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

Run the application from the build directory so the copied font assets are
available:

```powershell
.\build\qubit_canvas.exe
```

The executable also supports hidden framebuffer captures for unattended visual
regression checks:

```powershell
.\build\qubit_canvas.exe --preset random --qubits 6 --algorithm-page 3 `
  --final-step --floor-field --compare-layer 0 `
  --capture C:\captures\random-final.ppm
```

Supported capture presets are `qft`, `grover`, `w`, `dicke`, `graph`, `random`,
`weighted`, `bit-flip`, `steane`, `shor-code`, `phase-flip`,
`five-qubit-code`, `quantum-counting`, `amplitude-estimation`, `ripple-adder`,
`draper-adder`, `iqp`, `surface-code`, and the `swap-routing` visual-regression
scene (with six qubits). `--gate-page 3 --gate fSim` captures the advanced gate
catalog with `fSim` armed. `--select-gate 4` selects the fourth circuit
instruction, which is useful for capturing the stationary angle editor on a
parameterized gate.

For a Visual Studio multi-configuration generator, the executable may instead
be located at `build\Debug\qubit_canvas.exe`.

## Numerical Behavior

The simulation core stores the complete state vector, so its memory and
execution cost scale exponentially with qubit count. Single-, two-, and
three-qubit instructions remain compact as 2x2, 4x4, and 8x8 matrices and are
applied directly to amplitude pairs, quartets, or octets. They therefore use
O(2^n) execution memory instead of expanding every gate into an O(4^n)
full-register matrix.
State preparation and register-wide Grover use compact Householder reflections
evaluated directly against the state vector, also in O(2^n) memory.
The explicit full-register gate API remains available for custom unitaries
that genuinely require it.

Density matrices with at most 16 basis states are rendered exactly. Larger
registers are grouped into a maximum 16x16 display using
probability-preserving row and column buckets. This keeps the inspector and 3D
views readable without changing the simulator's underlying quantum state.

Near-zero density values remain as restrained dark cubes, preserving the
matrix architecture without producing bright visual noise.

Layer histories use `glDrawElementsInstanced()`: the rounded cube geometry is
uploaded once, and a ten-qubit 16x16 visualization stores one compact instance
per displayed cell. Playback changes the submitted instance count instead of
rebuilding geometry, which keeps the camera stable and algorithm switching
responsive.

## Project Layout

```text
assets/                 JetBrains Mono runtime asset
include/quantum_sim/    Public simulator and GUI headers
src/                    Core simulator, algorithms, debugger, and GUI sources
src/gui/rendering/      OpenGL density volume and visualization renderers
tests/                  Automated regression tests
third_party/            GLFW, Dear ImGui, and GLAD
```

The main targets are:

- `qubit_canvas_core`: complex math, gates, registers, circuits, algorithms,
  and debugger state.
- `qubit_canvas_gui`: Dear ImGui panels and OpenGL renderers.
- `qubit_canvas`: desktop executable.
- `qubit_canvas_tests`: regression test executable.

## Development Workflow

Feature work is kept on focused `feature/*` branches and merged with
`--no-ff` into `develop`. Release integration then merges `develop` into
`release/0.1.0`.

QubitCanvas is under active development.

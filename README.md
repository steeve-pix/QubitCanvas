# QubitCanvas

QubitCanvas is an interactive quantum-circuit simulator and debugger written
from scratch in C++20. It combines a state-vector simulation core with a
JetBrains Mono desktop interface for building circuits, stepping through their
execution, and inspecting how the quantum state changes after every gate.

The simulator does not depend on an external quantum-computing framework.
GLFW, Dear ImGui, GLAD, and OpenGL provide the desktop and rendering layers.

## Highlights

- Editable multi-qubit circuit with repeated placement, insertion, drag
  reordering, selection, undo, redo, and deletion.
- Step-by-step debugger with play, pause, restart, scrub, and sampling controls.
- Synchronized circuit, density-matrix, probability, and Bloch-sphere views.
- Hover documentation with matrices for every gate in the gate library.
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

Two synchronized layouts are available:

- **Layer Stack** places complete Y-Z density matrices along the X axis and
  reveals pre-uploaded solid and ghost instance ranges through the selected
  debugger step.
- **Floor Field** shows the selected density matrix as one square X-Z grid and
  maps `|ρ|` linearly to voxel height.

The inspector heatmap always follows the selected 3D layer and outlines the
cell under the pointer. Hovering either view reports row, column, magnitude,
intensity, phase in radians, real component, and imaginary component.

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
| Single-qubit | `H`, `X`, `Y`, `Z`, `S`, `Sdg`, `T`, `Tdg` |
| Rotations | `Rx`, `Ry`, `Rz` |
| Controlled and exchange | `CX`, `CY`, `CZ`, `SWAP`, `iSWAP` |

Rotation angles are stored in radians and displayed as exact or decimal
multiples of `π`. Detailed angle controls and hover readouts place the decimal
radian value beside that notation, for example `π/2 (1.571 rad)`. Hover a gate
button to see its name, purpose, and unitary matrix; zero-valued entries are
intentionally subdued so the matrix structure scans quickly.

`Escape` cancels an active gate placement. `Space` toggles playback whenever a
text field is not accepting input.

Gate placement stays armed after a successful insertion so the same operation
can be applied repeatedly without returning to the library. The `H`, `X`, `Y`,
`Z`, `S`, and `T` keys arm their matching gates directly. Single-qubit targets
and both endpoints of controlled gates receive an on-canvas preview before the
operation is committed.

The circuit toolbar provides fit and zoom controls, an authoring-focused layout
that temporarily hides the visualizers, and an optional Follow edits mode that
shows the state after each manual edit. Existing gates can be dragged to a new
timeline position or moved one step with the adjacent arrow controls. Manual
edits rebuild only the affected debugger and density-history suffix.

`SWAP` is drawn as two crossed exchange paths rather than endpoint crosses.
`iSWAP` uses the same path geometry with a centered `(i)` marker.

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

The algorithm register slider controls the circuit size for every preset from
1 to 10 qubits. Presets that require two or three working qubits are disabled
below their minimum. Fixed demonstrations use their leading qubits and leave
any additional register qubits in the initial state.

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

For a Visual Studio multi-configuration generator, the executable may instead
be located at `build\Debug\qubit_canvas.exe`.

## Numerical Behavior

The simulation core stores the complete state vector, so its memory and
execution cost scale exponentially with qubit count. Single-qubit and
two-qubit instructions remain compact as 2x2 and 4x4 matrices and are applied
directly to amplitude pairs or quartets. They therefore use O(2^n) execution
memory instead of expanding every gate into an O(4^n) full-register matrix.
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

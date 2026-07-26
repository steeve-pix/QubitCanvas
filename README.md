# QubitCanvas

QubitCanvas is an interactive quantum-circuit simulator and debugger written
from scratch in C++20. It combines a state-vector simulation core with a
JetBrains Mono desktop interface for building circuits, stepping through their
execution, and inspecting how the quantum state changes after every gate.

The simulator does not depend on an external quantum-computing framework.
GLFW, Dear ImGui, GLAD, and OpenGL provide the desktop and rendering layers.

## Highlights

- Editable multi-qubit circuit with gate selection, undo, redo, and deletion.
- Step-by-step debugger with play, pause, restart, scrub, and sampling controls.
- Synchronized circuit, density-matrix, probability, and Bloch-sphere views.
- Hover documentation with matrices for every gate in the gate library.
- Exact quantum notation for familiar fractions, radicals, complex values, and
  rational multiples of `π`, with compact decimal fallback.
- JetBrains Mono typography throughout the interface.
- Raw OpenGL 3.3 Core density visualization rendered through an off-screen
  HDR framebuffer, bloom composite, and Dear ImGui texture display.
- Automated regression coverage for simulation, algorithms, debugger state,
  and density-volume conversion.

## Density Volume

The Density Volume panel converts each debugger state into
`ρ = |ψ><ψ|` and renders every matrix cell as an opaque instance of one shared
indexed rounded cube. A compact per-instance record supplies position, scale,
warm amplitude color, emissive strength, layer identity, and picking identity.
Soft ambient, warm directional key, cool fill, contact darkening, HDR bloom,
and ACES-style tone mapping give the volume readable top and side faces without
transparent geometry. A procedural perspective grid anchors the structure.

Two synchronized layouts are available:

- **Layer Stack** places complete Y-Z density matrices along the X axis and
  reveals their pre-uploaded instances through the selected debugger step.
- **Floor Field** shows the selected density matrix as one square X-Z grid.

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

## Gate Library

| Category | Gates |
| --- | --- |
| Single-qubit | `H`, `X`, `Y`, `Z`, `S`, `Sdg`, `T`, `Tdg` |
| Rotations | `Rx`, `Ry`, `Rz` |
| Controlled and exchange | `CX`, `CY`, `CZ`, `SWAP`, `iSWAP` |

Rotation angles are entered and displayed in radians. Hover a gate button to
see its name, purpose, and unitary matrix; zero-valued entries are intentionally
subdued so the matrix structure scans quickly.

`Escape` cancels an active gate placement. `Space` toggles playback whenever a
text field is not accepting input.

## Built-in Circuits

- Bell state
- GHZ state
- Register-wide `|+>` state
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

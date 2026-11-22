# NoctisLaby Engine

**NoctisLaby Engine (NLE)** is an independent, modular, native 2D and 3D game engine.

The project is being developed toward broad feature parity with mature open-source engines. Its architecture is informed by studying established engine implementations, including Godot. NoctisLaby is a separate project and is not a Godot fork.

## Technology

- **C++** — engine runtime and native systems.
- **C** — stable low-level interfaces and platform integration where appropriate.
- **C#** — managed gameplay scripting through a native interop boundary.
- **CMake** — cross-platform build configuration.

## Repository map

| Directory | Responsibility |
| --- | --- |
| `core/` | Fundamental types, memory, objects, logging and utilities |
| `main/` | Engine startup, lifecycle and runtime orchestration |
| `servers/` | Rendering, physics, audio, input and other engine services |
| `scene/` | Scene tree, nodes, resources and gameplay-facing systems |
| `scripting/` | Script abstraction, C API and C# integration |
| `editor/` | Editor application and development tools |
| `drivers/` | Concrete hardware and library backends |
| `platform/` | Operating-system-specific implementations |
| `modules/` | Optional engine features |
| `tests/` | Unit, integration, regression and performance tests |
| `thirdparty/` | External dependencies and license records |
| `docs/` | Architecture and development documentation |

Directories and systems will be introduced with working implementations rather than empty placeholders.

## Build

Requirements:
- CMake 3.24 or newer
- A C++20 compiler
- A C17-compatible C compiler

```sh
cmake -S . -B build
cmake --build build --config Release
```

Run the runtime:

```sh
./build/bin/nle_runtime
```

On multi-configuration generators, the executable may be under `build/bin/Release`.

## Development status

The project is in its foundation phase. The initial target is a clean, reproducible native build and a tested core. Rendering, physics, scene management, scripting, editor tooling and platform export will be implemented incrementally.

Feature parity is measured by implemented behavior, integration and tests—not by directory names.

## Licensing and upstream code

Code reused from other projects must retain its applicable copyright and license notices. Third-party components may have licenses different from the engine's own license; each dependency must be reviewed and recorded before inclusion.

## Commit conventions

Commits use Conventional Commits, for example:

- `build: establish cross-platform engine foundation`
- `feat(core): add object lifetime management`
- `test(core): cover reference counting behavior`
- `docs: document resource ownership model`

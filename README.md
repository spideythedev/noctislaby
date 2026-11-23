# NoctisLaby Engine

**NoctisLaby Engine (NLE)** is an independent, modular, native 2D and 3D game engine.

The project is being developed toward broad feature parity with mature open-source engines. Its architecture is informed by studying established engine implementations, including Godot. NoctisLaby is a separate project and is not a Godot fork.

## Technology

- **C++20** — native engine runtime and core systems.
- **C17** — stable low-level interfaces and platform integration.
- **C#** — planned first-class managed gameplay scripting through a versioned native interop boundary.
- **CMake** — cross-platform build configuration.

## Repository map

| Directory | Responsibility |
| --- | --- |
| `core/` | Fundamental types, memory, objects, math, resources and utilities |
| `main/` | Engine startup, lifecycle and runtime orchestration |
| `servers/` | Rendering, physics, audio, input and other engine services |
| `scene/` | Node hierarchy, scene tree, 2D/3D nodes and gameplay-facing systems |
| `scripting/` | Script abstraction, C API and future managed runtime integration |
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
ctest --test-dir build -C Release --output-on-failure
```

Run the runtime:

```sh
./build/bin/nle_runtime
```

On multi-configuration generators, the executable may be under `build/bin/Release`.

## Implemented foundation

The current native foundation includes:

- CMake build configuration, generated version metadata and compiler warning policies.
- A shared core library and a C-compatible, versioned engine metadata API.
- Typed error codes, thread-safe logging and aligned allocation with live memory statistics.
- Object identity, object registry, intrusive reference counting, typed references and runtime class registration.
- Thread-safe signals and interned string identifiers.
- A tagged Variant value type.
- 2D/3D vectors, rectangles, AABBs, planes, colors, quaternions, bases and affine transforms.
- Binary file access, a persistent versioned resource UID registry, canonical `res://` and `user://` paths, resource caching, concurrent load deduplication, and pluggable format loader/saver dispatch.
- A deterministic `.nleproject` configuration reader/writer with typed accessors and transactional parsing.\n- Project creation, validation, version checks, default settings and canonical main-scene paths.\n- A thread-safe input event queue with frame-based pressed/just-pressed/just-released state, mouse position tracking and configurable digital action maps.\n- An initial deterministic 2D physics server with dynamic/static AABB bodies, gravity integration, collision filtering and positional/impulse resolution.
- Node ownership, scene-tree lifecycle, a fixed-timestep main loop with bounded physics catch-up, frame/physics callbacks, and transformable Node2D/Node3D types.
- A backend-neutral rendering server, generation-safe resource identifiers, a null rendering device with buffer and texture resource operations, and a render graph that validates resource hazards and compiles deterministic pass order.
- Automated core, resource, scene and C-ABI tests.

The foundation is intentionally not presented as full engine parity.

## Current gaps

GPU rendering backends and draw submission, physics simulation, audio drivers, animation systems, navigation, networking, the full C# host/bindings, editor tooling, asset import pipelines and platform export systems remain to be implemented.

Feature parity is measured by working behavior, integration and tests—not by directory names.

## Licensing and upstream code

Code reused from other projects must retain its applicable copyright and license notices. Third-party components may have licenses different from the engine's own license; each dependency must be reviewed and recorded before inclusion. The current foundation code is independently authored; Godot source is being studied as an architectural and implementation reference.

## Commit conventions

Commits use Conventional Commits, for example:

- `build: establish cross-platform engine foundation`
- `feat(core): add object lifetime management`
- `test(core): cover reference counting behavior`
- `docs: document resource ownership model`

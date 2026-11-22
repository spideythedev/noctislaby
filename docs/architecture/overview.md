# NoctisLaby Engine Architecture

## Design goals

NoctisLaby is a native, modular engine with a C++ runtime, a carefully versioned C ABI for low-level integration, and C# gameplay scripting.

The architecture separates engine-facing interfaces from platform and hardware implementations. Scene and gameplay code should depend on stable engine abstractions, not directly on a graphics API or operating-system API.

## Major layers

- **Core:** foundational types, memory, object lifetime, reflection, serialization and utilities.
- **Main:** startup, engine instance, lifecycle and runtime orchestration.
- **Servers:** rendering, physics, audio, input, navigation, networking, text and display services.
- **Scene:** node lifecycle, scene tree, resources, 2D/3D objects, animation and GUI.
- **Scripting:** language abstraction, native C interface, .NET hosting and managed bindings.
- **Editor:** project management, scene editing, inspection, import, debugging and export workflows.
- **Drivers and platform:** concrete graphics/audio/input backends and operating-system integration.
- **Modules and extensions:** optional engine capabilities and external native integrations.

## Current implementation

The current foundation provides:
- CMake-based C17/C++20 configuration and generated engine version metadata.
- Core memory allocation, object identity, object registry, reference counting, runtime class factories and signals.
- String interning, a tagged Variant and foundational 2D/3D math types.
- Binary file access, resource metadata, a versioned persistent UID registry, resource caching and pluggable format loader/saver dispatch.
- Node ownership, scene-tree lifecycle, a fixed-timestep main loop with bounded catch-up, frame/physics callbacks and hierarchical spatial transforms.
- A rendering server, generation-checked resource IDs, null buffer backend and dependency-aware render graph.
- A versioned C ABI metadata library and cross-platform build/test workflow.

This is a functional early engine foundation, not a complete editor, game runtime or claim of Godot feature parity.

## Architectural constraints

1. Keep platform-specific code behind platform or driver interfaces.
2. Keep editor-only dependencies out of exported game runtimes.
3. Treat resource identity and object identity as separate concerns.
4. Define ownership and lifetime rules before exposing objects across C# interop.
5. Keep the C ABI stable and versioned; do not expose C++ implementation details as ABI.
6. Add tests alongside each subsystem.
7. Record upstream provenance and preserve license notices for all reused code.

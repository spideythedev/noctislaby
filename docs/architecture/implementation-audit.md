# NoctisLaby implementation audit

Audit baseline: `main` at commit `1bc77414f8fe7dbf08bf14d8738cfbb747164222` (2026-11-22).

This is a source-level audit of the current NLE implementation. It distinguishes working foundations from production gaps and records correctness issues addressed during the review.

## Repository inventory

The audited tree contains 110 entries at the audit baseline. Implemented code is concentrated in:

- `core/`: errors, logging, aligned memory, object IDs/registry, basic class factories, intrusive RefCounted, signals, StringName, Variant, math, file access and resource dispatch/cache.
- `scene/`: owned Node hierarchy, lifecycle callbacks, SceneTree frame dispatch, Node2D and Node3D transforms.
- `servers/rendering/`: backend-neutral device/server API, null buffer backend, generation-safe RIDs and a CPU-side dependency compiler for declared render passes.
- `scripting/c_api/`: a small versioned C API exposing engine metadata.
- `main/`: a minimal runtime bootstrap.
- `tests/`: one native test executable and a C ABI smoke test.

The README's directory map is aspirational in several places: editor, drivers, platform, modules and thirdparty are not implemented directories yet.

## Correctness findings addressed

1. Signal callbacks were stored in an unordered map, so callback invocation order was unspecified. Connection-ID-ordered storage now provides deterministic dispatch, with a regression test.
2. Rendering device metadata returned a string view whose lifetime was tied to the device after the server lock was released. The returned metadata now owns its device name.
3. Render graph declarations accepted unknown enum values. Invalid access values are now rejected.
4. Render graph allocation paths could throw through an ErrorCode-based API. Allocation and container-length failures now map to OutOfMemory.
5. Render graph topological ordering scanned every pass repeatedly. A min-priority ready queue now preserves stable order while reducing scheduling work to O((V + E) log V), excluding dependency construction.
6. Regression tests cover deterministic signal order and invalid render-graph access declarations.

## Remaining correctness and design risks

### Object lifetime and threading
- ObjectDB lookup returns a raw pointer after releasing its registry mutex. This is an identity lookup, not a lifetime-pinning handle; cross-thread destruction/use requires a stronger ownership or access protocol.
- RefCounted requires callers to acquire references only from a live object. Weak references, safe weak locking, destruction diagnostics and explicit ownership contracts are not implemented.
- Signal callbacks are copied before invocation. Disconnecting during an emission affects subsequent emissions, not the already-copied callback snapshot. Callback exceptions currently propagate to the emitter.
- Runtime class enumeration currently uses an unordered registry and should be made deterministic before it becomes a public reflection contract.

### Resource pipeline
- Resource UIDs are value types only; there is no UID database, path remapping or persistent UID index.
- Resource loader/saver dispatch is extension-based and plugin-oriented, but no built-in serialized scene/resource format exists.
- There is no dependency graph, import database, asynchronous loading, cancellation, load progress or cache policy matrix.
- Cache keys are exact path strings; canonicalization and alias handling remain unspecified.

### Scene runtime
- Node ownership and basic lifecycle callbacks exist, but there is no deferred deletion queue, process priority, groups, input propagation, notifications, pause tree, viewport/canvas, scene serialization or packed-scene instantiation.
- SceneTree currently exposes manual frame and physics dispatch. There is no main-loop scheduler, fixed-step accumulator, time scaling or platform event loop.
- Scene mutation and object lifetime are not designed for arbitrary concurrent mutation.

### Rendering
- The current render graph is a CPU-side pass dependency planner, not a GPU command graph.
- Resource declarations identify whole logical resources only; no texture subresources or buffer ranges are tracked.
- There are no texture/sampler/shader/pipeline abstractions, command buffers, queues, fences, barriers, swapchain, render targets, draw submission or GPU allocator.
- No hardware backend exists. The null backend is for validation and headless testing only.

### Other major subsystems
- Physics, audio, input, display, navigation, animation, networking, text shaping, XR and import/export pipelines are absent.
- C# runtime hosting, generated managed bindings, debugger integration and hot reload are absent.
- There is no editor application, project manager, inspector, asset browser, scene editor, profiler or exporter.
- There is no dependency manifest or third-party license inventory because no third-party runtime libraries have been introduced.

## Release gates

### 1.0.0-stable must not be tagged until
- A documented supported platform/compiler matrix is continuously green.
- A real project format, resource serialization and a minimal runnable sample game exist.
- The runtime has an actual frame scheduler and platform event/input path.
- At least one hardware rendering backend renders tested 2D and 3D scenes.
- Resource/object lifetime, scene lifecycle and error handling have dedicated regression tests.
- Packaging, install layout, versioned C ABI and release artifacts are reproducible.
- Documentation accurately lists implemented features and known limitations.

### 1.7.0-stable must not be tagged until
- Physics, audio, input, navigation and animation have integrated implementations.
- The editor supports project creation, scene editing, inspection and asset import.
- Managed C# scripting and native extension APIs have working examples and tests.
- Rendering includes resource synchronization, pipeline management and production diagnostics.
- Export/deployment workflows and platform support are validated.
- Compatibility, performance and regression suites cover the published feature set.

Version numbers are release labels, not substitutes for these acceptance gates.

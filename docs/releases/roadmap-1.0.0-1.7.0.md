# NoctisLaby Engine — 1.0.0 and 1.7.0 release programme

## Release policy

NLE follows capability-based releases. A version is published only when its acceptance gates pass. A target date or version number never overrides an unresolved correctness, security, licensing or packaging blocker.

## 1.0.0-stable — usable native engine foundation

### Runtime and core
- [ ] Stable object ownership, weak references and lifetime-safe object access.
- [ ] Reflection metadata for methods, properties, signals and enums.
- [ ] Expanded Variant and conversion/operator coverage.
- [x] Persistent versioned resource UID registry with integrity checks.\n- [ ] Canonical resource paths, project-wide UID remapping and versioned resource serialization.
- [ ] Async resource loading, dependency tracking and cache policies.
- [x] Deterministic `.nleproject` configuration parser/writer with transactional load.\n- [x] Project directory creation/opening, descriptor validation and version checks.\n- [ ] Reproducible sample project format and end-to-end launch workflow.

### Scene and runtime
- [x] Fixed-timestep MainLoop with bounded catch-up, frame clamping and time scaling.\n- [ ] Platform event loop, high-resolution clock and frame pacing.
- [ ] Deferred deletion, process priorities and pause handling.\n- [x] Thread-safe input event queue and frame-scoped digital input state.\n- [ ] Platform input drivers, event pumping and configurable action maps.
- [ ] Scene/resource serialization and packed scene instantiation.
- [ ] Tested 2D and 3D scene lifecycle and transform behavior.

### Rendering
- [ ] Hardware backend with tested 2D and 3D rendering.
- [x] Texture resource descriptions, formats, usage flags and null-backend storage/readback.\n- [ ] Hardware textures, samplers, shaders, pipelines, render targets and swapchain.
- [ ] Command buffers, submission, fences and resource lifetime rules.
- [ ] Render graph integration with backend resource transitions.

### Delivery
- [ ] Supported platform/compiler matrix.
- [ ] Installable runtime and SDK artifacts.
- [ ] Sample project and end-to-end tests.
- [ ] C ABI versioning and compatibility policy.
- [ ] API documentation, migration notes and release checklist.

## 1.7.0-stable — expanded production engine

- [ ] Physics 2D and 3D with selectable backends.
- [ ] Audio mixer, buses, effects, streaming and platform drivers.
- [ ] Input and display servers with platform implementations.
- [ ] Animation, navigation, networking and text systems.
- [ ] C# runtime host, generated bindings, debugger and hot reload.
- [ ] Native extension SDK with generated stable C ABI.
- [ ] Editor project manager, scene editor, inspector and asset browser.
- [ ] Import pipeline, debugger, profiler and export/deployment pipeline.
- [ ] Performance, compatibility, soak and regression test suites.

## Engineering sequence

1. Close core correctness and ownership risks.
2. Complete the resource and serialization model.
3. Build runtime scheduling and scene execution.
4. Deliver a real hardware rendering path.
5. Add platform display and input.
6. Implement physics and audio services.
7. Add scripting and extension boundaries.
8. Build the editor and asset pipeline.
9. Harden packaging, CI, compatibility and documentation.
10. Run release candidates against the acceptance gates.

## Status

This is a roadmap, not a claim that either release is ready. Work is tracked through implementation commits, tests and CI evidence.

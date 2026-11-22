# Rendering architecture

## Layer boundaries

NoctisLaby separates gameplay-facing scene objects from graphics API details:

```text
Scene and rendering resources
            |
      RenderingServer
            |
      RenderingDevice
       /    |     \
    Vulkan  D3D12  Metal
            |
       GPU driver
```

The current implementation includes the server/device contracts and a null backend. The null backend is useful for headless execution, deterministic resource tests and future server integration. It does not rasterize geometry or submit GPU work.

## Resource identifiers

A `RID` contains a 32-bit slot index and a 32-bit generation. A device validates both before resolving a resource. When a slot is released, its generation advances before it can be reused. This prevents stale handles from silently resolving to unrelated resources.

Slots at generation exhaustion are retired rather than wrapped back to an old generation.

## Device contract

The initial `RenderingDevice` contract provides:

- Explicit initialization and shutdown.
- Backend identity and API version metadata.
- Buffer allocation with a usage mask and optional initial bytes.
- Bounded buffer updates and readback.
- Explicit resource release.

The server owns the selected device and serializes calls through its mutex. The device also protects its own resource table, so it remains safe when used directly by tests or other controlled callers.

## Null backend

`NullRenderingDevice` stores buffer bytes in host memory. It validates resource state, bounds, stale identifiers and allocation failures. It is the reference backend for the server contract, not a substitute for a hardware renderer.

## Planned GPU implementation

The next rendering milestones are:

1. Define device capabilities, limits, formats and synchronization primitives.
2. Add command buffers and submission/fence semantics.
3. Add buffers, textures, samplers and shader modules to the device API.
4. Implement Vulkan as the first hardware backend.
5. Build shader compilation, material and pipeline caches.
6. Add render targets, framebuffers, render graph and scene submission.
7. Add Direct3D 12 and Metal after the API contract is stable.

Graphics dependencies must be evaluated and their licenses recorded before they are vendored.


## Render graph

The render graph is a backend-neutral frame-planning layer. Passes declare resource reads and writes; compilation derives execution dependencies and returns a stable topological order.

- Imported resources may be read before a graph pass writes them.
- Transient resources must be written before they are read.
- A write depends on the previous writer and all readers since that writer, preventing write-after-read hazards.
- Read/write access requires an initialized resource and becomes the new writer.
- Explicit pass dependencies support ordering that is not expressed through resource access.
- Duplicate resource declarations in one pass, invalid identifiers, self-dependencies and dependency cycles are rejected.

The current graph compiles scheduling metadata only. It does not submit GPU commands, allocate physical GPU resources, insert API barriers, or perform transient-resource aliasing. Those responsibilities belong to later backend and execution layers.

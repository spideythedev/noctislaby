# Godot parity implementation map

This document tracks the source areas reviewed and the corresponding NoctisLaby work. The paths below identify upstream reference points; they do not imply that NoctisLaby is a fork or that upstream files have been copied.

## Core source references

| Godot source area | NoctisLaby target | Current state |
| --- | --- | --- |
| `core/object/object.h/.cpp` | `core/object/object.h/.cpp`, `object_db.*`, `object_id.h` | Basic identity and registry implemented; notifications, properties and method binding remain |
| `core/object/ref_counted.h` | `core/object/ref_counted.h` | Intrusive ownership and typed Ref handles implemented; weak references and diagnostics remain |
| `core/object/class_db.h` | `core/object/class_db.*` | Basic registration, inheritance metadata and factories implemented; full reflection remains |
| `core/string/string_name.h` | `core/string/string_name.*` | Thread-safe string interning implemented |
| `core/os/memory.h` | `core/memory/memory.*` | Aligned allocation, reallocation and basic live statistics implemented |
| `core/variant/variant.h` | `core/variant/variant.*` | Initial tagged value set implemented; full engine type system and conversion matrix remain |
| `core/io/resource.h` | `core/io/resource.*`, `resource_uid.h` | Resource identity and metadata implemented |
| Resource loading and caching | `core/io/resource_cache.*`, `resource_loader.*`, `resource_saver.*` | Cache and pluggable format dispatch implemented; built-in formats and dependency tracking remain |
| Core math types | `core/math/` | Initial vector, geometry, color, quaternion and transform primitives implemented |

Upstream files reviewed as architectural references include:

- [Godot Object](https://github.com/godotengine/godot/blob/master/core/object/object.h)
- [Godot RefCounted](https://github.com/godotengine/godot/blob/master/core/object/ref_counted.h)
- [Godot ClassDB](https://github.com/godotengine/godot/blob/master/core/object/class_db.h)
- [Godot StringName](https://github.com/godotengine/godot/blob/master/core/string/string_name.h)
- [Godot Memory](https://github.com/godotengine/godot/blob/master/core/os/memory.h)
- [Godot Variant](https://github.com/godotengine/godot/blob/master/core/variant/variant.h)
- [Godot Resource](https://github.com/godotengine/godot/blob/master/core/io/resource.h)

## Scene source references

| Godot source area | NoctisLaby target | Current state |
| --- | --- | --- |
| `scene/main/node.h/.cpp` | `scene/main/node.*` | Owned child hierarchy, names, paths, lifecycle and callbacks implemented |
| `scene/main/scene_tree.h/.cpp` | `scene/main/scene_tree.*` | Root management, enter/ready/exit, process and physics dispatch, deferred deletion and stale-ID validation implemented; process priorities and pause modes remain |
| 2D/3D spatial nodes | `scene/2d/node_2d.*`, `scene/3d/node_3d.*` | Local and hierarchical transforms implemented |

## Not yet mapped to implementation

These areas still require source-level study and an explicit implementation plan:

- Server architecture and RenderingDevice abstraction.
- Rendering storage, render graph, shader compilation and backend drivers.
- Physics servers, collision spaces, shape resources and physics backends.
- Audio server, buses, effects, streaming and platform audio drivers.
- Input, display, text shaping, navigation and networking servers.
- Full scene resource serialization, import pipeline and animation system.
- C# runtime hosting, generated bindings, debugger integration and hot reload.
- Editor application, inspector, scene editor, profiler and editor plugins.
- Platform export templates, build configurations, extension APIs and release tooling.

## Porting and provenance rules

1. Study the upstream implementation and its dependencies before choosing a port boundary.
2. Record the exact upstream path, revision, copyright holder and license for reused code.
3. Preserve required notices in copied or substantially adapted files.
4. Audit bundled dependencies separately; the engine's license does not automatically replace their licenses.
5. Keep upstream-derived changes isolated enough to review and update.
6. Do not mark a subsystem complete until its behavior, integration and tests are in place.

# AI Agent Rules for KojeomGameEngine

This document defines the rules and guidelines that AI agents must follow when working on the KojeomGameEngine project.

## Project Overview

KojeomGameEngine is a C++ game engine built with DirectX 11, featuring a WPF-based editor. The project follows a modular architecture with clear separation between core engine, graphics, and utility components. The engine supports Forward and Deferred rendering, PBR materials, skeletal animation with state machines, physics, audio, and a full UI system.

## Solution Structure

- **Solution:** `KojeomEngine.sln` (Visual Studio 2022, C++17, x64)
- **Core Engine:** `Engine/` (Static Library, .lib)
- **Editor Bridge:** `Editor/EngineInterop/` (C++ DLL for C# interop)
- **Editor UI:** `Editor/KojeomEditor/` (C# WPF, .NET 8.0)
- **Samples:** `samples/` (16 sample projects: BasicRendering, Lighting, PBR, PostProcessing, Terrain, Water, Sky, Particles, SkeletalMesh, Gameplay, Physics, UI, Layout, AnimationStateMachine, LOD, DebugRendering)

## Engine Module Structure

```
Engine/
├── Core/               # KEngine - Main engine class, Win32 window, main loop, subsystem ownership
├── Graphics/           # Rendering pipeline and all visual subsystems
│   ├── GraphicsDevice.h/cpp      # DirectX 11 device, swap chain, render targets
│   ├── Renderer.h/cpp            # Central rendering orchestrator
│   ├── Camera.h/cpp              # Perspective/orthographic camera
│   ├── Shader.h/cpp              # HLSL shader compilation and management
│   ├── Mesh.h/cpp                # Mesh with vertex/index/constant buffers
│   ├── Material.h/cpp            # PBR material (7 texture slots, presets)
│   ├── Texture.h/cpp             # Texture loading and manager
│   ├── Light.h                   # Directional, Point, Spot light structures
│   ├── Shadow/                   # Shadow maps, cascaded shadow maps
│   ├── Deferred/                 # G-Buffer, deferred renderer
│   ├── PostProcess/              # HDR, lens effects (bloom), auto exposure, DOF, motion blur
│   ├── SSAO/                     # Screen-space ambient occlusion
│   ├── SSR/                      # Screen-space reflections
│   ├── TAA/                      # Temporal anti-aliasing
│   ├── SSGI/                     # Screen-space global illumination
│   ├── Sky/                      # Procedural sky rendering
│   ├── Volumetric/               # Volumetric fog
│   ├── Water/                    # Water rendering
│   ├── Terrain/                  # Terrain rendering
│   ├── Culling/                  # Frustum culling, GPU occlusion culling
│   ├── CommandBuffer/            # Deferred command-based rendering
│   ├── Instanced/                # GPU instanced rendering
│   ├── IBL/                      # Image-based lighting (irradiance, prefiltered env, BRDF LUT)
│   ├── LOD/                      # LOD generation and system
│   ├── Particle/                 # Particle system emitter
│   ├── Performance/              # GPU timer, frame stats
│   └── Debug/                    # Debug renderer
├── Input/              # Keyboard, mouse, raw input, action mapping
├── Audio/              # XAudio2-based audio, 3D sound
├── Physics/            # Rigid body, physics world, collision detection
├── Scene/              # Actor-Component system, scene management
├── Assets/             # Static mesh, skeletal mesh, skeleton, animation, model loader (Assimp)
├── Serialization/      # Binary archive, JSON archive (custom parser)
├── UI/                 # Canvas-based UI system (text, button, image, slider, checkbox, layouts)
├── DebugUI/            # ImGui-based debug overlay
└── Utils/              # Common.h, Logger.h, Math.h
```

## Editor Structure

```
Editor/
├── EngineInterop/      # C++ DLL exposing flat C API (extern "C") for P/Invoke
│   ├── EngineAPI.h     # ~42 exported functions for engine operations
│   └── EngineAPI.cpp   # Implementation wrapping C++ engine classes
└── KojeomEditor/       # C# WPF editor (.NET 8.0)
    ├── Services/       # EngineInterop P/Invoke wrapper, UndoRedoService
    ├── ViewModels/     # MainViewModel, SceneViewModel, PropertiesViewModel, ComponentViewModel
    └── Views/          # ViewportControl, SceneHierarchy, PropertiesPanel, MaterialEditor, ContentBrowser
```

## Code Style Guidelines

### Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Classes | PascalCase with 'K' prefix | `KRenderer`, `KGraphicsDevice`, `KActor` |
| Structs | PascalCase with 'F' prefix | `FRenderObject`, `FDirectionalLight`, `FVertex` |
| Enums | PascalCase with 'E' prefix | `EKeyCode`, `EColliderType`, `ERenderPath` |
| Functions/Methods | PascalCase | `Initialize()`, `BeginFrame()`, `CreateCubeMesh()` |
| Variables | camelCase with descriptive names | `graphicsDevice`, `currentCamera` |
| Member Variables | camelCase with 'b' prefix for booleans | `bInFrame`, `bDebugLayerEnabled` |
| Constants | PascalCase in namespace | `Colors::CornflowerBlue`, `EngineConstants::DefaultFOV` |
| Namespaces | PascalCase | `KojeomEngine::KDebugUI` |

### File Organization

- Header files use `#pragma once`
- Include order: Project includes -> External includes (D3D11, DirectXMath) -> Standard library
- Source files paired: `ClassName.h` / `ClassName.cpp`

### Header File Structure

```cpp
#pragma once

#include "../Utils/Common.h"

#include <d3d11.h>
#include <memory>
#include <vector>

class KClassName
{
public:
    KClassName() = default;
    ~KClassName() = default;

    KClassName(const KClassName&) = delete;
    KClassName& operator=(const KClassName&) = delete;

    HRESULT Initialize();
    void Cleanup();

private:
    ComPtr<ID3D11Device> Device;
    bool bInitialized = false;
};
```

## DirectX 11 Specific Rules

### Resource Management

1. **Use ComPtr** for all COM objects: `ComPtr<ID3D11Device> Device;`
2. **Check HRESULT** return values on all D3D11 calls
3. **Resource creation order**: Device -> Context -> SwapChain -> RenderTargetView -> DepthStencilView
4. **Use Shader Model 5.0** (`vs_5_0`, `ps_5_0`)

### Constant Buffer Alignment

All constant buffers must be 16-byte aligned. Use `static_assert` to validate sizes. Pad struct members as needed.

### Shader Register Conventions

- `b0`: Transform buffer (per-object)
- `b1`: Light buffer (per-frame)
- `b2+`: Material/Custom buffers
- `t0`: Diffuse/albedo texture
- `t1+`: Normal, metallic, roughness, AO, emissive, height textures

## Architecture Patterns

### Entity-Component System
- `KActor` owns `KActorComponent`s via `GetComponent<T>()`
- Components: `KStaticMeshComponent`, `KSkeletalMeshComponent`
- `KScene` manages actors with parent-child hierarchy

### Singleton Subsystems
- `KEngine` (global instance via `GetInstance()`)
- `KInputManager` (global instance)
- `KAudioManager` (global instance)
- `KDebugUI` (global instance, `KojeomEngine::KDebugUI`)

### C#/C++ Interop
- `EngineInterop.dll` exposes flat C API (`extern "C"`, `__declspec(dllexport)`)
- C# consumes via P/Invoke (`DllImport`)
- Preprocessor: `ENGINEAPI_EXPORTS` controls dllexport/dllimport

### Serialization
- Binary archive (`KBinaryArchive`) for scene/mesh/skeleton data
- JSON archive (`KJsonArchive`) with custom DOM parser

## Error Handling

1. **Use LOG macros**: `LOG_INFO()`, `LOG_WARNING()`, `LOG_ERROR()`, `LOG_HRESULT_ERROR()`
2. **Return HRESULT** for functions that can fail
3. **Validate pointers** at function entry

## Memory Management

1. **Use smart pointers** for owning resources (`std::shared_ptr`, `std::unique_ptr`)
2. **Use ComPtr** for all COM objects
3. **Delete copy constructor/assignment** for resource-owning classes
4. **Prefer stack allocation** for small, short-lived objects

## Performance Guidelines

1. Minimize D3D11 state changes (sort by shader, then texture)
2. Use GPU instancing for repeated objects
3. Use Map/Unmap for frequently updated data, UpdateSubresource for rarely updated
4. Use frustum and occlusion culling
5. Profile with GPU timer (`KGPUTimer`)

## Documentation Rules

- All documentation in Markdown format under `docs/`
- Rules and guidelines: `docs/rules/`
- Technical documentation: `docs/` subdirectories
- Code comments: Doxygen-style for public APIs
- File headers: brief `@file` and `@brief` descriptions

## Git Commit Format

```
[Category] Brief description

Detailed description if necessary.

- List of changes
- Another change
```

Categories: `[Core]`, `[Graphics]`, `[Input]`, `[Audio]`, `[Physics]`, `[Scene]`, `[UI]`, `[Editor]`, `[Docs]`, `[Build]`, `[Refactor]`, `[Fix]`, `[Feature]`

## AI Agent Specific Instructions

1. **Always read existing code** before making changes to understand the patterns used
2. **Follow existing naming conventions** strictly (K prefix for classes, F for structs, E for enums)
3. **Maintain consistency** with the existing architecture and module boundaries
4. **Document new features** in the appropriate docs folder
5. **Test changes** before marking work as complete
6. **Keep changes focused** - one feature/fix per commit
7. **Update all related files** when modifying APIs (headers, interop, editor bindings)

## Prohibited Actions

1. **Do not** modify third-party library code
2. **Do not** commit debug/test code to main branch
3. **Do not** break existing API without updating all usages (including EngineInterop and C# editor)
4. **Do not** use raw pointers for owning resources
5. **Do not** ignore compiler warnings
6. **Do not** change the build configuration without updating documentation
7. **Do not** add new dependencies without explicit approval

# AI Agent Rules for KojeomGameEngine

This document defines the rules and guidelines that AI agents must follow when working on the KojeomGameEngine project.

## Project Overview

KojeomGameEngine is a C++ game engine built with DirectX 11, featuring a WPF-based editor. The project follows a modular architecture with clear separation between core engine, graphics, and utility components. The engine supports Forward and Deferred rendering, PBR materials, skeletal animation with state machines, physics, audio, and a full UI system.

## Solution Structure

- **Solution:** `KojeomEngine.sln` (Visual Studio 2022, C++17, x64)
- **Core Engine:** `Engine/` (Static Library, .lib)
- **Editor Bridge:** `Editor/EngineInterop/` (C++ DLL for C# interop)
- **Editor UI:** `Editor/KojeomEditor/` (C# WPF, .NET 8.0)
- **Samples:** `samples/` (15 sample projects: BasicRendering, Lighting, PBR, PostProcessing, Terrain, Water, Sky, Particles, SkeletalMesh, Gameplay, Physics, UI, AnimationStateMachine, LOD, DebugRendering)

## Engine Module Structure

```
Engine/
├── Core/               # KEngine - Main engine class, Win32 window, main loop, subsystem ownership, ISubsystem interface, KSubsystemRegistry
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
│   ├── Sky/                      # Procedural atmospheric sky rendering
│   ├── Volumetric/               # Volumetric fog
│   ├── Water/                    # Water rendering with reflection/refraction
│   ├── Terrain/                  # Terrain rendering with LOD and splat-map texturing
│   ├── Culling/                  # Frustum culling, GPU occlusion culling
│   ├── CommandBuffer/            # Deferred command-based rendering with sort keys
│   ├── Instanced/                # GPU instanced rendering
│   ├── IBL/                      # Image-based lighting (irradiance, prefiltered env, BRDF LUT)
│   ├── LOD/                      # LOD generation (quadric error metrics) and runtime system
│   ├── Particle/                 # GPU-accelerated particle system emitter
│   ├── Performance/              # GPU timer, frame stats
│   └── Debug/                    # Debug renderer
├── Input/              # Keyboard, mouse, raw input, action mapping, input callbacks
├── Audio/              # XAudio2-based audio, 3D sound, volume channels
├── Physics/            # Rigid body, physics world, collision detection, raycast
├── Scene/              # Actor-Component system, scene management, parent-child hierarchy
├── Assets/             # Static mesh (LOD), skeletal mesh, skeleton, animation, animation state machine, model loader (Assimp + fallbacks)
├── Serialization/      # Binary archive, JSON archive (custom DOM parser)
├── UI/                 # Canvas-based UI system (element, panel, button, text, image, slider, checkbox, layouts, font)
├── DebugUI/            # ImGui-based debug overlay with panel registration
└── Utils/              # Common.h, Logger.h, Math.h
```

## Editor Structure

```
Editor/
├── EngineInterop/      # C++ DLL exposing flat C API (extern "C") for P/Invoke
│   ├── EngineAPI.h     # 107 exported functions for engine operations
│   └── EngineAPI.cpp   # Implementation wrapping C++ engine classes via FEngineWrapper
└── KojeomEditor/       # C# WPF editor (.NET 8.0)
    ├── Services/       # EngineInterop P/Invoke wrapper (1,111 lines, 100 DllImport), UndoRedoService
    ├── ViewModels/     # MainViewModel, SceneViewModel, PropertiesViewModel, ComponentViewModel
    └── Views/          # ViewportControl, SceneHierarchy, PropertiesPanel, MaterialEditor, RendererSettings, ContentBrowser
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
| Parameters | camelCase with 'In' prefix | `InGraphicsDevice`, `InMesh`, `InName` |
| Type Aliases | PascalCase | `ActorPtr`, `ComponentPtr`, `ComPtr<T>` |
| Template Params | PascalCase | `T`, `FuncType` |

### File Organization

- Header files use `#pragma once`
- Include order: `#include "../Utils/Common.h"` first, then project headers, then external headers (D3D11, DirectXMath), then standard library
- Use custom type aliases from `Engine/Utils/Common.h` (`uint8`, `uint32`, `int32`, etc.) instead of `<cstdint>` types directly
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
5. **Use Map/Unmap** for frequently updated buffers, **UpdateSubresource** for rarely updated

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
- **Engine components** (registered in `EComponentType`):
  - `KStaticMeshComponent` -- wraps `KStaticMesh` + `KMaterial` + `KShader`
  - `KSkeletalMeshComponent` -- wraps `KSkeletalMesh` + `KSkeleton` + `KAnimationInstance`
  - `KTerrainComponent` -- wraps `KTerrain` + `KHeightMap`
  - `KWaterComponent` -- wraps `KWater`
- `KScene` manages actors with parent-child hierarchy
- Camera and lighting are managed by the renderer directly, not as actor components

### Singletons
- `KEngine` (global instance via `GetInstance()`)
- `KInputManager` (global instance)
- `KAudioManager` (global instance)
- `KDebugUI` (global instance, `KojeomEngine::KDebugUI`)

### Subsystem Interface
- `ISubsystem` -- base interface for all engine modules (Initialize, Tick, Shutdown)
- `KSubsystemRegistry` -- owns and manages subsystems with type-based registration and ordered lifecycle
- Registered subsystems: `KAudioSubsystem`, `KPhysicsSubsystem`

### C#/C++ Interop
- `EngineInterop.dll` exposes 107 flat C functions (`extern "C"`, `__declspec(dllexport)`)
- C# consumes via P/Invoke (`DllImport`, 100 DllImport declarations in `EngineInterop.cs`)
- Internal `FEngineWrapper` class manages engine instance, model loader, and debug renderer
- String returns use `thread_local std::string` buffers
- Preprocessor: `ENGINEAPI_EXPORTS` controls dllexport/dllimport
- API groups: Engine lifecycle (7), Scene management (5), Actor management (17), Components (7), Camera (9), Renderer settings (12), Lighting (14), Material/PBR (8), Static mesh (3), Skeletal/Animation (7), Model loading (4), Model info (3), Scene queries (4), Texture (2), DebugRenderer (4)

### Serialization
- Binary archive (`KBinaryArchive`) with `<<`/`>>` operators for all engine types
- JSON archive (`KJsonArchive`) with custom DOM parser (KJsonValue hierarchy)

## Error Handling

1. **Use LOG macros**: `LOG_INFO()`, `LOG_WARNING()`, `LOG_ERROR()`
2. **Return HRESULT** for functions that can fail
3. **Use CHECK_HRESULT macro** to early-return on failure
4. **Validate pointers** at function entry
5. **Use cleanup macros**: `SAFE_RELEASE(p)`, `SAFE_DELETE(p)`, `SAFE_DELETE_ARRAY(p)`

## Memory Management

1. **Use smart pointers** for owning resources (`std::shared_ptr`, `std::unique_ptr`)
2. **Use ComPtr** for all COM objects
3. **Delete copy constructor/assignment** for resource-owning classes
4. **Raw pointers only for non-owning observation**
5. **Use `static_assert`** for 16-byte alignment on constant buffer structs

## Performance Guidelines

1. Minimize D3D11 state changes (sort by shader, then texture via `KCommandBuffer`)
2. Use GPU instancing for repeated objects (`KInstancedRenderer`)
3. Use Map/Unmap for frequently updated data, UpdateSubresource for rarely updated
4. Use frustum culling (`KFrustum`) and GPU occlusion culling (`KOcclusionCuller`)
5. Profile with GPU timer (`KGPUTimer`)
6. Use LOD system (`KLODSystem`) for distance-based mesh simplification

## C# Editor Code Style

- **File-scoped namespaces**: `namespace KojeomEditor;`
- Classes/properties/methods: PascalCase
- Private fields: `_camelCase` with underscore prefix (`_engine`, `_viewModel`)
- Local variables: camelCase
- Nullable reference types enabled
- MVVM pattern with `INotifyPropertyChanged` and `RelayCommand`
- P/Invoke via `DllImport` with `CallingConvention.Cdecl`
- `IDisposable` for native resource management

## Documentation Rules

- All documentation in Markdown format under `docs/`
- Rules and guidelines: `docs/rules/`
- Technical documentation: `docs/` subdirectories
- Code comments: Doxygen-style for public APIs (`@brief`, `@param`, `@return`)
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
5. **Test changes** before marking work as complete (build test with msbuild/dotnet build)
6. **Keep changes focused** -- one feature/fix per commit
7. **Update all related files** when modifying APIs (headers, interop, editor bindings)

## Prohibited Actions

1. **Do not** modify third-party library code under `Engine/third_party/`
2. **Do not** commit debug/test code to main branch
3. **Do not** break existing API without updating all usages (including EngineInterop and C# editor)
4. **Do not** use raw pointers for owning resources
5. **Do not** ignore compiler warnings
6. **Do not** change the build configuration without updating documentation
7. **Do not** add new dependencies without explicit approval

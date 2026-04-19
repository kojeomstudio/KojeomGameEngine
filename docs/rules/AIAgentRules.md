# AI Agent Rules for KojeomGameEngine

This document defines the rules and guidelines that AI agents must follow when working on the KojeomGameEngine project.

## Project Overview

KojeomGameEngine is a C++ game engine built with DirectX 11, featuring a WPF-based editor. The project follows a modular architecture with clear separation between core engine, graphics, and utility components. The engine supports Forward and Deferred rendering, PBR materials, skeletal animation with state machines, physics, audio, and a full UI system.

## Solution Structure

- **Solution:** `KojeomEngine.sln` (Visual Studio 2022, C++17, x64)
- **Core Engine:** `Engine/` (Static Library, .lib)
- **Editor Bridge:** `Editor/EngineInterop/` (C++ DLL for C# interop)
- **Editor UI:** `Editor/KojeomEditor/` (C# WPF, .NET 8.0)
- **Samples:** `samples/` (16 sample projects: AnimationStateMachine, BasicRendering, DebugRendering, Gameplay, Lighting, LOD, Particles, PBR, Physics, PostProcessing, SkeletalMesh, Sky, Terrain, UI, UI/Layout, Water)

## Engine Module Structure

```
Engine/ (149 files, ~32,560 lines)
├── Core/ (3 files, ~935 lines)        # KEngine - Main engine class, Win32 window, main loop, subsystem ownership, ISubsystem interface, KSubsystemRegistry
│   ├── Engine.h/cpp                   # Main engine singleton
│   └── Subsystem.h                    # ISubsystem interface, KSubsystemRegistry
├── Graphics/ (73 files, ~20,064 lines) # Rendering pipeline and all visual subsystems (20 sub-systems)
│   ├── GraphicsDevice.h/cpp           # DirectX 11 device, swap chain, render targets
│   ├── Renderer.h/cpp                 # Central rendering orchestrator
│   ├── Camera.h/cpp                   # Perspective/orthographic camera
│   ├── Shader.h/cpp                   # HLSL shader compilation and management
│   ├── Mesh.h/cpp                     # Mesh with vertex/index/constant buffers
│   ├── Material.h/cpp                 # PBR material (7 texture slots, presets)
│   ├── Texture.h/cpp                  # Texture loading and manager
│   ├── Light.h                        # Directional, Point, Spot light structures
│   ├── Shadow/ (8 files)              # Shadow maps, cascaded shadow maps
│   ├── Deferred/ (4 files)            # G-Buffer, deferred renderer
│   ├── PostProcess/ (10 files)        # HDR, bloom, FXAA, color grading, auto exposure, DOF, motion blur, lens effects
│   ├── SSAO/ (2 files)                # Screen-space ambient occlusion
│   ├── SSR/ (2 files)                 # Screen-space reflections
│   ├── SSGI/ (2 files)                # Screen-space global illumination
│   ├── TAA/ (2 files)                 # Temporal anti-aliasing
│   ├── Sky/ (2 files)                 # Procedural atmospheric sky rendering
│   ├── Volumetric/ (2 files)          # Volumetric fog
│   ├── Water/ (2 files)               # Water rendering with reflection/refraction
│   ├── Terrain/ (2 files)             # Terrain rendering with LOD, height map, splat-map texturing
│   ├── Culling/ (4 files)             # Frustum culling, GPU occlusion culling
│   ├── CommandBuffer/ (2 files)       # Deferred command-based rendering with sort keys
│   ├── Instanced/ (2 files)           # GPU instanced rendering
│   ├── IBL/ (2 files)                 # Image-based lighting (irradiance, prefiltered env, BRDF LUT)
│   ├── LOD/ (4 files)                 # LOD generation (quadric error metrics) and runtime system
│   ├── Particle/ (2 files)            # GPU-accelerated particle system emitter
│   ├── Performance/ (2 files)         # GPU timer, frame stats
│   └── Debug/ (2 files)               # Debug renderer (grid, axis, wireframe)
├── Input/ (3 files, ~658 lines)       # Keyboard, mouse, raw input, action mapping, input callbacks
│   ├── InputManager.h/cpp             # Global input manager singleton
│   └── InputTypes.h                   # Key codes, mouse buttons, input types
├── Audio/ (6 files, ~929 lines)       # XAudio2-based audio, 3D sound, volume channels
│   ├── AudioManager.h/cpp             # Global audio manager singleton
│   ├── AudioSubsystem.h               # ISubsystem adapter wrapping AudioManager
│   ├── AudioTypes.h                   # Audio type definitions
│   └── Sound.h/cpp                    # Sound resource
├── Physics/ (6 files, ~937 lines)     # Rigid body, physics world, collision detection, raycast
│   ├── PhysicsWorld.h/cpp             # Physics simulation world
│   ├── PhysicsSubsystem.h             # ISubsystem adapter for physics
│   ├── PhysicsTypes.h                 # Physics type definitions
│   └── RigidBody.h/cpp                # Rigid body component
├── Scene/ (4 files, ~690 lines)       # Actor-Component system, scene management, parent-child hierarchy
│   ├── Actor.h/cpp                    # KActor - entity with components
│   └── SceneManager.h/cpp             # KScene - scene management
├── Assets/ (18 files, ~4,600 lines)   # Static mesh (LOD), skeletal mesh, skeleton, animation, animation state machine, model loader (Assimp + fallbacks), actor components
│   ├── StaticMesh.h/cpp               # Static mesh with LOD support
│   ├── SkeletalMeshComponent.h/cpp    # Skeletal mesh actor component
│   ├── StaticMeshComponent.h/cpp      # Static mesh actor component
│   ├── LightComponent.h/cpp           # Light actor component
│   ├── Skeleton.h/cpp                 # Skeleton hierarchy
│   ├── Animation.h/cpp                # Animation clip
│   ├── AnimationInstance.h/cpp        # Runtime animation state
│   ├── AnimationStateMachine.h/cpp    # Animation state machine
│   └── ModelLoader.h/cpp              # Model loading (Assimp + fallbacks)
├── Serialization/ (4 files, ~1,030 lines) # Binary archive, JSON archive (custom DOM parser)
│   ├── BinaryArchive.h/cpp            # KBinaryArchive
│   └── JsonArchive.h/cpp              # KJsonArchive with custom DOM parser
├── UI/ (27 files, ~2,208 lines)       # Canvas-based UI system (element, panel, button, text, image, slider, checkbox, layouts, font)
│   ├── UICanvas.h/cpp                 # Root canvas container
│   ├── UIElement.h/cpp                # Base UI element
│   ├── UIPanel.h/cpp                  # Container panel
│   ├── UIButton.h/cpp                 # Button element
│   ├── UIText.h/cpp                   # Text element
│   ├── UIImage.h/cpp                  # Image element
│   ├── UISlider.h/cpp                 # Slider element
│   ├── UICheckbox.h/cpp               # Checkbox element
│   ├── UIFont.h/cpp                   # Font rendering
│   ├── UILayout.h/cpp                 # Base layout
│   ├── UIHorizontalLayout.h/cpp       # Horizontal layout
│   ├── UIVerticalLayout.h/cpp         # Vertical layout
│   ├── UIGridLayout.h/cpp             # Grid layout
│   └── UITypes.h                      # UI type definitions
├── DebugUI/ (2 files, ~241 lines)     # ImGui-based debug overlay with panel registration
│   └── DebugUI.h/cpp                  # KDebugUI singleton
└── Utils/ (3 files, ~311 lines)       # Common.h, Logger.h, Math.h
```

## Editor Structure

```
Editor/
├── EngineInterop/ (2 files, ~1,247 lines) # C++ DLL exposing flat C API (extern "C") for P/Invoke
│   ├── EngineAPI.h     # 113 exported functions for engine operations
│   └── EngineAPI.cpp   # Implementation wrapping C++ engine classes via FEngineWrapper
└── KojeomEditor/ (23 files, ~4,379 lines) # C# WPF editor (.NET 8.0)
    ├── Services/       # EngineInterop P/Invoke wrapper (113 DllImport), UndoRedoService (2 C# files, ~1,230 lines)
    ├── ViewModels/     # MainViewModel, SceneViewModel, PropertiesViewModel, ComponentViewModel (4 C# files, ~928 lines)
    ├── Views/          # ViewportControl, SceneHierarchy, PropertiesPanel, MaterialEditor, RendererSettings, ContentBrowser (6 XAML + 6 code-behind, ~1,814 lines)
    └── Styles/         # CommonStyles.xaml
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
| Constants | ALL_CAPS in namespace | `EngineConstants::DEFAULT_FOV`, `EngineConstants::DEFAULT_WINDOW_WIDTH` |
| Namespaces | PascalCase | `KojeomEngine::KDebugUI` |
| Parameters | camelCase with 'In' prefix | `InGraphicsDevice`, `InMesh`, `InName` |
| Type Aliases | PascalCase | `ActorPtr`, `ComponentPtr`, `ComPtr<T>` |
| Template Params | PascalCase | `T`, `FuncType` |

### File Organization

- Header files use `#pragma once`
- Include order: `#include "../Utils/Common.h"` first, then project headers, then external headers (D3D11, DirectXMath), then standard library
- Use custom type aliases from `Engine/Utils/Common.h` (`uint8`, `uint16`, `uint32`, `uint64`, `int8`, `int16`, `int32`, `int64`) instead of `<cstdint>` types directly
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

### Shader Architecture

The engine uses a two-class shader system:
- **`KShader`** — individual compiled shader (vertex or pixel), created via `KShader::CompileFromString()` from inline HLSL source strings
- **`KShaderProgram`** — combined vertex + pixel shader pair with input layout and constant buffer reflection
- **`EShaderType`** enum: `Vertex`, `Pixel`, `Geometry`, `Hull`, `Domain`, `Compute`
- **IMPORTANT**: There are NO `.hlsl` shader files in the repository. All shaders are embedded as inline C++ string literals and compiled at runtime. When adding new shaders, define them as inline string literals, not as separate `.hlsl` files.

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
  - `KLightComponent` -- wraps light properties, attached to actors for scene lighting
  - `KTerrainComponent` -- wraps `KTerrain` + `KHeightMap`
  - `KWaterComponent` -- wraps `KWater`
- `KScene` manages actors with parent-child hierarchy
- Camera and lighting are managed by the renderer directly, not as actor components

### Singletons
- `KEngine` (global instance via `GetInstance()`)
- `KInputManager` (global instance)
- `KAudioManager` (global instance)
- `KDebugUI` (global instance, `KojeomEngine::KDebugUI`)
- `KAudioManager` also has an `ISubsystem` adapter wrapper:
  - `KAudioSubsystem` wraps `KAudioManager`
  - Access via `KEngine::GetSubsystem<T>()` as alternative to singleton `GetInstance()`
- `KPhysicsWorld` is a regular class owned by `KPhysicsSubsystem` (NOT a singleton)
- `ESubsystemState` enum: `Uninitialized`, `Initialized`, `Running`, `Shutdown`

### Subsystem Interface
- `ISubsystem` -- base interface for all engine modules (Initialize, Tick, Shutdown)
- `KSubsystemRegistry` -- owns and manages subsystems with type-based registration and ordered lifecycle (defined in `Engine/Core/Subsystem.h`)
- Registered subsystems: `KAudioSubsystem`, `KPhysicsSubsystem`

### C#/C++ Interop
- `EngineInterop.dll` exposes 113 flat C functions (`extern "C"`, `__declspec(dllexport)`)
- C# consumes via P/Invoke (`DllImport`, 113 DllImport declarations in `EngineInterop.cs`)
- Internal `FEngineWrapper` class manages engine instance, model loader, and debug renderer
- String returns use `thread_local std::string` buffers
- Preprocessor: `ENGINEAPI_EXPORTS` controls dllexport/dllimport
- API groups: Engine lifecycle (9), Scene management (8), Actor management (23), Camera (11), Renderer settings (17), Directional light (7), Point light (4), Spot light (2), Material (9), StaticMesh component (3), SkeletalMesh component (7), Model loading (7), Texture (2), DebugRenderer (4)

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
8. **Do not** create `.hlsl` shader files. All shaders must be defined as inline C++ string literals and compiled at runtime via `KShader::CompileFromString()`.
9. **Do not** add new C API functions to `EngineAPI.h`/`EngineAPI.cpp` without also adding corresponding C# `DllImport` declarations in `Editor/KojeomEditor/Services/EngineInterop.cs`. Currently 113 C API functions with 113 C# DllImport declarations.

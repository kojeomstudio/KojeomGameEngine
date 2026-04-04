# AGENTS.md — KojeomGameEngine

Instructions for AI coding agents working in this repository.

## Project Overview

C++17 / DirectX 11 game engine with a WPF-based editor (C# / .NET 8.0). Supports Forward and Deferred rendering, PBR, skeletal animation, physics, audio, and a canvas UI system.

- **Solution:** `KojeomEngine.sln` (Visual Studio 2022, MSVC v143, C++17, x64 only)
- **Core Engine:** `Engine/` (static library, .lib)
- **Editor Bridge:** `Editor/EngineInterop/` (C++ DLL exposing flat C API for P/Invoke)
- **Editor UI:** `Editor/KojeomEditor/` (C# WPF, .NET 8.0, x64)
- **Samples:** `samples/` (16 sample projects demonstrating engine features)

## Build Commands

```bash
# Build entire solution (from repo root)
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64
msbuild KojeomEngine.sln /p:Configuration=Release /p:Platform=x64

# Build a single project
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64 /t:Engine
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64 /t:BasicRenderingSample

# Build the C# editor
dotnet build Editor/KojeomEditor/KojeomEditor.csproj
dotnet build Editor/KojeomEditor/KojeomEditor.csproj -c Release
```

## Testing

There is **no automated test framework** in this project. No test runner, test framework, or test projects exist. Validation is done through:
- 16 sample projects under `samples/`
- `LOG_INFO`, `LOG_WARNING`, `LOG_ERROR` macros for debug output
- `_CrtSetDbgFlag` for debug memory leak detection

To manually verify changes, build and run a relevant sample project.

## Linting / Formatting

No linter or formatter is configured. No `.clang-format`, `.clang-tidy`, or `.editorconfig` files exist. Code follows Visual Studio defaults (4-space indentation). Ensure new code matches the style of existing files in the same module.

## C++ Naming Conventions (strictly enforced)

| Type | Convention | Example |
|------|------------|---------|
| Classes | PascalCase with `K` prefix | `KEngine`, `KRenderer`, `KActor`, `KGraphicsDevice` |
| Structs | PascalCase with `F` prefix | `FVertex`, `FDirectionalLight`, `FTransform`, `FBoundingBox` |
| Enums | `E` prefix (prefer `enum class`) | `ERenderPath`, `EKeyCode`, `EColliderType` |
| Functions/Methods | PascalCase | `Initialize()`, `BeginFrame()`, `CreateCubeMesh()` |
| Local variables | camelCase | `graphicsDevice`, `currentTime`, `exitCode` |
| Member variables | camelCase, `b` prefix for booleans | `GraphicsDevice`, `bIsRunning`, `bInFrame` |
| Constants | PascalCase inside namespace | `EngineConstants::DefaultFOV`, `Colors::CornflowerBlue` |
| Namespaces | PascalCase | `KojeomEngine::KDebugUI` |
| Type aliases | PascalCase | `ActorPtr`, `ComponentPtr`, `ComPtr<T>` |
| Parameters | camelCase, `In` prefix for inputs | `InGraphicsDevice`, `InMesh`, `InName` |
| Template params | PascalCase | `T`, `FuncType` |

Use the custom type aliases from `Engine/Utils/Common.h` (`uint8`, `uint32`, `int32`, etc.) instead of `<cstdint>` types directly.

## C++ Code Style

### Includes

- Always `#pragma once` (no `#ifndef` guards)
- First include: `#include "../Utils/Common.h"` (pulls in `<windows.h>`, DirectX, STL)
- Then project headers, then external headers, then standard library
- Paired files: `ClassName.h` / `ClassName.cpp`

### Header file template

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

    HRESULT Initialize(KGraphicsDevice* InDevice);
    void Cleanup();

    bool IsInitialized() const { return bInitialized; }

private:
    KGraphicsDevice* Device = nullptr;
    ComPtr<ID3D11Buffer> VertexBuffer;
    bool bInitialized = false;
};
```

### Resource Management

- `ComPtr<T>` for all COM objects (D3D11 interfaces)
- `std::unique_ptr` for exclusive ownership of subsystems
- `std::shared_ptr` for shared resources (meshes, textures, actors)
- Raw pointers only for non-owning observation (never own resources with raw pointers)
- Delete copy constructor/assignment for resource-owning classes
- `static_assert` for 16-byte alignment on constant buffer structs

### Error Handling

- Return `HRESULT` for functions that can fail; check with `FAILED(hr)`
- Use `CHECK_HRESULT(hr)` macro to early-return on failure
- Use `LOG_INFO()`, `LOG_WARNING()`, `LOG_ERROR()`, `LOG_HRESULT_ERROR()` macros
- Validate pointers at function entry before use
- Use `SAFE_RELEASE(p)`, `SAFE_DELETE(p)`, `SAFE_DELETE_ARRAY(p)` macros

### DirectX 11 Rules

- Shader Model 5.0 (`vs_5_0`, `ps_5_0`)
- Check `HRESULT` on all D3D11 calls
- Resource creation order: Device -> Context -> SwapChain -> RenderTargetView -> DepthStencilView
- Use `Map/Unmap` for frequently updated buffers, `UpdateSubresource` for rarely updated
- Shader registers: `b0` = transform, `b1` = light, `b2+` = material/custom; `t0` = diffuse, `t1+` = other textures

### Documentation

- Doxygen-style comments for public APIs: `/** @brief ... @param ... @return ... */`
- Brief `@file` and `@brief` descriptions at top of headers

## C# Editor Code Style

- File-scoped namespaces: `namespace KojeomEditor;`
- Classes/properties/methods: PascalCase
- Private fields: `_camelCase` with underscore prefix (`_engine`, `_viewModel`)
- Local variables: camelCase
- Nullable reference types enabled
- MVVM pattern with `INotifyPropertyChanged` and `RelayCommand`
- P/Invoke via `DllImport` with `CallingConvention.Cdecl`
- `IDisposable` for native resource management

## Architecture Patterns

- **Entity-Component**: `KActor` owns `KActorComponent`s via `GetComponent<T>()`
- **Singletons**: `KEngine`, `KInputManager`, `KAudioManager`, `KDebugUI` (via `GetInstance()`)
- **C#/C++ Interop**: `EngineInterop.dll` flat C API (`extern "C"`) consumed by C# P/Invoke
- **Serialization**: `KBinaryArchive` for binary data, `KJsonArchive` with custom DOM parser

## Engine Module Map

```
Engine/
├── Core/           # KEngine - window, main loop, subsystem ownership
├── Graphics/       # Device, renderer, camera, shader, mesh, material, texture, light
│   ├── CommandBuffer/# Deferred command-based rendering
│   ├── Debug/      # Debug renderer
│   ├── Shadow/     # Shadow maps, cascaded shadow maps
│   ├── Deferred/   # G-Buffer, deferred renderer
│   ├── PostProcess/# HDR, lens effects (bloom), auto exposure, DOF, motion blur
│   ├── SSAO/       # Screen-space ambient occlusion
│   ├── SSR/        # Screen-space reflections
│   ├── TAA/        # Temporal anti-aliasing
│   ├── SSGI/       # Screen-space global illumination
│   ├── Sky/        # Procedural sky
│   ├── Volumetric/ # Volumetric fog
│   ├── Water/      # Water rendering
│   ├── Terrain/    # Terrain rendering
│   ├── Culling/    # Frustum and GPU occlusion culling
│   ├── Instanced/  # GPU instanced rendering
│   ├── IBL/        # Image-based lighting
│   ├── LOD/        # LOD generation and system
│   ├── Particle/   # Particle system
│   └── Performance/# GPU timer, frame stats
├── Input/          # Keyboard, mouse, raw input, action mapping
├── Audio/          # XAudio2 audio, 3D sound
├── Physics/        # Rigid body, collision detection
├── Scene/          # Actor-Component system, scene management
├── Assets/         # Mesh, skeletal mesh, animation, model loader (Assimp)
├── Serialization/  # Binary and JSON archives
├── UI/             # Canvas-based UI system
├── DebugUI/        # ImGui debug overlay
└── Utils/          # Common.h, Logger.h, Math.h
```

## Git Commit Format

```
[Category] Brief description

Detailed description if necessary.

- List of changes
```

Categories: `[Core]`, `[Graphics]`, `[Input]`, `[Audio]`, `[Physics]`, `[Scene]`, `[UI]`, `[Editor]`, `[Docs]`, `[Build]`, `[Refactor]`, `[Fix]`, `[Feature]`

## Critical Rules

1. Read existing code before making changes to understand established patterns
2. When modifying engine APIs, also update `Editor/EngineInterop/EngineAPI.h`, `EngineAPI.cpp`, and C# editor bindings in `Editor/KojeomEditor/Services/`
3. Never use raw pointers for owning resources
4. Never modify third-party library code under `Engine/lib/`
5. Never commit debug/test code to main branch
6. Never add new dependencies without explicit approval
7. Never break existing APIs without updating all call sites
8. Document new features under `docs/`

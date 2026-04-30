# KojeomGameEngine - Agent Work Instruction

> **Purpose**: This document instructs AI agents on what to build, how to build it, and how to verify it.  
> **Tech Stack**: C++17 / DirectX 11 / MSVC v143 / x64 (Engine) + C# / .NET 8.0 / WPF (Editor)  
> **Solution**: `KojeomEngine.sln` (Visual Studio 2022)

---

## RULES (Mandatory)

| # | Rule | Detail |
|---|------|--------|
| R1 | Read rules first | Always read `docs/rules/AIAgentRules.md` before starting work |
| R2 | Read existing code | Never change code without reading surrounding context first |
| R3 | Check commit history | Use `git log --oneline -30` to see what was done in prior sessions |
| R4 | Build after every change | Run `msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64` and `dotnet build Editor/KojeomEditor/KojeomEditor.csproj` after changes |
| R5 | Do not modify work/ or pipeline/ | These paths are used by CI/CD; never edit them |
| R6 | Do not modify third_party/ | `Engine/third_party/` is off-limits |
| R7 | Non-interactive mode | Make all decisions autonomously; do not ask for user input |
| R8 | Context compaction | When resuming from a previous session, run context compaction first |
| R9 | Naming conventions | Follow C++ naming strictly (see AGENTS.md): `K` prefix for classes, `F` for structs, `E` for enums, PascalCase methods, camelCase locals |
| R10 | No .hlsl files | All shaders are inline C++ string literals compiled via `KShader::CompileFromString()` |
| R11 | Update interop together | If you modify Engine C++ APIs, you MUST also update `Editor/EngineInterop/EngineAPI.h`, `EngineAPI.cpp`, and C# `DllImport` in `Editor/KojeomEditor/Services/` |
| R12 | Use custom types | Use `uint8`, `uint32`, `int32` etc. from `Engine/Utils/Common.h` (NOT `<cstdint>`) |
| R13 | Only edit when asked | Do not act on this file unless user explicitly asks |

---

## PROJECT ARCHITECTURE

### Dependency Flow

```text
Game (samples)
  -> Engine (Core, Graphics, Scene, Assets, Input, Audio, Physics, Serialization, UI, DebugUI)
    -> Platform (Windows/Win32 via DirectX 11)
      -> third_party (Assimp, STB, etc.)
```

### Engine Module Map

```text
Engine/
  Core/           # KEngine (singleton), ISubsystem, KSubsystemRegistry, main loop
  Graphics/       # Rendering: Device, Renderer, Camera, Shader, Mesh, Material, Texture, Light
  Graphics/Shadow/       # Shadow maps, cascaded shadow maps
  Graphics/Deferred/     # G-Buffer, deferred renderer
  Graphics/PostProcess/  # HDR, bloom, FXAA, color grading, auto exposure, DOF, motion blur
  Graphics/SSAO/         # Screen-space ambient occlusion
  Graphics/SSR/          # Screen-space reflections
  Graphics/SSGI/         # Screen-space global illumination
  Graphics/TAA/          # Temporal anti-aliasing
  Graphics/Sky/          # Procedural sky
  Graphics/Volumetric/   # Volumetric fog
  Graphics/Water/        # Water rendering
  Graphics/Terrain/      # Terrain rendering (height map, splat-map, LOD)
  Graphics/Culling/      # Frustum culling, GPU occlusion culling
  Graphics/CommandBuffer/# Deferred command-based rendering with sort keys
  Graphics/Instanced/    # GPU instanced rendering
  Graphics/IBL/          # Image-based lighting
  Graphics/LOD/          # LOD generation (quadric error metrics), runtime system
  Graphics/Particle/     # GPU-accelerated particle system
  Graphics/Performance/  # GPU timer, frame stats
  Graphics/Debug/        # Debug renderer (grid, axis, wireframe)
  Input/          # Keyboard, mouse, raw input, action mapping
  Audio/          # XAudio2, 3D sound
  Physics/        # Rigid body, collision, raycast
  Scene/          # KActor, KActorComponent, KScene (parent-child hierarchy)
  Assets/         # StaticMesh, SkeletalMesh, Skeleton, Animation, AnimationStateMachine, ModelLoader (Assimp)
  Serialization/  # Binary archive, JSON archive
  UI/             # Canvas-based UI system
  DebugUI/        # ImGui debug overlay
  Utils/          # Common.h, Logger.h, Math.h
```

### Editor Structure

```text
Editor/
  EngineInterop/     # C++ DLL: flat C API (extern "C", 115 exported functions) for P/Invoke
    EngineAPI.h      # Function declarations
    EngineAPI.cpp    # Implementation (FEngineWrapper)
  KojeomEditor/      # C# WPF (.NET 8.0, x64)
    Services/        # EngineInterop P/Invoke wrapper (115 DllImport), UndoRedo
    ViewModels/      # MVVM ViewModels
    Views/           # XAML views + code-behind
    Styles/          # CommonStyles.xaml
```

### Key Patterns

- **Entity-Component**: `KActor` owns `KActorComponent`s (`KStaticMeshComponent`, `KSkeletalMeshComponent`, `KLightComponent`, `KTerrainComponent`, `KWaterComponent`)
- **Singletons**: `KEngine`, `KInputManager`, `KAudioManager`, `KDebugUI` (via `GetInstance()`)
- **Subsystems**: `ISubsystem` -> `KAudioSubsystem`, `KPhysicsSubsystem`, `KRenderSubsystem` (via `KEngine::GetSubsystem<T>()`)
- **Resource Management**: `ComPtr<T>` for COM, `std::shared_ptr` for shared assets, `std::unique_ptr` for exclusive ownership
- **Error Handling**: Return `HRESULT`, check with `FAILED(hr)`, use `CHECK_HRESULT`, `LOG_INFO/WARNING/ERROR`

---

## CURRENT IMPLEMENTATION STATUS

### Completed

| Module | Status | Details |
|--------|--------|---------|
| Core/Engine | Done | KEngine singleton, ISubsystem, KSubsystemRegistry, Win32 window, main loop |
| Graphics/Device | Done | DirectX 11 device, swap chain, render targets |
| Graphics/Renderer | Done | Forward + Deferred render paths, PBR |
| Graphics/Camera | Done | Perspective/orthographic camera |
| Graphics/Shader | Done | Inline HLSL compile, ShaderProgram |
| Graphics/Mesh | Done | Vertex/index/constant buffers |
| Graphics/Material | Done | PBR material (7 texture slots) |
| Graphics/Texture | Done | Texture loading and manager |
| Graphics/Light | Done | Directional, Point, Spot |
| Graphics/Shadow | Done | Shadow maps, cascaded shadow maps |
| Graphics/PostProcess | Done | HDR, bloom, FXAA, color grading, auto exposure, DOF, motion blur |
| Graphics/SSAO | Done | Screen-space ambient occlusion |
| Graphics/SSR | Done | Screen-space reflections |
| Graphics/SSGI | Done | Screen-space global illumination |
| Graphics/TAA | Done | Temporal anti-aliasing |
| Graphics/Sky | Done | Procedural atmospheric sky |
| Graphics/Volumetric | Done | Volumetric fog |
| Graphics/Water | Done | Water with reflection/refraction |
| Graphics/Terrain | Done | Height map, splat-map, LOD |
| Graphics/Culling | Done | Frustum + GPU occlusion culling |
| Graphics/CommandBuffer | Done | Deferred command rendering with sort keys |
| Graphics/Instanced | Done | GPU instanced rendering |
| Graphics/IBL | Done | Irradiance, prefiltered env, BRDF LUT |
| Graphics/LOD | Done | Quadric error metrics LOD |
| Graphics/Particle | Done | GPU particle system |
| Graphics/Performance | Done | GPU timer, frame stats |
| Graphics/Debug | Done | Grid, axis, wireframe |
| Input | Done | Keyboard, mouse, raw input, action mapping |
| Audio | Done | XAudio2, 3D sound |
| Physics | Done | Rigid body, collision, raycast |
| Scene | Done | KActor, KActorComponent, KScene |
| Assets/StaticMesh | Done | With LOD support |
| Assets/SkeletalMesh | Done | Skeletal mesh component |
| Assets/Skeleton | Done | Skeleton hierarchy |
| Assets/Animation | Done | Animation clip, AnimationInstance, AnimationStateMachine |
| Assets/ModelLoader | Done | Assimp + fallbacks (OBJ, GLTF/GLB) |
| Assets/Components | Done | StaticMesh, SkeletalMesh, Light components |
| Serialization | Done | Binary + JSON archives |
| UI | Done | Canvas-based UI (element, panel, button, text, image, slider, checkbox, layouts, font) |
| DebugUI | Done | ImGui debug overlay |
| Editor/EngineInterop | Done | 115 exported C API functions |
| Editor/KojeomEditor | Done | WPF editor with viewport, hierarchy, properties, content browser |
| Samples | Done | 16 sample projects |
| CLI Test | Done | CLITestRunner sample |

---

## TASKS

### T1: Renderer Architecture Enhancement (Rendering Pipeline Refinement)

**Priority**: High | **Status**: New

The renderer is functional but can be improved. Review and enhance:

1. **Render Graph / Pass Ordering**  
   Evaluate current rendering pass ordering in `Engine/Graphics/Renderer.cpp`. Ensure correct pass dependencies:
   - Shadow pass -> Geometry pass -> Lighting pass -> Post-processing pass
   - Verify G-Buffer population in deferred path is complete before lighting

2. **Multi-threaded Command Buffer Submission**  
   `Engine/Graphics/CommandBuffer/` exists but review if multi-threaded command recording can be added for better CPU utilization.

3. **Shader Quality Improvements**  
   Review all inline shaders for:
   - Correct PBR BRDF implementation
   - Energy conservation in lighting calculations
   - Proper tone mapping in HDR pipeline

**Verification**: `msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64 /t:Engine` then run `samples/BasicRendering/` or `samples/PBR/`

### T2: FBX Model Loading Enhancement

**Priority**: High | **Status**: Enhancement

ModelLoader already supports Assimp + fallbacks (OBJ, GLTF/GLB). Enhance:

1. **FBX Animation Import**  
   Verify FBX skeletal mesh and animation clip import works correctly through Assimp.  
   Key files: `Engine/Assets/ModelLoader.cpp`, `Engine/Assets/Skeleton.cpp`, `Engine/Assets/Animation.cpp`

2. **Asset Cache Optimization**  
   Implement proper asset caching to avoid redundant loads of the same mesh/texture/skeleton.  
   Consider a central `KAssetManager` that tracks loaded assets by file path hash.

3. **Material Import from FBX/GLTF**  
   Import material properties (albedo, normal, metallic, roughness) from model files and create KMaterial instances.

**Verification**: Load an FBX file with skeletal animation via CLITestRunner or a sample

### T3: Animation System Advanced Features

**Priority**: Medium | **Status**: Enhancement

AnimationStateMachine exists but can be expanded:

1. **Blend Tree**  
   Add 1D/2D blend tree support for smooth blending between multiple animation clips based on parameters.

2. **Root Motion Extraction**  
   Extract and apply root motion from animation clips to character movement.

3. **Animation Events / Notifies**  
   Add keyframe event system for triggering sounds, effects, or logic at specific animation times.

**Files**: `Engine/Assets/AnimationStateMachine.cpp`, `Engine/Assets/AnimationInstance.cpp`

### T4: Editor Feature Completion

**Priority**: Medium | **Status**: Enhancement

The WPF editor has viewport, hierarchy, properties, content browser. Enhance:

1. **Scene Save/Load**  
   Implement scene serialization to/from JSON/Binary via `KJsonArchive`/`KBinaryArchive` through the interop API.

2. **Undo/Redo for Transform Operations**  
   UndoRedoService exists; verify it covers all transform gizmo operations.

3. **Material Editor Improvements**  
   Add texture slot preview, material preset management.

4. **Play-In-Editor Mode**  
   Add ability to run the game simulation within the editor viewport and stop/reset.

**Files**: `Editor/KojeomEditor/Views/`, `Editor/KojeomEditor/ViewModels/`, `Editor/EngineInterop/EngineAPI.h`

### T5: CLI Test Interface Expansion

**Priority**: Medium | **Status**: Enhancement

CLITestRunner sample exists. Expand CLI capabilities:

1. **Scene Dump Mode**  
   Export RenderScene (entities, transforms, materials, draw calls) as JSON for validation.

2. **Asset Validation Mode**  
   Verify all asset references in a scene file resolve to valid files.

3. **Screenshot Comparison**  
   Capture framebuffer and compare against baseline for regression detection.

4. **Exit Code Contract**  
   | Code | Meaning |
   |------|---------|
   | 0 | Success |
   | 1 | Command line parse error |
   | 2 | Scene/asset load failure |
   | 3 | Validation failure |
   | 4 | Renderer init failure |
   | 5 | Runtime smoke failure |

**Files**: `samples/CLITestRunner/`

### T6: Stability (GitHub Actions CI/CD)

**Priority**: High | **Status**: Maintenance

1. Verify all GitHub Actions workflows are logically correct
2. Ensure build succeeds on both Debug and Release configurations
3. Verify CI runs the CLITestRunner for automated validation
4. Check that no secrets or sensitive data are in the repository

**Verification**: Check `.github/workflows/` and run CI locally if possible

### T7: Security Review

**Priority**: High | **Status**: Maintenance

1. **Path Validation**  
   Verify all file path operations (asset loading, content browser, scene save/load) use proper validation against path traversal attacks.  
   Recent fixes addressed this in ContentBrowser and MainViewModel; verify completeness.

2. **Buffer Bounds**  
   Verify all buffer operations (vertex, index, constant, texture) have proper bounds checking.

3. **Resource Cleanup**  
   Verify no use-after-free or resource leaks in engine shutdown path.

4. **No Hardcoded Secrets**  
   Scan for API keys, passwords, tokens in source code.

**Files**: All engine modules, editor services, `Engine/Serialization/`

---

## BUILD COMMANDS

```bash
# Full solution build
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64
msbuild KojeomEngine.sln /p:Configuration=Release /p:Platform=x64

# Single project build
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64 /t:Engine
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64 /t:BasicRenderingSample

# C# Editor build
dotnet build Editor/KojeomEditor/KojeomEditor.csproj
dotnet build Editor/KojeomEditor/KojeomEditor.csproj -c Release
```

## TESTING

No automated test framework exists. Validation is done through:
- 16 sample projects under `samples/`
- `LOG_INFO`, `LOG_WARNING`, `LOG_ERROR` macros
- CLITestRunner sample for automated CLI testing

## GIT COMMIT FORMAT

```
[Category] Brief description

- Change 1
- Change 2
```

Categories: `[Core]`, `[Graphics]`, `[Input]`, `[Audio]`, `[Physics]`, `[Scene]`, `[UI]`, `[Editor]`, `[Docs]`, `[Build]`, `[Refactor]`, `[Fix]`, `[Feature]`, `[Security]`

## ARCHITECTURE REFERENCE

Detailed architecture documents are in `work/architecture/`:
- `00_Overview.md` - Engine goals, scope, and core principles
- `01_Runtime_Architecture.md` - Module dependencies and execution flow
- `02_Feature_Roadmap.md` - Phase-by-phase implementation plan
- `03_Testing_Quality_Maintenance.md` - Test layers and quality rules
- `04_Tools_Editor_Strategy.md` - C#/C++ tool boundary strategy
- `05_Renderable_Content_Features.md` - Static mesh, skeletal mesh, terrain, animation
- `06_Agent_CLI_Test_Interface.md` - CLI modes and agent verification workflow

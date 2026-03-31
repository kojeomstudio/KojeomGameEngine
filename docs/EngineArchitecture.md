# KojeomGameEngine Architecture Overview

## Summary

C++17 / DirectX 11 game engine with WPF-based editor (C# / .NET 8.0). Modular architecture with 12 engine modules, 20+ graphics sub-systems, and full editor integration via C#/C++ interop.

## Solution Structure

```
KojeomEngine.sln (Visual Studio 2022, MSVC v145, C++17, x64)
├── Engine/                    # Static library (.lib) - Core engine
├── Editor/
│   ├── EngineInterop/         # C++ DLL (flat C API for P/Invoke)
│   └── KojeomEditor/          # C# WPF editor (.NET 8.0)
└── samples/                   # 16 sample projects
```

## Engine Modules

| Module | Path | Description |
|--------|------|-------------|
| Core | `Engine/Core/` | KEngine singleton, Win32 window, main loop, subsystem ownership |
| Graphics | `Engine/Graphics/` | Full rendering pipeline (20+ sub-systems) |
| Input | `Engine/Input/` | Keyboard, mouse, raw input, action mapping |
| Audio | `Engine/Audio/` | XAudio2-based audio, 3D sound |
| Physics | `Engine/Physics/` | Rigid body, collision detection, raycast |
| Scene | `Engine/Scene/` | Actor-Component system, scene management, serialization |
| Assets | `Engine/Assets/` | Static/skeletal mesh, skeleton, animation, model loader (FBX/OBJ/GLTF) |
| Serialization | `Engine/Serialization/` | Binary archive, JSON archive (custom DOM parser) |
| UI | `Engine/UI/` | Canvas-based UI (text, button, image, slider, checkbox, layouts) |
| DebugUI | `Engine/DebugUI/` | ImGui debug overlay |
| Utils | `Engine/Utils/` | Common.h, Logger.h, Math.h |

## Graphics Sub-Systems

| Sub-System | Path | Feature |
|------------|------|---------|
| Forward/Deferred | `Graphics/Renderer.h`, `Graphics/Deferred/` | Dual render path support |
| PBR | `Graphics/Material.h`, `Graphics/Renderer.h` | Metallic-Roughness workflow, 7 texture slots |
| Shadows | `Graphics/Shadow/` | Shadow maps, cascaded shadow maps |
| Post-Processing | `Graphics/PostProcess/` | HDR, bloom, auto exposure, DOF, motion blur, lens effects |
| SSAO | `Graphics/SSAO/` | Screen-space ambient occlusion |
| SSR | `Graphics/SSR/` | Screen-space reflections |
| TAA | `Graphics/TAA/` | Temporal anti-aliasing |
| SSGI | `Graphics/SSGI/` | Screen-space global illumination |
| Sky | `Graphics/Sky/` | Procedural sky rendering |
| Volumetric Fog | `Graphics/Volumetric/` | Volumetric fog effect |
| Water | `Graphics/Water/` | Water rendering |
| Terrain | `Graphics/Terrain/` | Terrain rendering |
| IBL | `Graphics/IBL/` | Image-based lighting (irradiance, prefiltered env, BRDF LUT) |
| LOD | `Graphics/LOD/` | LOD generation and system |
| Particle | `Graphics/Particle/` | Particle system |
| Culling | `Graphics/Culling/` | Frustum and GPU occlusion culling |
| Instanced | `Graphics/Instanced/` | GPU instanced rendering |
| Command Buffer | `Graphics/CommandBuffer/` | Deferred command-based rendering |
| Debug | `Graphics/Debug/` | Debug renderer |
| Performance | `Graphics/Performance/` | GPU timer, frame stats |

## Key Design Patterns

### Entity-Component System
- `KActor` owns `KActorComponent`s via `GetComponent<T>()`
- Components: `KStaticMeshComponent`, `KSkeletalMeshComponent`
- `KScene` manages actors with parent-child hierarchy
- Component types: Base, StaticMesh, SkeletalMesh, Water, Terrain

### Singletons
- `KEngine::GetInstance()` - global engine instance
- `KInputManager`, `KAudioManager`, `KDebugUI`

### C#/C++ Interop
- `EngineInterop.dll` exposes ~40 flat C functions (`extern "C"`)
- C# consumes via P/Invoke (`DllImport`, `CallingConvention.Cdecl`)
- Covers: Engine lifecycle, Scene/Actor CRUD, Camera, Renderer, Model loading, Components

### Serialization
- `KBinaryArchive` - binary read/write with stream operators
- `KJsonArchive` - custom JSON DOM parser (no external dependency)
- Actor hierarchy serialization for scene save/load

### Animation System
- `KSkeleton` - bone hierarchy with bind poses (up to 256 bones)
- `KAnimation` - keyframe animation channels (position, rotation, scale)
- `KAnimationInstance` - runtime animation playback with speed control, play modes (Once/Loop/PingPong)
- `KAnimationStateMachine` - state machine with transitions, conditions (float/bool parameters), blending, notifies
- GPU skinning via `FSkinnedVertex` (4 bone influences) and `FBoneMatrixBuffer`

### Model Loading
- Primary: Assimp (when `USE_ASSIMP` is defined) - FBX, OBJ, GLTF, DAE, etc.
- Fallback parsers (no Assimp required):
  - OBJ: Full vertex/normal/UV parsing, fan triangulation, scale/flipUV support
  - GLTF/GLB: Placeholder cube mesh (full loading requires Assimp)
  - FBX: ASCII FBX geometry, basic skeleton hierarchy (binary FBX requires Assimp)

## Editor Architecture

```
Editor/KojeomEditor/ (.NET 8.0, WPF)
├── Services/
│   ├── EngineInterop.cs      # P/Invoke wrapper (~655 lines)
│   └── UndoRedoService.cs    # Undo/Redo system
├── ViewModels/
│   ├── MainViewModel.cs      # Main window VM, transform mode state
│   ├── SceneViewModel.cs     # Scene hierarchy VM, engine sync
│   ├── PropertiesViewModel.cs # Properties panel VM
│   └── ComponentViewModel.cs # Component VM
├── Views/
│   ├── ViewportControl.xaml  # D3D11 viewport (HwndHost), fly camera, object picking, drag-drop
│   ├── SceneHierarchyControl.xaml  # Actor tree
│   ├── PropertiesPanelControl.xaml # Transform/properties
│   ├── MaterialEditorControl.xaml  # PBR material editing
│   └── ContentBrowserControl.xaml  # Asset browser
└── Styles/
    └── CommonStyles.xaml     # Shared resource dictionary
```

### Editor Features
- **Viewport**: Native D3D11 rendering via HwndHost, WASD fly camera, mouse picking with raycasting, drag-and-drop asset spawning
- **Transform Modes**: Select, Move, Rotate, Scale (toolbar buttons)
- **Play/Pause/Stop**: Simulation control with scene state refresh
- **Scene Management**: New, Open, Save scene files (.scene)
- **Undo/Redo**: Action-based undo/redo service
- **Status Bar**: Real-time FPS, draw calls, vertex count display

## Sample Projects

| Sample | Feature | Path |
|--------|---------|------|
| BasicRendering | Basic rendering pipeline, cube, sphere, camera orbit | `samples/BasicRendering/` |
| Lighting | Directional/point/spot lights | `samples/Lighting/` |
| PBR | Physically-based rendering, deferred path | `samples/PBR/` |
| PostProcessing | Post-processing effects (HDR, bloom, DOF, motion blur) | `samples/PostProcessing/` |
| Terrain | Terrain rendering system | `samples/Terrain/` |
| Water | Water rendering system | `samples/Water/` |
| Sky | Procedural sky system | `samples/Sky/` |
| Particles | Particle system emitter | `samples/Particles/` |
| SkeletalMesh | GPU skinning, procedural humanoid, bone debug viz | `samples/SkeletalMesh/` |
| Gameplay | Input handling + audio playback | `samples/Gameplay/` |
| Physics | Physics simulation, collision, ball spawning | `samples/Physics/` |
| UI | Canvas UI system elements | `samples/UI/` |
| Layout | UI layout system (vertical, horizontal, grid) | `samples/UI/Layout/` |
| AnimationStateMachine | Animation state machine, transitions, blending, locomotion | `samples/AnimationStateMachine/` |
| LOD | Level-of-detail generation and switching | `samples/LOD/` |
| DebugRendering | Debug renderer, debug UI overlay | `samples/DebugRendering/` |

## Build

```bash
# C++ (Debug)
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64

# C++ (Release)
msbuild KojeomEngine.sln /p:Configuration=Release /p:Platform=x64

# C# Editor
dotnet build Editor/KojeomEditor/KojeomEditor.csproj
dotnet build Editor/KojeomEditor/KojeomEditor.csproj -c Release
```

## Dependencies

| Library | Purpose | Required |
|---------|---------|----------|
| DirectX 11 SDK | Graphics rendering | Yes (Windows SDK) |
| XAudio2 | Audio playback | Yes (Windows SDK) |
| Assimp | Model loading (FBX, GLTF, etc.) | Optional (`USE_ASSIMP`) |
| ImGui | Debug UI overlay | Included in source |

## Statistics

| Category | Count |
|----------|-------|
| Engine source files (.h + .cpp) | 144 |
| Engine total lines | ~36,000 |
| Editor C# files | 13 |
| Editor XAML files | 9 |
| Sample projects | 16 |
| Total solution projects | 19 |
| Engine modules | 12 |
| Graphics sub-systems | 20 |
| EngineInterop API functions | ~40 |

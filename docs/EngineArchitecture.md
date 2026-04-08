# KojeomGameEngine Architecture Overview

## Summary

C++17 / DirectX 11 game engine with WPF-based editor (C# / .NET 8.0). Modular architecture with 12 engine modules, 20 graphics sub-systems, and full editor integration via C#/C++ interop.

## Solution Structure

```
KojeomEngine.sln (Visual Studio 2022, MSVC v143, C++17, x64)
├── Engine/                    # Static library (.lib) - Core engine
├── Editor/
│   ├── EngineInterop/         # C++ DLL (flat C API for P/Invoke)
│   └── KojeomEditor/          # C# WPF editor (.NET 8.0)
└── samples/                   # 16 sample projects
```

## Engine Modules

| Module | Path | Files | Lines | Description |
|--------|------|------:|------:|-------------|
| Graphics | `Engine/Graphics/` | 73 | 19,887 | Full rendering pipeline (20 sub-systems) |
| Assets | `Engine/Assets/` | 16 | 4,187 | Static/skeletal mesh, skeleton, animation, model loader |
| UI | `Engine/UI/` | 27 | 2,183 | Canvas-based UI system |
| Physics | `Engine/Physics/` | 6 | 922 | Rigid body, collision detection, raycast |
| Core | `Engine/Core/` | 3 | 916 | KEngine singleton, Win32 window, main loop, ISubsystem, KSubsystemRegistry |
| Audio | `Engine/Audio/` | 6 | 845 | XAudio2 audio, 3D sound |
| Serialization | `Engine/Serialization/` | 4 | 819 | Binary archive, JSON archive |
| Input | `Engine/Input/` | 3 | 655 | Keyboard, mouse, raw input, action mapping |
| Scene | `Engine/Scene/` | 4 | 629 | Actor-Component system, scene management |
| DebugUI | `Engine/DebugUI/` | 2 | 241 | ImGui debug overlay |
| Utils | `Engine/Utils/` | 3 | 260 | Common.h, Logger.h, Math.h |
| **Total** | | **147** | **31,544** | |

## Graphics Sub-Systems

| Sub-System | Path | Files | Lines | Feature |
|------------|------|------:|------:|---------|
| Core | `Graphics/` (root) | 15 | 5,143 | Renderer, Device, Shader, Mesh, Material, Texture, Camera, Light |
| PostProcess | `Graphics/PostProcess/` | 10 | 2,964 | HDR, bloom, auto exposure, DOF, motion blur |
| Shadow | `Graphics/Shadow/` | 8 | 1,152 | Shadow maps, cascaded shadow maps, skinned shadow shader for skeletal meshes |
| Deferred | `Graphics/Deferred/` | 4 | 1,180 | G-Buffer, deferred renderer |
| Debug | `Graphics/Debug/` | 2 | 874 | Debug wireframe/shape rendering |
| IBL | `Graphics/IBL/` | 2 | 931 | Image-based lighting (irradiance, prefiltered env, BRDF LUT) |
| LOD | `Graphics/LOD/` | 4 | 784 | LOD generation and system |
| Terrain | `Graphics/Terrain/` | 2 | 672 | Terrain rendering |
| Particle | `Graphics/Particle/` | 2 | 563 | Particle system |
| Culling | `Graphics/Culling/` | 4 | 527 | Frustum and GPU occlusion culling |
| CommandBuffer | `Graphics/CommandBuffer/` | 2 | 525 | Deferred command recording |
| SSAO | `Graphics/SSAO/` | 2 | 630 | Screen-space ambient occlusion |
| SSGI | `Graphics/SSGI/` | 2 | 624 | Screen-space global illumination |
| Sky | `Graphics/Sky/` | 2 | 574 | Procedural sky rendering |
| Volumetric | `Graphics/Volumetric/` | 2 | 519 | Volumetric fog |
| Water | `Graphics/Water/` | 2 | 541 | Water rendering |
| SSR | `Graphics/SSR/` | 2 | 515 | Screen-space reflections |
| TAA | `Graphics/TAA/` | 2 | 490 | Temporal anti-aliasing |
| Performance | `Graphics/Performance/` | 2 | 309 | GPU timer, frame stats |
| Instanced | `Graphics/Instanced/` | 2 | 227 | GPU instanced rendering |

## Key Design Patterns

### Entity-Component System
- `KActor` owns `KActorComponent`s via `GetComponent<T>()`
- Components: `KStaticMeshComponent`, `KSkeletalMeshComponent`, `KLightComponent`
- `KScene` manages actors with parent-child hierarchy
- Component types: Transform, StaticMesh, SkeletalMesh, Light, Camera, Audio (stub), Physics (stub)

### Singletons
- `KEngine::GetInstance()` - global engine instance
- `KInputManager` - keyboard, mouse, raw input, action mapping
- `KAudioManager` - XAudio2 audio engine
- `KDebugUI` - ImGui debug overlay

### Subsystem Interface
- `ISubsystem` - base interface with `Initialize()`, `Tick()`, `Shutdown()` lifecycle methods
- `KSubsystemRegistry` - type-based registration, ordered initialization and reverse-order shutdown

### C#/C++ Interop
- `EngineInterop.dll` exposes 107 flat C functions (`extern "C"`)
- C# consumes via P/Invoke (`DllImport`, `CallingConvention.Cdecl`)
- API groups: Engine lifecycle (7), Scene management (8), Actor management (20), Camera (10), Renderer settings (12), Lighting (11), Material (8), Components (13), Model loading (8), Texture (2)

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

### Skeletal Mesh Shadow Casting
- `FShadowCaster` struct includes skeletal mesh fields: `bIsSkeletal`, `SkeletalVertexBuffer`, `SkeletalIndexBuffer`, `SkeletalIndexCount`, `BoneMatrixBuffer`
- Skinned shadow shader (`SkinnedShadowShader`) renders skeletal meshes into shadow maps using bone matrix palette
- Both `KShadowRenderer` and `KCascadedShadowRenderer` support skeletal shadow casters via `AddShadowCaster()`

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
│       ├── EngineInterop.cs      # P/Invoke wrapper (888 lines, 100 DllImport declarations)
│   └── UndoRedoService.cs    # Undo/Redo system (231 lines, command pattern)
├── ViewModels/
│   ├── MainViewModel.cs      # Main window VM, transform mode state
│   ├── SceneViewModel.cs     # Scene hierarchy VM, engine sync
│   ├── PropertiesViewModel.cs # Properties panel VM
│   └── ComponentViewModel.cs # Component VM (StaticMesh, Light, Camera, Material)
├── Views/
│   ├── ViewportControl.xaml  # Native Win32 child window D3D11 viewport, WASD fly camera, object picking, drag-drop
│   ├── SceneHierarchyControl.xaml  # Actor tree with context menu
│   ├── PropertiesPanelControl.xaml # Transform/properties editing
│   ├── MaterialEditorControl.xaml  # PBR material editing, 7 presets
│   ├── RendererSettingsControl.xaml # Rendering feature toggles (11 checkboxes)
│   └── ContentBrowserControl.xaml  # Asset browser with folder tree, thumbnails, drag-drop
└── Styles/
    └── CommonStyles.xaml     # VS Code dark theme colors
```

### Editor Statistics

| Category | Count |
|----------|-------|
| C# source files | 14 |
| XAML files | 9 |
| C# lines of code | 3,341 |
| XAML lines | 730 |

### Editor Features
- **Viewport**: Native Win32 child window with D3D11 rendering, WASD fly camera, mouse picking with raycasting, drag-and-drop asset spawning, proper HWND positioning via `MoveWindow`/`ScreenToClient` for correct panel resizing
- **Renderer Settings**: Toggle panel for rendering features (SSAO, Post Processing, Shadows, Sky, TAA, Debug UI, SSR, Volumetric Fog, Wireframe Mode, Show Grid, Show Axis) with real-time engine synchronization
- **Transform Modes**: Select, Move, Rotate, Scale (toolbar buttons)
- **Play/Pause/Stop**: Toolbar buttons (simulation logic scaffold only)
- **Scene Management**: New, Open, Save scene files (.scene)
- **Undo/Redo**: Action-based undo/redo wired to actor create/delete and transform operations (AddItemAction, RemoveItemAction, SetPropertyAction)
- **Status Bar**: Real-time FPS, draw calls, vertex count display
- **Material Editor**: PBR parameters (Albedo, Metallic, Roughness, AO), 7 presets (Default, Metal, Plastic, Rubber, Gold, Silver, Copper); fully connected to engine via EngineInterop material API (Set/Get Albedo, Metallic, Roughness, AO)

## EngineInterop API

### API Groups (107 exported functions)

| Group | Functions | Description |
|-------|----------:|-------------|
| Engine Lifecycle | 7 | Create, Destroy, Initialize, Tick, Render, Resize, InitializeEmbedded |
| Subsystem Access | 2 | GetSceneManager, GetRenderer |
| Scene Management | 8 | Create, Load, Save, SetActive, GetActive, Raycast, GetActorCount, GetActorAt |
| Actor Management | 20 | Create, Destroy, Transform (Get/Set), Name, Components, Visibility, Hierarchy |
| Camera | 10 | GetMain, Transform, Matrices, FOV, Near/Far |
| Renderer Settings | 12 | RenderPath, DebugMode, Stats, SSAO, PostProcess, Shadow, Sky, TAA, DebugUI, SSR, VolumetricFog |
| Directional Light | 7 | Set/Get properties, Direction, Color, Ambient, Intensity |
| Point Light | 4 | Add, Clear, Count, Get |
| Spot Light | 2 | Add, Clear |
| Material | 8 | Set/Get Albedo, Metallic, Roughness, AO (connected to editor MaterialEditorControl) |
| StaticMesh Component | 4 | Get, SetMesh, GetMaterial, CreateDefaultMesh |
| SkeletalMesh Component | 8 | Get, Play/Stop/Pause/Resume, AnimationCount, AnimationName, LoadModel |
| Model Loading | 6 | Load, Unload, LoadStaticMesh, LoadSkeletalMesh, HasSkeleton, AnimationInfo |
| Texture | 2 | Load, Unload |
| DebugRenderer | 4 | DrawGrid, DrawAxis, SetEnabled, RenderFrame |
| Other | 3 | GetLightComponent (stub), AddChild, GetChild, GetParent |

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
| Engine source files (.h + .cpp) | 147 |
| Engine total lines | ~31,544 |
| Editor C# files | 14 |
| Editor XAML files | 9 |
| Editor C# lines | ~3,341 |
| Editor XAML lines | ~730 |
| Sample projects | 16 |
| Engine modules | 12 |
| Graphics sub-systems | 20 |
| EngineInterop API functions | 107 |
| Total solution projects | 19 |

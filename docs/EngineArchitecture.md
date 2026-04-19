# KojeomGameEngine Architecture Overview

## Summary

C++17 / DirectX 11 game engine with WPF-based editor (C# / .NET 8.0). Modular architecture with 11 engine modules, 20 graphics sub-systems, and full editor integration via C#/C++ interop.

## Solution Structure

```
KojeomEngine.sln (Visual Studio 2022, MSVC v143, C++17, x64)
├── Engine/                    # Static library (.lib) - Core engine
├── Editor/
│   ├── EngineInterop/         # C++ DLL (flat C API for P/Invoke, 2 files, ~1,247 lines)
│   └── KojeomEditor/          # C# WPF editor (.NET 8.0, 23 files, ~4,379 lines)
└── samples/                   # 16 sample projects (15 top-level + 1 nested)
```

## Engine Modules

| Module | Path | Files | Lines | Description |
|--------|------|------:|------:|-------------|
| Graphics | `Engine/Graphics/` | 73 | 20,064 | Full rendering pipeline (20 sub-systems) |
| Assets | `Engine/Assets/` | 18 | 4,600 | Static/skeletal mesh, skeleton, animation, state machine, model loader, light component |
| UI | `Engine/UI/` | 27 | 2,208 | Canvas-based UI system |
| Physics | `Engine/Physics/` | 6 | 937 | Rigid body, collision detection, raycast |
| Serialization | `Engine/Serialization/` | 4 | 1,030 | Binary archive, JSON archive |
| Core | `Engine/Core/` | 3 | 935 | KEngine singleton, Win32 window, main loop, ISubsystem, KSubsystemRegistry |
| Audio | `Engine/Audio/` | 6 | 929 | XAudio2 audio, 3D sound |
| Input | `Engine/Input/` | 3 | 658 | Keyboard, mouse, raw input, action mapping |
| Scene | `Engine/Scene/` | 4 | 690 | Actor-Component system, scene management |
| DebugUI | `Engine/DebugUI/` | 2 | 241 | ImGui debug overlay |
| Utils | `Engine/Utils/` | 3 | 311 | Common.h, Logger.h, Math.h |
| **Total** | | **149** | **32,603** | |

## Graphics Sub-Systems

| Sub-System | Path | Files | Lines | Feature |
|------------|------|------:|------:|---------|
| Core | `Graphics/` (root) | 15 | 5,308 | Renderer, Device, Shader, Mesh, Material, Texture, Camera, Light |
| PostProcess | `Graphics/PostProcess/` | 10 | 2,991 | HDR, bloom, FXAA, color grading, auto exposure, DOF, motion blur, lens effects |
| Shadow | `Graphics/Shadow/` | 8 | 1,150 | Shadow maps, cascaded shadow maps |
| Deferred | `Graphics/Deferred/` | 4 | 1,281 | G-Buffer, deferred renderer |
| IBL | `Graphics/IBL/` | 2 | 937 | Image-based lighting (irradiance, prefiltered env, BRDF LUT) |
| Debug | `Graphics/Debug/` | 2 | 874 | Debug renderer (grid, axis, wireframe) |
| LOD | `Graphics/LOD/` | 4 | 784 | LOD generation and system |
| Terrain | `Graphics/Terrain/` | 2 | 704 | Terrain rendering with splat-map texturing |
| SSAO | `Graphics/SSAO/` | 2 | 630 | Screen-space ambient occlusion |
| SSGI | `Graphics/SSGI/` | 2 | 624 | Screen-space global illumination |
| Sky | `Graphics/Sky/` | 2 | 574 | Procedural sky rendering |
| Volumetric | `Graphics/Volumetric/` | 2 | 519 | Volumetric fog |
| CommandBuffer | `Graphics/CommandBuffer/` | 2 | 516 | Deferred command recording |
| SSR | `Graphics/SSR/` | 2 | 515 | Screen-space reflections |
| Water | `Graphics/Water/` | 2 | 541 | Water rendering |
| Culling | `Graphics/Culling/` | 4 | 527 | Frustum and GPU occlusion culling |
| Particle | `Graphics/Particle/` | 2 | 563 | Particle system |
| TAA | `Graphics/TAA/` | 2 | 490 | Temporal anti-aliasing |
| Performance | `Graphics/Performance/` | 2 | 309 | GPU timer, frame stats |
| Instanced | `Graphics/Instanced/` | 2 | 227 | GPU instanced rendering |

## Key Design Patterns

### Entity-Component System
- `KActor` owns `KActorComponent`s via `GetComponent<T>()`
- Components: `KStaticMeshComponent`, `KSkeletalMeshComponent`, `KLightComponent`, `KTerrainComponent`, `KWaterComponent`
- `KScene` manages actors with parent-child hierarchy
- `EComponentType`: Base(0), StaticMesh(1), SkeletalMesh(2), Water(3), Terrain(4), Light(5)

### Singletons
- `KEngine::GetInstance()` - global engine instance
- `KEngine::InitializeWithExternalHwnd()` - embeds engine in an external window handle
- `KEngine::RunApplication<T>()` - template factory method for app entry point
- `KInputManager` - keyboard, mouse, raw input, action mapping
- `KAudioManager` - XAudio2 audio engine
- `KDebugUI` - ImGui debug overlay

### Subsystem Interface
- `ISubsystem` - base interface with `Initialize()`, `Tick()`, `Shutdown()` lifecycle methods
- `ESubsystemState` enum: `Uninitialized`, `Initialized`, `Running`, `Shutdown`
- `KSubsystemRegistry` - type-safe registration via `Register<T>()` with `std::type_index` keys, ordered initialization and reverse-order shutdown
- `KAudioSubsystem` wraps `KAudioManager` as an ISubsystem adapter
- `KPhysicsSubsystem` wraps `KPhysicsWorld` as an ISubsystem adapter

### C#/C++ Interop
- `EngineInterop.dll` exposes 113 flat C functions (`extern "C"`)
- C# consumes via P/Invoke (`DllImport`, `CallingConvention.Cdecl`)
- API groups: Engine lifecycle (9), Scene management (8), Actor management (23), Camera (11), Renderer settings (17), Directional light (7), Point light (4), Spot light (2), Material (9), StaticMesh component (3), SkeletalMesh component (7), Model loading (7), Texture (2), DebugRenderer (4)

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

### Shader Architecture
- Two-class system: `KShader` (individual compiled shader) and `KShaderProgram` (combined VS+PS program)
- All shaders are compiled from **inline HLSL source strings** via `KShader::CompileFromString()` — there are NO `.hlsl` shader files in the repository
- `EShaderType` enum: `Vertex`, `Pixel`, `Geometry`, `Hull`, `Domain`, `Compute`
- `KShaderProgram` manages vertex + pixel shader pair, input layout, and constant buffer reflection

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
│   ├── EngineInterop.cs      # P/Invoke wrapper (1,249 lines, 113 DllImport declarations)
│   └── UndoRedoService.cs    # Undo/Redo system (276 lines, command pattern)
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
| C# lines of code | ~3,630 |
| XAML lines | ~749 |
| **Editor total** | **23 files, ~4,379 lines** |

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

### API Groups (113 exported functions)

| Group | Functions | Description |
|-------|----------:|-------------|
| Engine Lifecycle | 9 | Create, Destroy, Initialize, Tick, Render, Resize, InitializeEmbedded, GetSceneManager, GetRenderer |
| Scene Management | 8 | Create, Load, Save, SetActive, GetActive, Raycast, GetActorCount, GetActorAt |
| Actor Management | 23 | Create, Destroy, Transform (Get/Set Position/Rotation/Scale), Name (Get/Set), Components (Add, Get, Count, Name, Type), Visibility, Hierarchy (AddChild, GetChild, GetChildCount, GetParent) |
| Camera | 11 | GetMain, SetPosition, SetRotation, GetPosition, GetViewMatrix, GetProjectionMatrix, SetFOV, GetFOV, SetNearFar, GetNearZ, GetFarZ |
| Renderer Settings | 17 | RenderPath, DebugMode, Stats, SSAO, PostProcess, Shadow, Sky, TAA, DebugUI, SSR, VolumetricFog, ShadowSceneBounds, CascadedShadows (Set/IsEnabled), IBL (Set/IsEnabled), LoadEnvironmentMap |
| Directional Light | 7 | SetDirectionalLight, SetDirection, SetColor, SetAmbient, GetDirectionalLight, SetIntensity, GetIntensity |
| Point Light | 4 | AddPointLight, ClearPointLights, GetPointLightCount, GetPointLight |
| Spot Light | 2 | AddSpotLight, ClearSpotLights |
| Material | 9 | SetAlbedo, SetMetallic, SetRoughness, SetAO, GetAlbedo, GetMetallic, GetRoughness, GetAO, SetTexture |
| StaticMesh Component | 3 | SetMesh, GetMaterial, CreateDefaultMesh |
| SkeletalMesh Component | 7 | PlayAnimation, StopAnimation, GetAnimationCount, PauseAnimation, ResumeAnimation, GetAnimationName, SetSkeletalMeshFromModel |
| Model Loading | 7 | Load, Unload, LoadAndGetStaticMesh, LoadAndGetSkeletalMesh, HasSkeleton, GetAnimationCount, GetAnimationName |
| Texture | 2 | Load, Unload |
| DebugRenderer | 4 | DrawGrid, DrawAxis, SetEnabled, RenderFrame |

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
| UI | Canvas UI system elements, layout system | `samples/UI/` |
| UI Layout | Vertical, horizontal, grid layouts | `samples/UI/Layout/` |
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
| Engine source files (.h + .cpp) | 149 |
| Engine total lines | ~32,560 |
| Editor C# files | 14 |
| Editor XAML files | 9 |
| Editor C# lines | ~3,630 |
| Editor XAML lines | ~749 |
| Sample projects | 16 |
| Engine modules | 11 |
| Graphics sub-systems | 20 |
| EngineInterop API functions | 113 |
| Total solution projects | 19 |

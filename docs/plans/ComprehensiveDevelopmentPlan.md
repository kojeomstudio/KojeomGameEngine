# KojeomGameEngine Comprehensive Development Plan

## Document Information

- **Created**: 2026-03-02
- **Author**: AI Agent
- **Status**: In Progress
- **Base Commit**: fe09afb
- **Priority**: Renderer First
- **Last Updated**: 2026-03-02

## Overview

This document outlines the comprehensive development plan for KojeomGameEngine, covering renderer enhancements, asset management, animation systems, and editor development.

## Development Priorities

1. **Renderer Work** (Highest Priority) - ✅ Completed Phases 1-6
2. **Asset System** (Static/Skeletal Mesh, FBX Loading) - ✅ Completed
3. **Scene/Map Management** - ✅ Completed
4. **Serialization System** - ✅ Completed
5. **C# Editor** - 🔄 In Progress

---

## Part 1: Renderer Development

### Phase 6: Performance Optimization

**Status**: ✅ Completed
**Priority**: High
**Completion Date**: 2026-03-02

#### Tasks

| Task | Status | Description |
|------|--------|-------------|
| Frustum culling | ✅ | KFrustum class with plane extraction from VP matrix |
| Instanced rendering | ✅ | KInstancedRenderer for batch rendering |
| GPU query timers | ✅ | KGPUTimer with timestamp queries |
| Occlusion culling | 🔲 | Pending (future optimization) |
| Command buffer optimization | 🔲 | Pending (future optimization) |

#### Implementation Notes

- **KFrustum**: 6-plane frustum extraction, sphere/box intersection tests
- **KInstancedRenderer**: Dynamic instance buffer (default 1024 instances)
- **KGPUTimer**: GPU timestamp queries, frame stats tracking

#### New Files

- `Engine/Graphics/Culling/Frustum.h/cpp`
- `Engine/Graphics/Instanced/InstancedRenderer.h/cpp`
- `Engine/Graphics/Performance/GPUTimer.h/cpp`

---

### Phase 5: Post-Processing

**Status**: ✅ Completed
**Priority**: High
**Completion Date**: 2026-03-02

#### Tasks

| Task | Status | Description |
|------|--------|-------------|
| Post-process framework | ✅ | KPostProcessor class with HDR pipeline |
| HDR tonemapping | ✅ | ACES Filmic tonemapper |
| Bloom effect | ✅ | Bright extraction, Gaussian blur |
| FXAA | ✅ | Fast approximate anti-aliasing |
| Color grading | ✅ | 3D LUT (32x32x32), saturation, contrast, color filter |

#### Implementation Notes

- **KPostProcessor**: Full HDR post-processing pipeline
- **HDR Render Target**: R16G16B16A16_FLOAT format
- **Bloom**: Threshold extraction + separable Gaussian blur
- **Tonemapping**: ACES Filmic curve with exposure control
- **FXAA**: Standard implementation with configurable parameters

#### New Files

- `Engine/Graphics/PostProcess/PostProcessor.h/cpp`

---

### Phase 2: Shadow Mapping

**Status**: ✅ Completed  
**Target Completion**: 2026-03-02  
**Priority**: Critical

#### Tasks

| Task | Status | Description |
|------|--------|-------------|
| Shadow map render target | 🔲 | Create depth-only render target for shadows |
| Shadow shader | 🔲 | Depth-only vertex/pixel shader |
| Shadow pass renderer | 🔲 | Render scene from light's perspective |
| PCF filtering | 🔲 | Percentage Closer Filtering |
| Shadow constant buffer | 🔲 | Shadow transformation matrices |
| Shadow bias configuration | 🔲 | Reduce shadow acne |

#### Technical Specification

```cpp
// Shadow map configuration
struct FShadowConfig
{
    UINT32 Resolution = 2048;
    float DepthBias = 0.0001f;
    float SlopeScaledDepthBias = 1.0f;
    UINT32 PCFKernelSize = 3;
};

// Shadow constant buffer (b3)
struct FShadowBuffer
{
    XMMATRIX LightViewProjection;
    XMFLOAT2 ShadowMapSize;
    float DepthBias;
    float Padding;
};
```

---

### Phase 3: Deferred Rendering

**Status**: ✅ Completed
**Priority**: High

#### Tasks

| Task | Status | Description |
|------|--------|-------------|
| G-Buffer design | ✅ | KGBuffer class with AlbedoMetallic, NormalRoughness, PositionAO |
| G-Buffer render targets | ✅ | Multiple render target setup (3 RTs + Depth) |
| Geometry pass | ✅ | Fill G-Buffer via KDeferredRenderer |
| Lighting pass | ✅ | Full-screen quad deferred lighting calculation |
| Forward+ transparency | ✅ | FForwardTransparentRenderData for transparent objects |

#### Implementation Notes

- KGBuffer: Manages 3 render targets with RTV and SRV
- KDeferredRenderer: Geometry pass and lighting pass coordination
- KRenderer: Added ERenderPath enum and SetRenderPath() for switching

---

### Phase 4: PBR Materials

**Status**: ✅ Completed
**Priority**: High
**Completion Date**: 2026-03-02

#### Tasks

| Task | Status | Description |
|------|--------|-------------|
| PBR shader | ✅ | Cook-Torrance BRDF with GGX distribution |
| Material system | ✅ | FPBRMaterialParams, KMaterial class |
| IBL support | ✅ | KIBLSystem with BRDF LUT generation |
| Material editor UI | 🔲 | Pending for C# editor |

#### Implementation Notes

- **FPBRMaterialParams**: Albedo, Metallic, Roughness, AO, Emissive properties
- **KMaterial**: Material management with texture slots (7 texture types)
- **KIBLSystem**: Image-based lighting with BRDF LUT
- **PBRShader**: Metalness/Roughness workflow with HDR tonemapping
- Pre-defined materials: Default, Metal, Plastic, Rubber, Gold, Silver, Copper

#### New Files

- `Engine/Graphics/Material.h/cpp`
- `Engine/Graphics/IBL/IBLSystem.h/cpp`

---

## Part 2: Asset System

### 2.1 Static Mesh System

**Status**: ✅ Completed
**Priority**: High
**Completion Date**: 2026-03-02 (Previous)

#### Components

| Component | Description |
|-----------|-------------|
| KStaticMesh | Static mesh container with LOD support |
| KStaticMeshComponent | Actor component for rendering |
| FMeshSection | Sub-mesh with material slot |
| FMeshLOD | Level of detail data |

---

### 2.2 Skeletal Mesh System

**Status**: ✅ Completed
**Priority**: High
**Completion Date**: 2026-03-02

#### Components

| Component | Description |
|-----------|-------------|
| KSkeleton | Bone hierarchy with bind poses |
| KAnimation | Animation data with keyframes |
| KAnimationInstance | Runtime animation playback |

#### Implementation Notes

- **KSkeleton**: Bone hierarchy with bind pose and inverse bind pose matrices
- **KAnimation**: Keyframe animation with position/rotation/scale channels
- **KAnimationInstance**: Runtime animation playback with multiple play modes

#### New Files

- `Engine/Assets/Skeleton.h/cpp`
- `Engine/Assets/Animation.h/cpp`
- `Engine/Assets/AnimationInstance.h/cpp`

#### Features

- Up to 256 bones per skeleton
- Quaternion-based rotation interpolation (SLERP)
- Position/Scale interpolation (LERP)
- Play modes: Once, Loop, PingPong
- Playback speed control
- Multiple animations per instance

---

### 2.3 Model Loader

**Status**: ✅ Completed
**Priority**: High
**Completion Date**: 2026-03-02

#### Components

| Component | Description |
|-----------|-------------|
| KModelLoader | FBX/OBJ model loading |
| FLoadedModel | Loaded model container |

#### Implementation Notes

- **KModelLoader**: Supports Assimp (optional) and built-in OBJ parser
- **FLoadedModel**: Contains StaticMesh, Skeleton, and Animations
- Built-in OBJ parser for basic model loading without Assimp
- Configurable options: scale, UV flip, normal generation, tangent generation

#### New Files

- `Engine/Assets/ModelLoader.h/cpp`

#### Features

- OBJ format support (built-in parser)
- FBX/GLTF/DAE support (with Assimp library)
- Model caching to avoid duplicate loading
- Animation loading support (with Assimp)
- Material slot extraction

---

## Part 3: Scene/Map Management

**Status**: ✅ Completed
**Priority**: Medium
**Completion Date**: 2026-03-02

### Components

| Component | Status | Description |
|-----------|--------|-------------|
| KActor | ✅ | Base game object with transform, hierarchy |
| KActorComponent | ✅ | Component base class |
| KScene | ✅ | Scene graph container with save/load |
| KSceneManager | ✅ | Scene management with multiple scenes |

### Implementation Notes

- **KActor**: Full hierarchy support (parent/children), component system
- **KScene**: Binary serialization for save/load
- **KSceneManager**: Multiple scene support, active scene management
- Added `FTransform` struct for position/rotation/scale

### New Files

- `Engine/Scene/Actor.h/cpp` - Actor and Scene classes
- `Engine/Scene/SceneManager.h/cpp` - Scene manager

### Architecture

```cpp
// Base actor
class KActor
{
public:
    virtual void BeginPlay();
    virtual void Tick(float DeltaTime);
    virtual void Render(KRenderer* Renderer);
    
    void AddComponent(std::shared_ptr<KActorComponent> Component);
    template<typename T>
    T* GetComponent() const;
    
    void SetWorldTransform(const XMMATRIX& Matrix);
    XMMATRIX GetWorldTransform() const;
    
private:
    std::string Name;
    XMMATRIX WorldTransform;
    std::vector<std::shared_ptr<KActorComponent>> Components;
    std::vector<std::shared_ptr<KActor>> Children;
    KActor* Parent;
};

// Scene container
class KScene
{
public:
    HRESULT Load(const std::wstring& Path);
    HRESULT Save(const std::wstring& Path);
    
    void AddActor(std::shared_ptr<KActor> Actor);
    void RemoveActor(const std::string& Name);
    KActor* FindActor(const std::string& Name) const;
    
    void Tick(float DeltaTime);
    void Render(KRenderer* Renderer);
    
private:
    std::string Name;
    std::vector<std::shared_ptr<KActor>> Actors;
};
```

---

## Part 4: Serialization System

**Status**: ✅ Completed
**Priority**: Medium
**Completion Date**: 2026-03-02

### Components

| Component | Status | Description |
|-----------|--------|-------------|
| KBinaryArchive | ✅ | Binary file read/write |
| KJsonArchive | ✅ | JSON serialization for editor |

### Implementation Notes

- **KBinaryArchive**: File-based binary serialization with streaming support
- **KJsonArchive**: Pure C++ JSON parser/serializer (no external dependencies)
- JSON types: KJsonValue, KJsonObject, KJsonArray, KJsonString, KJsonNumber, KJsonBool, KJsonNull

### New Files

- `Engine/Serialization/BinaryArchive.h/cpp` - Binary serialization
- `Engine/Serialization/JsonArchive.h/cpp` - JSON serialization

### Architecture

```cpp
// Binary serialization
class KBinaryArchive
{
public:
    void Serialize(const std::wstring& Path);
    void Deserialize(const std::wstring& Path);
    
    KBinaryArchive& operator<<(int32 Value);
    KBinaryArchive& operator<<(uint32 Value);
    KBinaryArchive& operator<<(float Value);
    KBinaryArchive& operator<<(const std::string& Value);
    KBinaryArchive& operator<<(const XMMATRIX& Value);
    
    KBinaryArchive& operator>>(int32& Value);
    KBinaryArchive& operator>>(uint32& Value);
    KBinaryArchive& operator>>(float& Value);
    KBinaryArchive& operator>>(std::string& Value);
    KBinaryArchive& operator>>(XMMATRIX& Value);
    
private:
    std::vector<uint8> Buffer;
    size_t ReadPosition;
};

// JSON serialization (for editor)
class KJsonArchive
{
public:
    void Save(const std::wstring& Path);
    void Load(const std::wstring& Path);
    
    // JSON-specific operations
    void WriteValue(const std::string& Key, const std::string& Value);
    void WriteValue(const std::string& Key, float Value);
    void WriteArray(const std::string& Key, const std::vector<float>& Values);
    
private:
    // JSON library integration (nlohmann/json)
};
```

---

## Part 5: C# Editor

**Status**: 🔄 In Progress
**Priority**: Medium
**Framework**: .NET 8.0 WPF
**Completion Date**: Partial - 2026-03-02

### Components

| Component | Status | Description |
|-----------|--------|-------------|
| MainWindow | ✅ | Main editor window with menu/toolbar |
| ViewportControl | ✅ | Viewport with engine rendering and camera controls |
| SceneHierarchyControl | ✅ | Scene tree view with selection |
| PropertiesPanelControl | ✅ | Actor properties display |
| ContentBrowserControl | ✅ | Asset browser with folder navigation |
| EngineInterop | ✅ | P/Invoke bindings to Engine |
| EngineInterop DLL | ✅ | C++ DLL for engine exports |

### Features

1. **Viewport Panel** (Complete)
   - ✅ Engine interop for rendering
   - ✅ Resize handling
   - ✅ Camera controls (WASD movement, mouse rotation)
   - 🔲 Object selection via picking

2. **Scene Hierarchy** (Partial)
   - ✅ Tree view of actors
   - ✅ Add/Remove actors (via ViewModel)
   - ✅ Actor selection sync with Properties panel
   - 🔲 Drag-and-drop reordering

3. **Properties Panel** (Partial)
   - ✅ Transform properties (Position, Rotation, Scale)
   - 🔲 Component properties
   - 🔲 Material editor

4. **Content Browser** (Partial)
   - ✅ Asset browser with folder tree view
   - ✅ Asset grid view with icons
   - ✅ Import assets dialog
   - 🔲 Thumbnail previews
   - 🔲 Drag-and-drop to viewport

5. **Toolbar** (Partial)
   - ✅ Play/Pause/Stop buttons
   - ✅ Save/Load scene menu
   - 🔲 Undo/Redo

### Implementation Notes

- **EngineInterop.cs**: P/Invoke bindings to C++ Engine API
- **EngineAPI.h/cpp**: C-exported functions for engine access
- **FEngineWrapper**: C++ wrapper class for engine components
- Scene management through KSceneManager
- Actor manipulation through KScene/KActor

### New Files

- `Editor/KojeomEditor/Services/EngineInterop.cs` - P/Invoke bindings
- `Editor/KojeomEditor/Views/ViewportControl.xaml(.cs)` - Viewport with camera controls
- `Editor/KojeomEditor/Views/SceneHierarchyControl.xaml(.cs)` - Scene tree with selection
- `Editor/KojeomEditor/Views/PropertiesPanelControl.xaml(.cs)` - Properties panel
- `Editor/KojeomEditor/Views/ContentBrowserControl.xaml(.cs)` - Content browser panel
- `Editor/EngineInterop/EngineAPI.h` - C API header
- `Editor/EngineInterop/EngineAPI.cpp` - C API implementation
- `Editor/EngineInterop/EngineInterop.vcxproj` - C++ DLL project

### Project Structure

```
Editor/
├── KojeomEditor.sln
├── KojeomEditor/
│   ├── KojeomEditor.csproj
│   ├── App.xaml(.cs)
│   ├── MainWindow.xaml(.cs)
│   ├── ViewModels/
│   │   ├── MainViewModel.cs
│   │   ├── SceneViewModel.cs
│   │   └── PropertiesViewModel.cs
│   ├── Views/
│   │   ├── ViewportControl.xaml(.cs)
│   │   ├── SceneHierarchy.xaml(.cs)
│   │   └── PropertiesPanel.xaml(.cs)
│   ├── Models/
│   │   ├── SceneModel.cs
│   │   └── ActorModel.cs
│   └── Services/
│       ├── EngineInterop.cs
│       └── AssetService.cs
└── KojeomEditor.EngineHost/
    └── (C++/CLI wrapper for engine)
```

### Engine Interop

```csharp
// C# side
public class EngineInterop
{
    [DllImport("KojeomEngine.dll")]
    public static extern IntPtr CreateEngine();
    
    [DllImport("KojeomEngine.dll")]
    public static extern void DestroyEngine(IntPtr engine);
    
    [DllImport("KojeomEngine.dll")]
    public static extern void RenderFrame(IntPtr engine);
    
    [DllImport("KojeomEngine.dll")]
    public static extern IntPtr LoadScene(IntPtr engine, string path);
}
```

---

## Implementation Schedule

### Week 1: Renderer Phase 2 (Shadow Mapping)

- Day 1-2: Shadow map render target and depth shader
- Day 3-4: Shadow pass implementation
- Day 5: PCF filtering and integration

### Week 2: Asset System Foundation

- Day 1-2: Serialization system
- Day 3-4: Static mesh system with serialization
- Day 5: Integration and testing

### Week 3: FBX Loading & Skeletal Mesh

- Day 1-2: Assimp integration and static mesh loading
- Day 3-4: Skeletal mesh and skeleton system
- Day 5: Animation system

### Week 4: Scene System

- Day 1-2: Actor and component system
- Day 3-4: Scene save/load
- Day 5: Integration testing

### Week 5-6: C# Editor

- Day 1-2: Project setup and basic WPF structure
- Day 3-4: Engine interop and viewport rendering
- Day 5-6: Scene hierarchy and properties panel
- Day 7-8: Content browser and asset import
- Day 9-10: Polish and testing

---

## Dependencies

### External Libraries

| Library | Purpose | Integration |
|---------|---------|-------------|
| Assimp | FBX/OBJ model loading | vcpkg |
| nlohmann/json | JSON serialization | vcpkg |
| DirectXTK | Texture loading, utilities | vcpkg |

### Build Requirements

- Visual Studio 2022
- Windows SDK 10.0
- .NET 8.0 SDK
- vcpkg package manager

---

## File Structure (Planned)

```
Engine/
├── Core/
│   ├── Engine.h/cpp
│   └── EnginePCH.h
├── Graphics/
│   ├── Renderer.h/cpp
│   ├── GraphicsDevice.h/cpp
│   ├── Camera.h/cpp
│   ├── Shader.h/cpp
│   ├── Mesh.h/cpp
│   ├── Texture.h/cpp
│   ├── Light.h
│   ├── Shadow/
│   │   ├── ShadowMap.h/cpp
│   │   └── ShadowRenderer.h/cpp
│   ├── Deferred/
│   │   ├── GBuffer.h/cpp
│   │   └── DeferredRenderer.h/cpp
│   ├── IBL/
│   │   └── IBLSystem.h/cpp
│   ├── PostProcess/
│   │   └── PostProcessor.h/cpp
│   ├── Culling/
│   │   └── Frustum.h/cpp
│   ├── Instanced/
│   │   └── InstancedRenderer.h/cpp
│   ├── Performance/
│   │   └── GPUTimer.h/cpp
│   ├── Material.h/cpp
│   └── ...
├── Assets/
│   ├── StaticMesh.h/cpp
│   ├── SkeletalMesh.h/cpp
│   ├── Skeleton.h/cpp
│   ├── Animation.h/cpp
│   ├── Material.h/cpp
│   └── ModelLoader.h/cpp
├── Scene/
│   ├── Actor.h/cpp
│   ├── ActorComponent.h/cpp
│   ├── Scene.h/cpp
│   └── SceneManager.h/cpp
├── Serialization/
│   ├── Archive.h/cpp
│   ├── BinaryArchive.h/cpp
│   └── JsonArchive.h/cpp
└── Utils/
    ├── Common.h
    ├── Logger.h
    └── Math.h

Editor/
├── KojeomEditor/
│   ├── KojeomEditor.csproj
│   └── ...
└── KojeomEditor.EngineHost/
    └── ...
```

---

## Testing Strategy

### Unit Tests

- Serialization/Deserialization tests
- Matrix and math operations
- Asset loading validation

### Integration Tests

- Full render pipeline test
- Scene load/save cycle
- Editor-Engine communication

### Visual Tests

- Shadow quality comparison
- Animation blending verification
- Deferred rendering output

---

## Notes

- All code must follow the guidelines in `docs/rules/AIAgentRules.md`
- Update this document as development progresses
- Commit early and often with descriptive messages
- Run compile tests after each significant change

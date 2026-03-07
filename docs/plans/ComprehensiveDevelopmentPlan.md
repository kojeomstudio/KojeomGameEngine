# KojeomGameEngine Comprehensive Development Plan

## Document Information

- **Created**: 2026-03-02
- **Author**: AI Agent
- **Status**: Completed
- **Base Commit**: fe09afb
- **Priority**: Renderer First
- **Last Updated**: 2026-03-03

## Overview

This document outlines the comprehensive development plan for KojeomGameEngine, covering renderer enhancements, asset management, animation systems, and editor development.

## Development Priorities

1. **Renderer Work** (Highest Priority) - ✅ Completed Phases 1-18 (Terrain System Added)
2. **Asset System** (Static/Skeletal Mesh, FBX Loading) - ✅ Completed
3. **Scene/Map Management** - ✅ Completed
4. **Serialization System** - ✅ Completed
5. **C# Editor** - ✅ Completed

---

## Part 1: Renderer Development

### Phase 7: Screen Space Ambient Occlusion

**Status**: ✅ Completed
**Priority**: High
**Completion Date**: 2026-03-02

#### Tasks

| Task | Status | Description |
|------|--------|-------------|
| SSAO kernel generation | ✅ | 64 samples with hemisphere distribution |
| Noise texture generation | ✅ | 4x4 random rotation texture |
| SSAO shader | ✅ | SSAO computation with bias and radius |
| Blur shader | ✅ | Separable Gaussian blur for SSAO |
| Renderer integration | ✅ | KSSAO class with Deferred rendering support |

#### Implementation Notes

- **KSSAO**: Full SSAO system with configurable parameters
- **SSAO Kernel**: 64-sample hemisphere distribution with randomization
- **Noise Texture**: 4x4 rotation texture for random sample rotation
- **Blur**: Separable Gaussian blur for smooth AO results
- **Integration**: Works with Deferred rendering pipeline

#### New Files

- `Engine/Graphics/SSAO/SSAO.h/cpp`

---

### Phase 8: Screen Space Reflections

**Status**: ✅ Completed
**Priority**: High
**Completion Date**: 2026-03-02

#### Tasks

| Task | Status | Description |
|------|--------|-------------|
| SSR ray marching | ✅ | Ray marching with depth buffer for hit detection |
| Fresnel effect | ✅ | Fresnel-Schlick approximation for realistic reflections |
| Edge fade | ✅ | Screen edge fade factor for smooth boundaries |
| Renderer integration | ✅ | KSSR class with Deferred rendering support |

#### Implementation Notes

- **KSSR**: Full SSR system with configurable parameters
- **Ray Marching**: Step-based ray marching with thickness check
- **Fresnel**: Fresnel-Schlick approximation for reflection intensity
- **Integration**: Works with Deferred rendering pipeline

#### New Files

- `Engine/Graphics/SSR/SSR.h/cpp`

---

### Phase 9: Temporal Anti-Aliasing

**Status**: ✅ Completed
**Priority**: High
**Completion Date**: 2026-03-02

#### Tasks

| Task | Status | Description |
|------|--------|-------------|
| History buffer | ✅ | Double-buffered history textures |
| TAA shader | ✅ | Temporal reprojection and blending |
| YCoCg clipping | ✅ | Color space clipping for ghosting reduction |
| Sharpening | ✅ | Optional sharpening pass |

#### Implementation Notes

- **KTAA**: Full TAA system with history management
- **YCoCg**: Color space conversion for better ghosting reduction
- **Sharpening**: Optional post-TAA sharpening
- **Integration**: Works with Deferred rendering pipeline

#### New Files

- `Engine/Graphics/TAA/TAA.h/cpp`

---

### Phase 10: Volumetric Fog

**Status**: ✅ Completed
**Priority**: Medium
**Completion Date**: 2026-03-02

#### Tasks

| Task | Status | Description |
|------|--------|-------------|
| Height fog | ✅ | Exponential height-based fog density |
| Volumetric lighting | ✅ | In-scattering with light contribution |
| Ray marching | ✅ | Step-based fog computation |
| Henyey-Greenstein phase | ✅ | Anisotropic scattering |

#### Implementation Notes

- **KVolumetricFog**: Full volumetric fog system
- **Height Fog**: Exponential density falloff with height
- **In-Scattering**: Volumetric light contribution with phase function
- **Integration**: Works with Deferred rendering pipeline

#### New Files

- `Engine/Graphics/Volumetric/VolumetricFog.h/cpp`

---

### Phase 11: Cascaded Shadow Maps

**Status**: ✅ Completed
**Priority**: High
**Completion Date**: 2026-03-02

#### Tasks

| Task | Status | Description |
|------|--------|-------------|
| CSM texture array | ✅ | KCascadedShadowMap with up to 4 cascades |
| Cascade split calculation | ✅ | Practical/Logarithmic split scheme |
| CSM renderer | ✅ | KCascadedShadowRenderer with per-cascade rendering |
| Frustum-aligned cascade bounds | ✅ | Automatic cascade bounds from camera frustum |

#### Implementation Notes

- **KCascadedShadowMap**: Manages up to 4 shadow map cascades with texture array
- **KCascadedShadowRenderer**: Full CSM rendering pipeline
- **Split Scheme**: Configurable lambda blend between practical and logarithmic
- **Integration**: Extends shadow system for large outdoor scenes

#### New Files

- `Engine/Graphics/Shadow/CascadedShadowMap.h/cpp`
- `Engine/Graphics/Shadow/CascadedShadowRenderer.h/cpp`

---

### Phase 12: Screen Space Global Illumination

**Status**: ✅ Completed
**Priority**: High
**Completion Date**: 2026-03-03

#### Tasks

| Task | Status | Description |
|------|--------|-------------|
| SSGI kernel generation | ✅ | 16 samples with hemisphere distribution |
| Noise texture generation | ✅ | 4x4 random rotation texture |
| SSGI shader | ✅ | Ray marching with indirect lighting |
| Blur shader | ✅ | Separable Gaussian blur for SSGI |
| Renderer integration | ✅ | KSSGI class with Deferred rendering support |

#### Implementation Notes

- **KSSGI**: Full SSGI system with configurable parameters
- **SSGI Kernel**: 16-sample hemisphere distribution with randomization
- **Noise Texture**: 4x4 rotation texture for random sample rotation
- **Blur**: Separable Gaussian blur for smooth GI results
- **Integration**: Works with Deferred rendering pipeline

#### New Files

- `Engine/Graphics/SSGI/SSGI.h/cpp`

---

### Phase 13: Particle System

**Status**: ✅ Completed
**Priority**: High
**Completion Date**: 2026-03-03

#### Tasks

| Task | Status | Description |
|------|--------|-------------|
| Particle structure | ✅ | FParticle struct with position, velocity, color, size |
| Particle emitter | ✅ | KParticleEmitter with emission shapes |
| Particle shader | ✅ | Billboard rendering with additive blending |
| Texture atlas support | ✅ | Animated textures support |

#### Implementation Notes

- **KParticleEmitter**: Full particle system with configurable parameters
- **Emission Shapes**: Point, Sphere, Cone, Box
- **Billboard Rendering**: Particles always face camera
- **Additive Blending**: Fire/smoke effects
- **Texture Atlas**: Animated particle textures

#### New Files

- `Engine/Graphics/Particle/ParticleEmitter.h/cpp`

---

### Phase 18: Terrain System

**Status**: ✅ Completed
**Priority**: High
**Completion Date**: 2026-03-07

#### Tasks

| Task | Status | Description |
|------|--------|-------------|
| Height map generation | ✅ | Perlin noise, raw file loading |
| Terrain mesh generation | ✅ | FTerrainVertex with position, normal, texcoord |
| LOD system | ✅ | Up to 4 LOD levels with distance-based switching |
| TerrainComponent | ✅ | KTerrainComponent for scene integration |
| Splat layer support | ✅ | FSplatLayer with diffuse/normal textures |

#### Implementation Notes

- **KHeightMap**: Height map loading and procedural generation
- **KTerrain**: Terrain mesh with LOD support
- **KTerrainComponent**: Actor component for scene integration
- **LOD System**: Configurable LOD distances for performance
- **Height Queries**: GetHeightAtWorldPosition for physics integration

#### New Files

- `Engine/Graphics/Terrain/Terrain.h/cpp`

---

### Phase 14: Skeletal Mesh Rendering

**Status**: ✅ Completed
**Priority**: High
**Completion Date**: 2026-03-07

#### Tasks

| Task | Status | Description |
|------|--------|-------------|
| SkeletalMeshComponent | ✅ | KSkeletalMesh and KSkeletalMeshComponent classes |
| GPU skinning shader | ✅ | Vertex shader with bone matrix palette |
| Bone matrix buffer | ✅ | FBoneMatrixBuffer (b5) with 256 bones |
| Renderer integration | ✅ | RenderSkeletalMesh method in KRenderer |
| Animation playback | ✅ | Integration with KAnimationInstance |

#### Implementation Notes

- **FSkinnedVertex**: Vertex with 4 bone influences and weights
- **KSkeletalMesh**: Skeletal mesh container with skinned vertex data
- **KSkeletalMeshComponent**: Actor component for rendering skeletal meshes
- **GPU Skinning**: Bone transforms applied in vertex shader
- **Animation**: Uses existing KAnimationInstance for playback

#### New Files

- `Engine/Assets/SkeletalMeshComponent.h/cpp`

---

### Phase 15: Enhanced Model Loading with FBX/GLTF Support

**Status**: ✅ Completed
**Priority**: High
**Completion Date**: 2026-03-07

#### Tasks

| Task | Status | Description |
|------|--------|-------------|
| Fix OBJ loader bug | ✅ | Mesh vertices/indices now properly set |
| Full Assimp integration | ✅ | Complete Assimp implementation for FBX/GLTF |
| Skeleton extraction from FBX | ✅ | BuildSkeletonRecursive with bind poses |
| Animation extraction | ✅ | Full animation channel extraction |
| Bone weight extraction | ✅ | Vertex bone weights with normalization |
| GLTF fallback loader | ✅ | Basic GLTF/JSON parsing without Assimp |
| StaticMesh LOD methods | ✅ | SetLODData, AddLOD methods |

#### Implementation Notes

- **KModelLoader**: Complete rewrite with full Assimp integration
- **Skeleton Extraction**: Recursive skeleton building with bind poses
- **Animation**: Position/rotation/scale key extraction
- **Bone Weights**: 4-bone limit with automatic normalization
- **GLTF Fallback**: Basic parser when Assimp not available

#### Modified Files

- `Engine/Assets/ModelLoader.h/cpp` - Complete Assimp integration
- `Engine/Assets/StaticMesh.h/cpp` - Added LOD management methods

---

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
| Occlusion culling | ✅ | KOcclusionQuery and KOcclusionCuller with hardware occlusion queries |
| Command buffer optimization | ✅ | KCommandBuffer with state caching and sort key rendering |

#### Implementation Notes

- **KFrustum**: 6-plane frustum extraction, sphere/box intersection tests
- **KInstancedRenderer**: Dynamic instance buffer (default 1024 instances)
- **KGPUTimer**: GPU timestamp queries, frame stats tracking
- **KOcclusionCuller**: Hardware occlusion queries with D3D11_QUERY_OCCLUSION
- **KCommandBuffer**: Render command batching with sort key optimization (Shader, Texture, Material, Depth sorting)

#### New Files

- `Engine/Graphics/Culling/Frustum.h/cpp`
- `Engine/Graphics/Culling/OcclusionQuery.h/cpp`
- `Engine/Graphics/CommandBuffer/CommandBuffer.h/cpp`
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
| Shadow map render target | ✅ | KShadowMap class with depth texture |
| Shadow shader | ✅ | Depth-only vertex/pixel shader |
| Shadow pass renderer | ✅ | KShadowRenderer with scene bounds |
| PCF filtering | ✅ | 3x3 PCF in pixel shader |
| Shadow constant buffer | ✅ | FShadowBuffer (b2) |
| Shadow bias configuration | ✅ | Configurable depth and slope bias |

#### Implementation Notes

- **KShadowMap**: Depth-only render target (2048x2048 default)
- **KShadowRenderer**: Shadow pass coordination with automatic scene bounds
- **FShadowBuffer**: Light view-projection, shadow map size, depth bias, PCF kernel

#### New Files

- `Engine/Graphics/Shadow/ShadowMap.h/cpp`
- `Engine/Graphics/Shadow/ShadowRenderer.h/cpp`

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

**Status**: ✅ Completed
**Priority**: Medium
**Framework**: .NET 8.0 WPF
**Completion Date**: 2026-03-02

### Components

| Component | Status | Description |
|-----------|--------|-------------|
| MainWindow | ✅ | Main editor window with menu/toolbar |
| ViewportControl | ✅ | Viewport with engine rendering and camera controls |
| SceneHierarchyControl | ✅ | Scene tree view with selection |
| PropertiesPanelControl | ✅ | Actor properties display with component support |
| ContentBrowserControl | ✅ | Asset browser with folder navigation |
| MaterialEditorControl | ✅ | PBR material editor with presets |
| EngineInterop | ✅ | P/Invoke bindings to Engine |
| EngineInterop DLL | ✅ | C++ DLL for engine exports |
| UndoRedoService | ✅ | Undo/Redo command system |

### Features

1. **Viewport Panel** (Complete)
   - ✅ Engine interop for rendering
   - ✅ Resize handling
   - ✅ Camera controls (WASD movement, mouse rotation)
   - ✅ Object selection via picking (raycast)

2. **Scene Hierarchy** (Complete)
    - ✅ Tree view of actors
    - ✅ Add/Remove actors (via ViewModel)
    - ✅ Actor selection sync with Properties panel
    - ✅ Drag-and-drop reordering

3. **Properties Panel** (Complete)
   - ✅ Transform properties (Position, Rotation, Scale)
   - ✅ Component properties (StaticMesh, Light, Camera)
   - ✅ General properties (Name, Visibility)

4. **Material Editor** (Complete)
    - ✅ PBR material parameters (Albedo, Metallic, Roughness, AO)
    - ✅ Material presets (Default, Metal, Plastic, Rubber, Gold, Silver, Copper)
    - ✅ Texture slot management
    - ✅ Texture file picker

5. **Content Browser** (Complete)
   - ✅ Asset browser with folder tree view
   - ✅ Asset grid view with icons
   - ✅ Import assets dialog
   - ✅ Thumbnail previews for image files
   - ✅ Drag-and-drop to viewport for actor spawning

6. **Toolbar** (Complete)
   - ✅ Play/Pause/Stop buttons
   - ✅ Save/Load scene menu
   - ✅ Undo/Redo (via UndoRedoService)

### Implementation Notes

- **EngineInterop.cs**: P/Invoke bindings to C++ Engine API
- **EngineAPI.h/cpp**: C-exported functions for engine access
- **FEngineWrapper**: C++ wrapper class for engine components
- **UndoRedoService**: Generic undo/redo system with IUndoableAction interface
- Scene management through KSceneManager
- Actor manipulation through KScene/KActor

### New Files

- `Editor/KojeomEditor/Services/EngineInterop.cs` - P/Invoke bindings
- `Editor/KojeomEditor/Services/UndoRedoService.cs` - Undo/Redo system
- `Editor/KojeomEditor/Views/ViewportControl.xaml(.cs)` - Viewport with camera controls and picking
- `Editor/KojeomEditor/Views/SceneHierarchyControl.xaml(.cs)` - Scene tree with selection
- `Editor/KojeomEditor/Views/PropertiesPanelControl.xaml(.cs)` - Properties panel with components
- `Editor/KojeomEditor/Views/ContentBrowserControl.xaml(.cs)` - Content browser panel
- `Editor/KojeomEditor/Views/MaterialEditorControl.xaml(.cs)` - Material editor
- `Editor/KojeomEditor/ViewModels/ComponentViewModel.cs` - Component ViewModels
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
│   ├── SSAO/
│   │   └── SSAO.h/cpp
│   ├── SSR/
│   │   └── SSR.h/cpp
│   ├── TAA/
│   │   └── TAA.h/cpp
│   ├── Volumetric/
│   │   └── VolumetricFog.h/cpp
│   ├── SSGI/
│   │   └── SSGI.h/cpp
│   ├── Particle/
│   │   └── ParticleEmitter.h/cpp
│   ├── Material.h/cpp
│   └── ...
├── Assets/
│   ├── StaticMesh.h/cpp
│   ├── StaticMeshComponent.h/cpp
│   ├── SkeletalMeshComponent.h/cpp
│   ├── Skeleton.h/cpp
│   ├── Animation.h/cpp
│   ├── AnimationInstance.h/cpp
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

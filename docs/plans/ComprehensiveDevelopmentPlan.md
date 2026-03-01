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

1. **Renderer Work** (Highest Priority)
2. **Asset System** (Static/Skeletal Mesh, FBX Loading)
3. **Scene/Map Management**
4. **Serialization System**
5. **C# Editor**

---

## Part 1: Renderer Development

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
| Color grading | 🔲 | Basic gamma implemented, advanced LUT pending |

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

**Status**: 🔄 In Progress
**Target Completion**: 2026-03-05
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
| Forward+ transparency | 🔲 | Alpha blending support (pending) |

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

**Status**: 🔲 Not Started
**Priority**: High

#### Components

| Component | Description |
|-----------|-------------|
| KStaticMesh | Static mesh container with LOD support |
| KStaticMeshComponent | Actor component for rendering |
| FMeshSection | Sub-mesh with material slot |
| FMeshLOD | Level of detail data |

#### Architecture

```cpp
// Static mesh with LOD support
class KStaticMesh
{
public:
    HRESULT LoadFromFile(const std::wstring& Path);
    HRESULT SaveToFile(const std::wstring& Path);
    void Render(ID3D11DeviceContext* Context, UINT32 LODLevel = 0);
    
private:
    std::vector<FMeshLOD> LODs;
    std::vector<FMaterialSlot> MaterialSlots;
    FBoundingBox BoundingBox;
    FSphere BoundingSphere;
};

struct FMeshLOD
{
    std::shared_ptr<KMesh> Mesh;
    float ScreenSize; // LOD transition threshold
};
```

---

### 2.2 Skeletal Mesh System

**Status**: 🔲 Not Started
**Priority**: High

#### Components

| Component | Description |
|-----------|-------------|
| KSkeleton | Bone hierarchy |
| KSkeletalMesh | Mesh with bone weights |
| KAnimation | Animation data |
| KAnimationInstance | Runtime animation playback |
| KSkeletalMeshComponent | Actor component |

#### Architecture

```cpp
// Bone structure
struct FBone
{
    std::string Name;
    INT32 ParentIndex;
    XMMATRIX BindPose;
    XMMATRIX InverseBindPose;
};

// Skeleton
class KSkeleton
{
public:
    UINT32 GetBoneIndex(const std::string& Name) const;
    const FBone& GetBone(UINT32 Index) const;
    UINT32 GetBoneCount() const { return Bones.size(); }
    
private:
    std::vector<FBone> Bones;
    std::unordered_map<std::string, UINT32> BoneNameToIndex;
};

// Animation keyframe
struct FTransformKey
{
    float Time;
    XMFLOAT3 Position;
    XMFLOAT4 Rotation; // Quaternion
    XMFLOAT3 Scale;
};

// Animation channel (per bone)
struct FAnimationChannel
{
    UINT32 BoneIndex;
    std::vector<FTransformKey> PositionKeys;
    std::vector<FTransformKey> RotationKeys;
    std::vector<FTransformKey> ScaleKeys;
};

// Animation data
class KAnimation
{
public:
    HRESULT LoadFromFile(const std::wstring& Path);
    float GetDuration() const { return Duration; }
    float GetTicksPerSecond() const { return TicksPerSecond; }
    
private:
    std::string Name;
    float Duration;
    float TicksPerSecond;
    std::vector<FAnimationChannel> Channels;
};

// Runtime animation playback
class KAnimationInstance
{
public:
    void PlayAnimation(std::shared_ptr<KAnimation> Anim);
    void Update(float DeltaTime);
    const std::vector<XMMATRIX>& GetBoneMatrices() const;
    
private:
    std::shared_ptr<KAnimation> CurrentAnimation;
    float CurrentTime;
    std::vector<XMMATRIX> BoneMatrices;
};
```

---

### 2.3 FBX Loader

**Status**: 🔲 Not Started
**Priority**: High

#### Dependencies

- **Assimp** (Open Asset Import Library) for FBX/OBJ/FBX loading

#### Components

| Component | Description |
|-----------|-------------|
| KModelLoader | Generic model loading interface |
| KAssimpLoader | Assimp-based implementation |
| KFBXLoader | FBX-specific loader (optional) |

#### Architecture

```cpp
class KModelLoader
{
public:
    static std::shared_ptr<KStaticMesh> LoadStaticMesh(const std::wstring& Path);
    static std::shared_ptr<KSkeletalMesh> LoadSkeletalMesh(const std::wstring& Path);
    static std::shared_ptr<KAnimation> LoadAnimation(const std::wstring& Path, UINT32 AnimationIndex = 0);
    
private:
    static HRESULT ProcessAssimpScene(const aiScene* Scene, ...);
    static HRESULT ProcessMesh(aiMesh* Mesh, ...);
    static HRESULT ProcessSkeleton(aiNode* Node, ...);
    static HRESULT ProcessAnimation(aiAnimation* Anim, ...);
};
```

---

## Part 3: Scene/Map Management

**Status**: 🔲 Not Started
**Priority**: Medium

### Components

| Component | Description |
|-----------|-------------|
| KActor | Base game object |
| KActorComponent | Component base class |
| KScene | Scene graph container |
| KSceneManager | Scene management |

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

**Status**: 🔲 Not Started
**Priority**: Medium

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

**Status**: 🔲 Not Started
**Priority**: Medium
**Framework**: .NET 8.0 WPF

### Features

1. **Viewport Panel**
   - 3D scene view with camera controls
   - Object selection and transformation (translate, rotate, scale)
   - Grid and axis visualization

2. **Scene Hierarchy**
   - Tree view of all actors in scene
   - Add/Remove/Rename actors
   - Drag-and-drop reordering

3. **Properties Panel**
   - Transform properties (Position, Rotation, Scale)
   - Component properties
   - Material editor

4. **Content Browser**
   - Asset browser with thumbnail previews
   - Import assets (FBX, textures)
   - Drag-and-drop to viewport

5. **Toolbar**
   - Play/Pause/Stop simulation
   - Save/Load scene
   - Undo/Redo

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
│   └── Deferred/
│       ├── GBuffer.h/cpp
│       └── DeferredRenderer.h/cpp
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

# Asset System

@file AssetSystem.md
@brief Comprehensive documentation for the KojeomGameEngine Asset system covering static meshes, skeletal meshes, skeletons, animations, animation state machines, and model loading.

## Table of Contents

- [Overview](#overview)
- [Static Mesh System](#static-mesh-system)
- [Skeletal Mesh System](#skeletal-mesh-system)
- [Skeleton System](#skeleton-system)
- [Animation System](#animation-system)
- [Animation State Machine](#animation-state-machine)
- [Model Loader](#model-loader)

---

## Overview

The Asset system (`Engine/Assets/`) provides all mesh, skeleton, animation, and model loading functionality. It is organized into the following modules:

| File | Description |
|------|-------------|
| `StaticMesh.h/.cpp` | Static mesh with LOD support, serialization, and render mesh creation |
| `StaticMeshComponent.h/.cpp` | Scene component for rendering static meshes |
| `SkeletalMeshComponent.h/.cpp` | Skinned vertex definition, skeletal mesh, and scene component |
| `Skeleton.h/.cpp` | Bone hierarchy, bind poses, inverse bind poses |
| `Animation.h/.cpp` | Keyframe animation channels and interpolation |
| `AnimationInstance.h/.cpp` | Single-animation playback with Once/Loop/PingPong modes |
| `AnimationStateMachine.h/.cpp` | State-driven animation blending with transitions and notifies |
| `ModelLoader.h/.cpp` | Model import via Assimp (primary) and OBJ/GLTF/FBX fallback parsers |

### Source Files

```
Engine/Assets/
  StaticMesh.h / .cpp
  StaticMeshComponent.h / .cpp
  SkeletalMeshComponent.h / .cpp
  Skeleton.h / .cpp
  Animation.h / .cpp
  AnimationInstance.h / .cpp
  AnimationStateMachine.h / .cpp
  ModelLoader.h / .cpp
```

---

## Static Mesh System

### KStaticMesh

`KStaticMesh` stores geometry data across multiple levels of detail and manages GPU render meshes. It supports binary serialization via `KBinaryArchive`.

```
constexpr uint32 STATIC_MESH_VERSION = 1;
constexpr uint32 MAX_MESH_LODS = 4;
```

#### FMeshLOD

Each LOD stores its own vertex and index data along with switching thresholds:

```cpp
struct FMeshLOD
{
    std::vector<FVertex> Vertices;
    std::vector<uint32> Indices;
    float ScreenSize;   // Screen-relative size threshold (1.0 = full detail)
    float Distance;     // Distance-based switching threshold
};
```

#### Key Methods

```cpp
class KStaticMesh
{
public:
    HRESULT LoadFromFile(const std::wstring& Path, ID3D11Device* Device);
    HRESULT SaveToFile(const std::wstring& Path);
    HRESULT CreateFromMeshData(ID3D11Device* Device, const std::vector<FVertex>& Vertices,
                               const std::vector<uint32>& Indices);

    void Render(ID3D11DeviceContext* Context, uint32 LODIndex = 0);
    void AddLOD(const std::vector<FVertex>& Vertices, const std::vector<uint32>& Indices,
                float ScreenSize = 1.0f);
    void SetLODData(uint32 LODIndex, const std::vector<FVertex>& Vertices,
                    const std::vector<uint32>& Indices);

    uint32 GetLODCount() const;
    FMeshLOD* GetLOD(uint32 Index);
    KMesh* GetRenderMesh(uint32 LODIndex = 0);
    std::shared_ptr<KMesh> GetRenderMeshShared(uint32 LODIndex = 0);

    uint32 GetMaterialSlotCount() const;
    void AddMaterialSlot(const FMaterialSlot& Slot);
    const FMaterialSlot* GetMaterialSlot(uint32 Index) const;

    void CalculateBounds();
    void Cleanup();
};
```

#### Usage Example

```cpp
auto staticMesh = std::make_shared<KStaticMesh>();
staticMesh->SetName("Cube");

std::vector<FVertex> vertices = { /* ... */ };
std::vector<uint32> indices = { /* ... */ };
staticMesh->CreateFromMeshData(device, vertices, indices);

FMaterialSlot slot;
slot.Name = "MainMaterial";
slot.StartIndex = 0;
slot.IndexCount = static_cast<uint32>(indices.size());
slot.MaterialIndex = 0;
staticMesh->AddMaterialSlot(slot);

staticMesh->SaveToFile(L"Models/Cube.kmesh");

staticMesh->Render(context, 0);
```

#### Serialization Format

Binary files store version, name, LOD count, vertex/index data per LOD, material slots, bounding box, and bounding sphere. Vertex data is serialized as individual float fields (Position, Color, Normal, TexCoord).

### KStaticMeshComponent

A `KActorComponent` subclass that attaches a `KStaticMesh` to an actor for rendering:

```cpp
class KStaticMeshComponent : public KActorComponent
{
public:
    void SetStaticMesh(std::shared_ptr<KStaticMesh> InMesh);
    void SetShader(std::shared_ptr<KShaderProgram> InShader);
    void SetMaterial(std::shared_ptr<KMaterial> InMaterial);
    void SetCastShadow(bool bCast);

    virtual void Render(KRenderer* Renderer) override;
    virtual void Serialize(KBinaryArchive& Archive) override;
    virtual void Deserialize(KBinaryArchive& Archive) override;
};
```

#### Usage Example

```cpp
auto actor = scene->CreateActor<KActor>("CubeActor");
auto meshComponent = actor->AddComponent<KStaticMeshComponent>();
meshComponent->SetStaticMesh(staticMesh);
meshComponent->SetShader(litShader);
meshComponent->SetCastShadow(true);
```

---

## Skeletal Mesh System

### FSkinnedVertex

The skinned vertex structure extends the base `FVertex` with bone influence data. Each vertex supports up to 4 bone influences with normalized weights:

```cpp
constexpr uint32 MAX_BONE_INFLUENCES = 4;
constexpr uint32 MAX_SKINNING_BONES = 256;

struct FSkinnedVertex
{
    XMFLOAT3 Position;
    XMFLOAT4 Color;
    XMFLOAT3 Normal;
    XMFLOAT2 TexCoord;
    XMFLOAT3 Tangent;
    XMFLOAT3 Bitangent;
    uint32 BoneIndices[MAX_BONE_INFLUENCES];  // Bone indices (up to 4)
    float BoneWeights[MAX_BONE_INFLUENCES];   // Normalized influence weights

    void AddBoneWeight(uint32 BoneIndex, float Weight);
    void NormalizeWeights();
};
```

`AddBoneWeight` appends a bone influence to the first available (zero-weight) slot. `NormalizeWeights` divides all weights by their sum so they sum to 1.0.

### FBoneMatrixBuffer

The GPU constant buffer sent to the vertex shader for skinning. It holds up to 256 bone transformation matrices:

```cpp
struct FBoneMatrixBuffer
{
    XMMATRIX BoneMatrices[MAX_SKINNING_BONES];  // 256 bone matrices
};
```

This buffer is created with `D3D11_USAGE_DYNAMIC` and `D3D11_CPU_ACCESS_WRITE`, updated each frame via `Map/Unmap`.

### KSkeletalMesh

`KSkeletalMesh` manages GPU buffers for skinned geometry. Unlike `KStaticMesh`, it does not support LODs:

```cpp
class KSkeletalMesh
{
public:
    HRESULT CreateFromData(ID3D11Device* Device,
                           const std::vector<FSkinnedVertex>& Vertices,
                           const std::vector<uint32>& Indices);
    void Render(ID3D11DeviceContext* Context);

    uint32 GetVertexCount() const;
    uint32 GetIndexCount() const;
    ID3D11Buffer* GetVertexBuffer() const;
    ID3D11Buffer* GetIndexBuffer() const;

    void Cleanup();
};
```

### KSkeletalMeshComponent

The scene component for skeletal mesh rendering. It owns a skeleton, animations, and an `KAnimationInstance` for playback:

```cpp
class KSkeletalMeshComponent : public KActorComponent
{
public:
    void SetSkeletalMesh(std::shared_ptr<KSkeletalMesh> InMesh);
    void SetSkeleton(std::shared_ptr<KSkeleton> InSkeleton);

    void PlayAnimation(const std::string& AnimationName,
                       EAnimationPlayMode PlayMode = EAnimationPlayMode::Loop);
    void StopAnimation();
    void PauseAnimation();
    void ResumeAnimation();

    void AddAnimation(const std::string& Name, std::shared_ptr<KAnimation> Animation);
    std::shared_ptr<KAnimation> GetAnimation(const std::string& Name) const;
    bool HasAnimation(const std::string& Name) const;

    void UpdateBoneMatrices(ID3D11DeviceContext* Context);
    void SetCastShadow(bool bCast);

    virtual void Tick(float DeltaTime) override;
    virtual void Render(KRenderer* Renderer) override;
};
```

#### Usage Example

```cpp
auto actor = scene->CreateActor<KActor>("Character");
auto skelComp = actor->AddComponent<KSkeletalMeshComponent>();

skelComp->SetSkeletalMesh(skeletalMesh);
skelComp->SetSkeleton(skeleton);
skelComp->AddAnimation("Walk", walkAnim);
skelComp->AddAnimation("Run", runAnim);
skelComp->PlayAnimation("Walk", EAnimationPlayMode::Loop);
```

During `Tick`, the component updates the `KAnimationInstance`, then computes final bone matrices into `FBoneMatrixBuffer`. During `Render`, it uploads the buffer to the GPU and calls `Renderer->RenderSkeletalMesh()`.

---

## Skeleton System

### FBone

Each bone stores its local transform, parent hierarchy reference, and computed world-space bind pose:

```cpp
constexpr uint32 MAX_BONES = 256;
constexpr int32 INVALID_BONE_INDEX = -1;

struct FBone
{
    std::string Name;
    int32 ParentIndex;             // INVALID_BONE_INDEX (-1) for root bones
    XMMATRIX BindPose;             // World-space bind pose matrix
    XMMATRIX InverseBindPose;      // Inverse of BindPose (for GPU skinning)
    XMFLOAT3 LocalPosition;
    XMFLOAT4 LocalRotation;        // Quaternion
    XMFLOAT3 LocalScale;

    void Serialize(KBinaryArchive& Archive);
    void Deserialize(KBinaryArchive& Archive);
};
```

### KSkeleton

Manages the bone hierarchy and provides lookup by name or index:

```cpp
class KSkeleton
{
public:
    uint32 AddBone(const FBone& Bone);
    uint32 GetBoneCount() const;

    int32 GetBoneIndex(const std::string& Name) const;
    const FBone* GetBone(uint32 Index) const;
    FBone* GetBoneMutable(uint32 Index);
    const FBone* GetBoneByName(const std::string& Name) const;

    void CalculateBindPoses();
    void CalculateInverseBindPoses();

    XMMATRIX GetBoneMatrix(uint32 Index) const;
    void SetBoneMatrix(uint32 Index, const XMMATRIX& Matrix);

    void Serialize(KBinaryArchive& Archive);
    void Deserialize(KBinaryArchive& Archive);
};
```

#### Bind Pose Calculation

`CalculateBindPoses()` computes world-space bind poses hierarchically. Root bones (ParentIndex == -1) use their local matrix directly. Child bones concatenate their local matrix with their parent's bind pose:

```
Bone.BindPose = LocalMatrix * Parent.BindPose
```

The local matrix is constructed as `Scale * Rotation * Translation` from the bone's `LocalPosition`, `LocalRotation` (quaternion), and `LocalScale`.

`CalculateInverseBindPoses()` computes `XMMatrixInverse(nullptr, bone.BindPose)` for each bone, used in the GPU skinning shader.

#### Usage Example

```cpp
auto skeleton = std::make_shared<KSkeleton>();
skeleton->SetName("CharacterSkeleton");

FBone rootBone;
rootBone.Name = "Hips";
rootBone.ParentIndex = INVALID_BONE_INDEX;
rootBone.LocalPosition = XMFLOAT3(0, 100, 0);
skeleton->AddBone(rootBone);

FBone spineBone;
spineBone.Name = "Spine";
spineBone.ParentIndex = skeleton->GetBoneIndex("Hips");
spineBone.LocalPosition = XMFLOAT3(0, 10, 0);
skeleton->AddBone(spineBone);

skeleton->CalculateBindPoses();
skeleton->CalculateInverseBindPoses();
```

---

## Animation System

### FTransformKey

A single keyframe containing time, position, rotation (quaternion), and scale:

```cpp
struct FTransformKey
{
    float Time;
    XMFLOAT3 Position;
    XMFLOAT4 Rotation;   // Quaternion
    XMFLOAT3 Scale;

    void Serialize(KBinaryArchive& Archive);
    void Deserialize(KBinaryArchive& Archive);
};
```

### FAnimationChannel

A channel animates a single bone with separate keyframe tracks for position, rotation, and scale:

```cpp
struct FAnimationChannel
{
    uint32 BoneIndex;              // Index into KSkeleton
    std::string BoneName;          // Bone name for lookup
    std::vector<FTransformKey> PositionKeys;
    std::vector<FTransformKey> RotationKeys;
    std::vector<FTransformKey> ScaleKeys;

    size_t GetPositionKeyIndex(float Time) const;
    size_t GetRotationKeyIndex(float Time) const;
    size_t GetScaleKeyIndex(float Time) const;

    XMFLOAT3 InterpolatePosition(float Time) const;
    XMFLOAT4 InterpolateRotation(float Time) const;
    XMFLOAT3 InterpolateScale(float Time) const;

    void Serialize(KBinaryArchive& Archive);
    void Deserialize(KBinaryArchive& Archive);
};
```

#### Interpolation

- **Position**: Linear interpolation (`XMVectorLerp`) between the two surrounding keys.
- **Rotation**: Spherical linear interpolation (`XMQuaternionSlerp`). Handles the double-cover issue by negating the target quaternion when the dot product is negative.
- **Scale**: Linear interpolation (`XMVectorLerp`).

Key indices are found via linear scan to locate the pair of keys surrounding the query time.

### KAnimation

Stores a complete animation asset with metadata and channels:

```cpp
constexpr uint32 ANIMATION_VERSION = 1;

class KAnimation
{
public:
    void SetName(const std::string& InName);
    void SetDuration(float InDuration);
    void SetTicksPerSecond(float InTicksPerSecond);

    float GetDuration() const;
    float GetTicksPerSecond() const;
    float GetDurationInSeconds() const;  // Duration / TicksPerSecond

    uint32 AddChannel(const FAnimationChannel& Channel);
    uint32 GetChannelCount() const;
    const FAnimationChannel* GetChannel(uint32 Index) const;
    const FAnimationChannel* GetChannelByBoneIndex(uint32 BoneIndex) const;

    void BuildBoneIndexMap(const KSkeleton* Skeleton);

    HRESULT LoadFromFile(const std::wstring& Path);
    HRESULT SaveToFile(const std::wstring& Path);

    void Serialize(KBinaryArchive& Archive);
    void Deserialize(KBinaryArchive& Archive);
};
```

#### Bone Index Mapping

`BuildBoneIndexMap()` resolves channel `BoneName` strings to integer bone indices using the provided `KSkeleton`. It populates an internal `BoneIndexToChannel` map for O(1) channel lookup by bone index.

#### File I/O

Animations are serialized as binary files via `KBinaryArchive`. The format includes:
- Version (uint32)
- Name (string)
- Duration (float)
- TicksPerSecond (float)
- Channel count (uint32)
- Per-channel: BoneIndex, BoneName, Position/Rotation/Scale key counts, then all keys

#### Usage Example

```cpp
auto animation = std::make_shared<KAnimation>();
animation->SetName("WalkCycle");
animation->SetDuration(60.0f);
animation->SetTicksPerSecond(30.0f);

FAnimationChannel hipChannel;
hipChannel.BoneName = "Hips";
hipChannel.PositionKeys.push_back({0.0f, {0, 100, 0}, {0,0,0,1}, {1,1,1}});
hipChannel.PositionKeys.push_back({1.0f, {0, 102, 0}, {0,0,0,1}, {1,1,1}});
animation->AddChannel(hipChannel);

animation->BuildBoneIndexMap(skeleton.get());
animation->SaveToFile(L"Animations/WalkCycle.kanim");
```

---

## Animation Instance

### Playback Modes

```cpp
enum class EAnimationPlayMode
{
    Once,      // Plays once, then stops
    Loop,      // Loops continuously
    PingPong   // Plays forward then reverses direction
};
```

### Playback States

```cpp
enum class EAnimationState
{
    Stopped,
    Playing,
    Paused
};
```

### KAnimationInstance

Manages single-animation playback, bone matrix evaluation, and blend weight:

```cpp
class KAnimationInstance
{
public:
    void SetSkeleton(KSkeleton* InSkeleton);

    void PlayAnimation(const std::string& Name, std::shared_ptr<KAnimation> Anim,
                       EAnimationPlayMode Mode = EAnimationPlayMode::Loop);
    void StopAnimation();
    void PauseAnimation();
    void ResumeAnimation();

    void SetPlaybackSpeed(float Speed);
    void SetPlayMode(EAnimationPlayMode Mode);
    void SetCurrentTime(float Time);
    void SetBlendWeight(float Weight);

    float GetCurrentTime() const;
    float GetNormalizedTime() const;
    EAnimationState GetState() const;
    bool IsPlaying() const;

    void Update(float DeltaTime);
    void UpdateBoneMatrices();

    const std::vector<XMMATRIX>& GetBoneMatrices() const;
    const XMMATRIX* GetBoneMatrixData() const;
    uint32 GetBoneMatrixCount() const;

    void AddAnimation(const std::string& Name, std::shared_ptr<KAnimation> Anim);
    void RemoveAnimation(const std::string& Name);
    void ClearAnimations();
};
```

#### Update Loop

1. Advance `CurrentTime` by `DeltaTime * PlaybackSpeed * (bReverse ? -1 : 1)`.
2. Clamp/wrap based on `EAnimationPlayMode`:
   - **Once**: Clamps to `[0, Duration]`, transitions to `Stopped`.
   - **Loop**: Wraps via `fmodf`.
   - **PingPong**: Reflects at boundaries, toggles `bReverse`.
3. Calls `UpdateBoneMatrices()` which evaluates the animation and computes final bone transforms.

#### Bone Transform Pipeline

```
For each bone:
  1. Interpolate position/rotation/scale from animation channels (or use bind pose defaults)
  2. Build local matrix: Scale * Rotation * Translation
  3. Propagate hierarchy: GlobalTransform = LocalTransform * Parent.GlobalTransform
  4. Final skinning matrix: FinalBoneMatrices[i] = GlobalTransform * InverseBindPose
```

#### Usage Example

```cpp
auto instance = std::make_shared<KAnimationInstance>();
instance->SetSkeleton(skeleton.get());
instance->AddAnimation("Walk", walkAnim);
instance->PlayAnimation("Walk", EAnimationPlayMode::Loop);
instance->SetPlaybackSpeed(1.5f);

// In game loop:
instance->Update(deltaTime);
const auto& boneMatrices = instance->GetBoneMatrices();
```

---

## Animation State Machine

The state machine (`KAnimationStateMachine`) provides state-driven animation blending with conditional transitions and animation notifies.

### State Status

```cpp
enum class EAnimStateStatus
{
    Inactive,
    Active,
    TransitioningOut,
    TransitioningIn
};
```

### FAnimNotify

Animation events triggered at a specific normalized time within a state:

```cpp
struct FAnimNotify
{
    std::string Name;
    float TriggerTime;     // Normalized time (0.0 - 1.0)
    float Duration;
    std::function<void()> Callback;
    bool bTriggered;
};
```

Notifies are checked each frame by `KAnimationState::CheckNotifies()`, which handles both normal playback and loop-around transitions correctly.

### FAnimTransitionCondition

Conditions that must be satisfied for a transition to fire:

```cpp
struct FAnimTransitionCondition
{
    std::string ParameterName;
    enum class EComparisonType
    {
        Equals, NotEquals,
        Greater, GreaterOrEquals,
        Less, LessOrEquals
    } ComparisonType;
    float CompareValue;
    bool bIsBool;          // If true, CompareValue > 0.5f is treated as the expected bool value
};
```

All conditions on a transition must evaluate to `true` for the transition to be eligible. Float parameters use the comparison type directly; bool parameters use `Equals`/`NotEquals`.

### KAnimationState

A named state containing an animation, playback settings, outgoing transitions, and notifies:

```cpp
class KAnimationState
{
public:
    KAnimationState(const std::string& InName);

    void SetAnimation(std::shared_ptr<KAnimation> InAnimation);
    void SetLooping(bool bLoop);
    void SetSpeed(float InSpeed);
    void SetBlendDuration(float Duration);

    void AddTransition(KAnimationTransition* Transition);
    void RemoveTransition(const std::string& TargetStateName);

    void AddNotify(const FAnimNotify& Notify);
    void ClearNotifies();
    const std::vector<FAnimNotify>& GetNotifies() const;

    void CheckNotifies(float CurrentTime, float PreviousTime, float Duration);

    float GetCurrentTime() const;
    void SetCurrentTime(float Time);

    EAnimStateStatus GetStatus() const;
    float GetStateWeight() const;
    void Update(float DeltaTime);
    void Reset();
};
```

### KAnimationTransition

A directed transition between two states with conditions, exit time, and blend settings:

```cpp
class KAnimationTransition
{
public:
    KAnimationTransition(const std::string& InSourceState, const std::string& InTargetState);

    void SetSourceState(const std::string& Name);
    void SetTargetState(const std::string& Name);
    void SetBlendDuration(float Duration);
    void SetExitTime(float Time);
    void SetHasExitTime(bool bHas);

    void AddCondition(const FAnimTransitionCondition& Condition);
    void ClearConditions();

    bool CheckConditions(const std::unordered_map<std::string, float>& FloatParams,
                         const std::unordered_map<std::string, bool>& BoolParams) const;

    float GetBlendProgress() const;
    bool CanTransition() const;
    void Reset();
};
```

- **Exit Time**: When `bHasExitTime` is true, the transition only fires when conditions are met AND the source animation reaches the specified normalized exit time. This prevents mid-animation interruption.

### KAnimationStateMachine

The top-level state machine that manages states, transitions, parameters, and blending:

```cpp
class KAnimationStateMachine
{
public:
    void SetSkeleton(KSkeleton* InSkeleton);

    KAnimationState* AddState(const std::string& Name, std::shared_ptr<KAnimation> Animation);
    void RemoveState(const std::string& Name);
    KAnimationState* GetState(const std::string& Name);
    bool HasState(const std::string& Name) const;

    KAnimationTransition* AddTransition(const std::string& FromState, const std::string& ToState,
                                        float BlendDuration = 0.25f,
                                        bool bHasExitTime = true,
                                        float ExitTime = 1.0f);
    void RemoveTransition(const std::string& FromState, const std::string& ToState);

    void SetDefaultState(const std::string& Name);
    const std::string& GetCurrentStateName() const;
    KAnimationState* GetCurrentState();

    void SetFloatParameter(const std::string& Name, float Value);
    float GetFloatParameter(const std::string& Name) const;
    void SetBoolParameter(const std::string& Name, bool Value);
    bool GetBoolParameter(const std::string& Name) const;

    void TriggerTransition(const std::string& TargetStateName);
    void ForceState(const std::string& StateName, float BlendDuration = 0.25f);

    void Update(float DeltaTime);
    void Reset();

    const std::vector<XMMATRIX>& GetBoneMatrices() const;
    const XMMATRIX* GetBoneMatrixData() const;

    bool IsTransitioning() const;
    float GetBlendProgress() const;
};
```

### Blending

The state machine uses a dual-temp-buffer blending approach. During a transition:

1. `TempBoneMatricesA` receives evaluated bone matrices from the source state (weight: `1.0 - BlendProgress`).
2. `TempBoneMatricesB` receives evaluated bone matrices from the target state (weight: `BlendProgress`).
3. `BlendedBoneMatrices` stores the result of blending.

`BlendBoneMatrices()` decomposes each matrix into Scale/Rotation/Translation, then blends using:
- **Scale**: `XMVectorLerp`
- **Rotation**: `XMQuaternionSlerp`
- **Translation**: `XMVectorLerp`

The blend progress advances by `DeltaTime / BlendDuration` each frame and finalizes at `1.0`.

### Usage Example

```cpp
auto stateMachine = std::make_shared<KAnimationStateMachine>();
stateMachine->SetSkeleton(skeleton.get());

stateMachine->AddState("Idle", idleAnim);
stateMachine->AddState("Walk", walkAnim);
stateMachine->AddState("Run", runAnim);

auto idleToWalk = stateMachine->AddTransition("Idle", "Walk", 0.3f);
idleToWalk->SetHasExitTime(false);

FAnimTransitionCondition speedCondition;
speedCondition.ParameterName = "Speed";
speedCondition.ComparisonType = FAnimTransitionCondition::EComparisonType::Greater;
speedCondition.CompareValue = 0.1f;
speedCondition.bIsBool = false;
idleToWalk->AddCondition(speedCondition);

FAnimNotify footstepNotify;
footstepNotify.Name = "Footstep";
footstepNotify.TriggerTime = 0.5f;
footstepNotify.Callback = []() { /* Play footstep sound */ };
stateMachine->GetState("Walk")->AddNotify(footstepNotify);

stateMachine->SetDefaultState("Idle");

// In game loop:
stateMachine->SetFloatParameter("Speed", playerSpeed);
stateMachine->Update(deltaTime);
const auto& bones = stateMachine->GetBoneMatrices();
```

---

## Model Loader

### FLoadedModel

The result of loading a model file:

```cpp
struct FLoadedModel
{
    std::shared_ptr<KStaticMesh> StaticMesh;
    std::shared_ptr<KSkeletalMesh> SkeletalMesh;
    std::shared_ptr<KSkeleton> Skeleton;
    std::vector<std::shared_ptr<KAnimation>> Animations;
    std::string Name;
    std::wstring SourcePath;
};
```

### FModelLoadOptions

Configuration options for model import:

```cpp
struct FModelLoadOptions
{
    bool bLoadAnimations = true;       // Extract animation data from the file
    bool bGenerateNormals = false;     // Generate smooth normals if missing
    bool bGenerateTangents = true;     // Calculate tangent/bitangent vectors
    bool bFlipUVs = true;              // Flip V coordinate (OpenGL -> DirectX)
    float Scale = 1.0f;                // Uniform scale factor applied to vertex positions
};
```

### KModelLoader

The loader supports both Assimp-based loading and built-in fallback parsers:

```cpp
class KModelLoader
{
public:
    HRESULT Initialize();
    void Cleanup();

    std::shared_ptr<FLoadedModel> LoadModel(const std::wstring& Path,
                                            const FModelLoadOptions& Options = FModelLoadOptions());
    std::shared_ptr<FLoadedModel> LoadModelAsync(const std::wstring& Path,
                                                 const FModelLoadOptions& Options = FModelLoadOptions());

    bool IsModelLoaded(const std::wstring& Path) const;
    std::shared_ptr<FLoadedModel> GetLoadedModel(const std::wstring& Path);
    void UnloadModel(const std::wstring& Path);
    void UnloadAllModels();

    static bool IsSupportedFormat(const std::wstring& Path);
};
```

### Loading Strategy

The loader uses a priority-based approach:

1. **Assimp (primary)**: Available when `USE_ASSIMP` is defined. Supports FBX, OBJ, GLTF, DAE, X, 3DS, BLEND, ASE, and more. Uses `aiProcess_Triangulate`, `aiProcess_JoinIdenticalVertices`, `aiProcess_LimitBoneWeights`, and `aiProcess_ValidateDataStructure` as baseline flags. Optionally adds normal/tangent generation and UV flipping based on `FModelLoadOptions`.

2. **OBJ fallback**: Built-in Wavefront OBJ parser. Supports positions, normals, texture coordinates, and face indices with `v/vt/vn` format. Triangulates polygon faces automatically.

3. **GLTF fallback**: Built-in glTF/GLB parser. Reads the JSON structure and extracts basic geometry. Creates a placeholder cube if full parsing is not possible. A warning is logged recommending Assimp for full GLTF/GLB support.

4. **FBX fallback**: Built-in ASCII FBX parser. Extracts mesh geometry (vertices, normals, UVs, indices with polygon triangulation) and bone hierarchy (LimbNode/Root types with local transforms from `Connect:` entries). Animation curves are extracted from `AnimationCurve` sections.

### Supported Formats

| Format | Assimp | Fallback | Notes |
|--------|--------|----------|-------|
| FBX | Full | ASCII only | Binary FBX requires Assimp |
| OBJ | Full | Full | Static geometry only |
| GLTF/GLB | Full | Basic geometry | Full support requires Assimp |
| DAE | Full | No | Assimp only |
| X | Full | No | Assimp only |
| 3DS | Full | No | Assimp only |
| BLEND | Full | No | Assimp only |
| ASE | Full | No | Assimp only |

### Bone Processing (Assimp)

When loading a model with bones via Assimp:

1. Bone names are collected from all mesh bones.
2. The skeleton hierarchy is built recursively from the scene's root node, creating `FBone` entries with parent indices and local transforms.
3. `InverseBindPose` is set from each Assimp bone's offset matrix.
4. `CalculateBindPoses()` is called to compute world-space bind poses.
5. Vertex bone weights are extracted, sorted by weight (descending), and clamped to `MAX_BONE_INFLUENCES` (4) with renormalization.
6. `FSkinnedVertex` data is constructed and used to create a `KSkeletalMesh`.

### Usage Example

```cpp
KModelLoader loader;
loader.Initialize();

FModelLoadOptions options;
options.bGenerateTangents = true;
options.bFlipUVs = true;
options.Scale = 1.0f;
options.bLoadAnimations = true;

auto model = loader.LoadModel(L"Models/Character.fbx", options);

if (model)
{
    if (model->StaticMesh)
    {
        auto meshComp = actor->AddComponent<KStaticMeshComponent>();
        meshComp->SetStaticMesh(model->StaticMesh);
    }

    if (model->SkeletalMesh && model->Skeleton)
    {
        auto skelComp = actor->AddComponent<KSkeletalMeshComponent>();
        skelComp->SetSkeletalMesh(model->SkeletalMesh);
        skelComp->SetSkeleton(model->Skeleton);

        for (auto& anim : model->Animations)
        {
            skelComp->AddAnimation(anim->GetName(), anim);
        }

        if (!model->Animations.empty())
        {
            skelComp->PlayAnimation(model->Animations[0]->GetName());
        }
    }
}

loader.Cleanup();
```

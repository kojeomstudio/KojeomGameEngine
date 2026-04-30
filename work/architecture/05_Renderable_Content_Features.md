# Renderable Content Features

## Purpose

이 문서는 실제 3D 오브젝트 배치를 위해 필요한 스태틱 메시, 스켈레탈 메시, 간단한 지형, 애니메이션 기능을 미니멀 엔진에 어떻게 넣을지 검토한다.

결론부터 말하면 네 기능 모두 기술적으로 가능하지만, 같은 난이도가 아니다. 미니멀 엔진의 목적이 "작은 3D 게임을 실제로 만들 수 있는 구조"라면 스태틱 메시와 간단한 지형은 초기 범위에 포함할 수 있고, 스켈레탈 메시와 애니메이션은 2차 마일스톤으로 두는 편이 안전하다.

## Feasibility Summary

| Feature | Initial Value | Difficulty | Recommended Timing | Reason |
|---|---:|---:|---|---|
| Static Mesh | very high | medium | early | 배치 가능한 3D 오브젝트의 기본 단위 |
| Simple Terrain | high | medium | after static mesh | 작은 3D 씬의 공간감을 빠르게 만든다 |
| Skeletal Mesh | high | high | after asset/render pipeline stabilizes | skeleton, skinning, bone buffer, import 문제가 추가된다 |
| Animation | high | high | after skeletal mesh | clip sampling, pose evaluation, blending, time update가 필요하다 |

## Core Flow

이 기능들은 다음 흐름으로 들어와야 한다.

```text
Asset file
  -> AssetStore load/import
  -> CPU asset representation
  -> World component references asset handle
  -> World Tick updates transform or animation pose
  -> World builds RenderScene commands
  -> RendererBackend uploads buffers/textures/bone matrices
  -> GPU draws static, skinned, or terrain geometry
```

중요한 원칙은 `World`가 그래픽 API 객체를 직접 소유하지 않는 것이다. `World`는 asset handle, transform, animation state만 알고, `RendererBackend`가 OpenGL buffer, texture, shader, uniform을 관리한다.

## Recommended Asset Format

초기 에셋 포맷은 `glTF 2.0`을 권장한다.

이유:

- Windows/Linux에서 동일하게 사용할 수 있다.
- static mesh, skeletal mesh, skin, animation clip, material 기초 정보를 하나의 공개 포맷으로 다룰 수 있다.
- 학습용 엔진에서 FBX SDK 의존성과 플랫폼 문제를 피할 수 있다.
- `tinygltf`, `cgltf`, `Assimp` 같은 선택지가 있다.

초기에는 다음 전략이 적절하다.

```text
Phase A
  -> OBJ or handmade mesh for triangle/cube

Phase B
  -> glTF static mesh

Phase C
  -> glTF skeletal mesh + skin

Phase D
  -> glTF animation clip
```

처음부터 완전한 importer를 만들려고 하면 엔진보다 importer 구현이 더 커진다. 따라서 "런타임에서 바로 읽는 glTF subset" 또는 "간단한 asset converter가 내부 JSON/binary mesh로 변환"하는 방식이 현실적이다.

## Static Mesh

스태틱 메시가 가장 먼저 들어가야 한다.

```text
StaticMeshAsset
  vertices
  indices
  submeshes
  material slots
  local bounds

StaticMeshComponent
  asset handle
  material override
  world transform

RenderScene
  DrawStaticMeshCommand[]
```

구현 핵심:

- CPU 측 `StaticMeshAsset`은 vertex/index 배열과 bounds를 가진다.
- GPU 측 vertex buffer/index buffer는 `RendererBackend`가 캐시한다.
- `StaticMeshComponent`는 파일 경로나 GPU buffer가 아니라 asset handle을 참조한다.
- material은 처음에는 texture 1개와 shader 1개만 지원해도 충분하다.

최소 vertex layout:

```text
position : float3
normal   : float3
uv0      : float2
```

tangent, color, multiple UV는 후속 확장이다.

## Simple Terrain

간단한 랜드스케이프 기능은 언리얼의 Landscape를 복제하지 말고 `height field terrain`으로 제한한다.

```text
TerrainAsset
  heightmap
  width
  height
  cell size
  material/texture references

TerrainComponent
  asset handle
  world transform
  chunk size

TerrainChunk
  grid vertices
  indices
  local bounds
```

초기 구현 방식:

- heightmap 이미지를 읽어 CPU에서 grid mesh를 생성한다.
- 일정 크기 chunk로 나눈다.
- 각 chunk는 static mesh와 거의 같은 방식으로 렌더링한다.
- height query를 제공해 플레이어 ground check에 사용한다.

초기에는 다음 기능을 제외한다.

- 런타임 sculpting.
- 복잡한 LOD.
- virtual texture.
- foliage 자동 배치.
- 월드 파티션식 streaming.

이렇게 제한하면 지형은 별도 거대 시스템이 아니라 "절차적으로 생성되는 static mesh 묶음"으로 이해할 수 있다.

## Skeletal Mesh

스켈레탈 메시는 스태틱 메시보다 훨씬 복잡하다. 이유는 렌더링할 vertex 위치가 `mesh transform`만으로 결정되지 않고, skeleton pose와 skin weight에 의해 매 프레임 변형되기 때문이다.

```text
SkeletonAsset
  bones[]
    name
    parent index
    inverse bind matrix

SkeletalMeshAsset
  vertices
    position
    normal
    uv0
    bone indices
    bone weights
  indices
  skeleton handle
  bounds

SkeletalMeshComponent
  skeletal mesh handle
  animator handle or animation state
```

GPU skinning을 기준으로 한 흐름:

```text
AnimatorComponent
  -> evaluates local bone transforms
  -> builds component-space pose
  -> computes final skin matrices
  -> RenderScene stores skinned draw command
  -> Renderer uploads bone matrices
  -> vertex shader applies weighted bone transform
```

초기에는 bone 수를 제한한다. 예를 들어 `max 64 bones` 또는 `max 128 bones`로 시작한다. OpenGL 4.5에서는 uniform array, UBO, texture buffer, SSBO 등 선택지가 있지만, 학습용 초기 구현은 uniform array 또는 UBO로 시작하고 한계가 보이면 SSBO로 확장한다.

## Animation

애니메이션은 "시간에 따라 pose를 만드는 시스템"이다. 처음부터 언리얼식 Montage, Blend Space, State Machine을 만들 필요는 없다.

최소 구조:

```text
AnimationClip
  duration
  ticks per second
  tracks[]
    bone index
    position keys
    rotation keys
    scale keys

Pose
  local bone transforms[]

AnimatorComponent
  skeleton handle
  current clip handle
  playback time
  speed
  loop flag
```

최소 실행 흐름:

```text
Engine frame
  -> World::Tick(deltaSeconds)
  -> AnimatorComponent advances playback time
  -> AnimationClip samples local transforms
  -> Skeleton hierarchy builds component-space pose
  -> final skin matrices computed
  -> RenderScene receives DrawSkinnedMeshCommand
```

초기 애니메이션 기능:

- 단일 clip 재생.
- loop 재생.
- 정지/재생.
- clip time seek.
- 두 clip 사이의 단순 cross fade.

후속 기능:

- state machine.
- event notify.
- root motion.
- additive animation.
- blend tree.

## Required Runtime Components

기능을 추가하면 `World` 컴포넌트는 다음 정도로 확장된다.

```text
StaticMeshComponent
  -> static object draw

TerrainComponent
  -> height field terrain draw and height query

SkeletalMeshComponent
  -> skinned object draw

AnimatorComponent
  -> animation time and pose evaluation
```

`SkeletalMeshComponent`와 `AnimatorComponent`를 분리하는 것이 좋다. 이유는 렌더링 가능한 스켈레탈 메시와 시간 기반 애니메이션 상태가 같은 개념이 아니기 때문이다. 예를 들어 T-pose preview는 스켈레탈 메시만 있어도 가능하지만, clip 재생은 animator가 필요하다.

## Renderer Changes

`RenderScene`은 draw command 종류를 늘려야 한다.

```text
RenderScene
  CameraData
  LightData
  StaticDrawCommands[]
  SkinnedDrawCommands[]
  TerrainDrawCommands[]

StaticDrawCommand
  mesh handle
  material handle
  world matrix

SkinnedDrawCommand
  skeletal mesh handle
  material handle
  world matrix
  bone matrix span/handle

TerrainDrawCommand
  terrain chunk handle
  material handle
  world matrix
```

초기에는 command type별로 렌더링 path를 나눠도 된다. 하나의 범용 render graph를 만들 필요는 없다.

## Recommended Implementation Order

```text
1. StaticMeshAsset + StaticMeshComponent
2. glTF static mesh subset or simple internal mesh format
3. TerrainAsset as generated grid chunks
4. TerrainComponent + height query
5. SkeletonAsset + SkeletalMeshAsset
6. GPU skinning shader
7. AnimationClip + AnimatorComponent
8. single clip playback
9. simple cross fade
```

이 순서가 좋은 이유는 각 단계가 이전 단계의 검증 결과를 재사용하기 때문이다. 지형은 static mesh renderer를 재사용하고, skeletal mesh는 mesh asset pipeline을 확장하며, animation은 skeletal mesh pose 입력만 추가한다.

## Testing Strategy

| Feature | Minimum Test |
|---|---|
| Static mesh | cube/glTF mesh load, vertex/index count, bounds, draw smoke |
| Terrain | heightmap load, chunk count, height query, terrain draw smoke |
| Skeleton | parent hierarchy validation, inverse bind matrix count |
| Skeletal mesh | bone indices/weights validity, T-pose draw smoke |
| Animation clip | known time sample result, loop wrap, missing track handling |
| Animator | delta time advance, pause, seek, final skin matrix count |

렌더링 결과는 자동 검증이 어렵기 때문에 CPU 데이터 검증과 smoke rendering을 함께 사용한다. 특히 animation은 "0초, 중간, 끝 시간"에서 특정 bone transform이 예상값과 가까운지 테스트하는 방식이 효과적이다.

## Final Recommendation

스태틱 메시와 간단한 지형은 미니멀 엔진의 초기 목표에 포함해도 된다. 실제 3D 게임 제작에 바로 필요하고, 엔진 구조를 과도하게 흔들지 않는다.

스켈레탈 메시와 애니메이션은 반드시 필요하지만 초기 1차 목표에는 넣지 않는 편이 좋다. 이 둘은 asset import, skeleton hierarchy, pose evaluation, GPU skinning, shader data upload까지 여러 계층을 동시에 건드린다. 따라서 static mesh, terrain, asset store, render scene, material 기초가 안정된 뒤 추가해야 한다.

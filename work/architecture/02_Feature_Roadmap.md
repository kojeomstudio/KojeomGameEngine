# Feature Roadmap

## Purpose

이 문서는 엔진을 한 번에 크게 만들지 않기 위한 구현 순서를 정의한다. 각 단계는 실행 가능한 결과물과 검증 기준을 가져야 한다.

## Phase 0 - Project Skeleton

목표는 빈 창을 띄우기 전에 빌드, 로그, 테스트를 먼저 세우는 것이다.

구현 항목:

- CMake 기반 C++20 프로젝트.
- Windows와 Linux에서 같은 소스 트리를 빌드하는 구조.
- `Source/Core`, `Source/Platform`, `Source/App`, `Source/Game` 폴더.
- `LogInfo`, `LogWarning`, `LogError`.
- `ASSERT` 또는 `CHECK`.
- 최소 단위 테스트 프로젝트.

완료 기준:

- Windows 실행 파일과 Linux 실행 파일이 같은 CMake target에서 빌드된다.
- 로그 파일이 생성된다.
- 실패하는 테스트를 하나 만들고 실제로 실패를 확인할 수 있다.

## Phase 1 - Window And Loop

목표는 엔진 루프의 뼈대를 만든다.

구현 항목:

- command line parsing.
- `AppConfig`.
- `AppMode`.
- GLFW 또는 SDL2 기반 창 생성.
- 플랫폼 이벤트 펌프.
- `Engine::Initialize`, `Engine::Run`, `Engine::Shutdown`.
- `Engine::TickOneFrame`.
- `Clock`을 통한 `deltaSeconds` 계산.
- ESC 종료.

완료 기준:

- Windows와 Linux에서 흰색 또는 검은색 빈 창이 뜬다.
- 창 닫기와 ESC 종료가 동작한다.
- `--mode smoke --frames 3` 실행 후 자동 종료된다.
- 프레임 시간이 로그 또는 디버그 출력으로 확인된다.

## Phase 2 - OpenGL Clear And Triangle

목표는 OpenGL 4.5 렌더링 파이프라인의 최소 경로를 Windows/Linux 양쪽에서 확인한다. DirectX 11은 Windows 전용 후속 백엔드로 분리한다.

구현 항목:

- `RendererBackend`.
- `OpenGLContext`.
- default framebuffer 또는 render target 설정.
- 화면 clear.
- 하드코딩 삼각형 vertex buffer.
- 최소 vertex shader, pixel shader.

완료 기준:

- Windows와 Linux에서 창 배경색이 매 프레임 clear된다.
- Windows와 Linux에서 삼각형이 화면에 보인다.
- 창 크기 변경 시 viewport와 render target 상태가 정상 갱신된다.

## Phase 3 - Camera, Mesh, Texture

목표는 3D 게임의 최소 렌더링 요소를 구성한다.

구현 항목:

- `Vector`, `Matrix`, `Transform`.
- perspective camera.
- vertex/index buffer 기반 mesh.
- constant buffer.
- texture object.
- sampler state.
- 단순 Lambert 또는 unlit material.

완료 기준:

- 큐브 또는 간단한 모델이 카메라 앞에 렌더링된다.
- 카메라 이동으로 시점이 바뀐다.
- 텍스처가 적용된다.

## Phase 4 - World And Entity

목표는 게임 상태를 렌더러에서 분리한다.

구현 항목:

- `EntityId`.
- `TransformComponent`.
- `CameraComponent`.
- `MeshRendererComponent`.
- `World::Tick`.
- `World::BuildRenderScene`.

완료 기준:

- 게임 코드는 OpenGL/DirectX/Vulkan 리소스를 직접 만지지 않는다.
- 여러 개의 메시 엔티티가 월드에 배치된다.
- `RenderScene`을 통해 렌더러가 그린다.

## Phase 5 - AssetStore And Scene File

목표는 코드에 박힌 리소스를 파일 기반으로 이동한다.

구현 항목:

- `AssetPath`.
- `AssetStore`.
- mesh, texture, shader 로딩 캐시.
- JSON 씬 파일.
- 씬 로드 후 엔티티 생성.

완료 기준:

- 씬 파일 수정만으로 오브젝트 위치나 메시가 바뀐다.
- 같은 텍스처를 여러 엔티티가 참조해도 한 번만 로드된다.
- 누락된 파일은 명확한 에러 로그를 남긴다.

## Phase 6 - Gameplay Sample

목표는 엔진이 실제 게임 코드를 담을 수 있음을 증명한다.

구현 항목:

- 플레이어 이동.
- 추적 카메라.
- 충돌은 AABB 또는 간단한 ground check 수준.
- 간단한 목표 지점 또는 수집 오브젝트.
- 시작, 업데이트, 종료 흐름.

완료 기준:

- 1분 정도 플레이 가능한 작은 씬이 있다.
- 게임 코드는 엔진 모듈을 사용하지만 엔진 내부 구현을 수정하지 않는다.

## Phase 6A - Static Mesh And Simple Terrain

목표는 실제 3D 오브젝트 배치와 간단한 지형 배치를 가능하게 만드는 것이다.

구현 항목:

- `StaticMeshAsset`.
- `StaticMeshComponent`.
- glTF static mesh subset 또는 내부 mesh format.
- `TerrainAsset`.
- heightmap 기반 terrain chunk 생성.
- `TerrainComponent`.
- terrain height query.

완료 기준:

- 씬 파일에서 여러 static mesh를 배치할 수 있다.
- heightmap 기반 지형이 렌더링된다.
- 플레이어 또는 카메라가 terrain height query를 사용할 수 있다.

## Phase 6B - Skeletal Mesh And Basic Animation

목표는 캐릭터형 오브젝트와 단일 clip 재생을 가능하게 만드는 것이다.

구현 항목:

- `SkeletonAsset`.
- `SkeletalMeshAsset`.
- `SkeletalMeshComponent`.
- `AnimationClip`.
- `AnimatorComponent`.
- GPU skinning shader.
- 단일 clip loop playback.

완료 기준:

- T-pose skeletal mesh가 렌더링된다.
- 단일 animation clip이 loop 재생된다.
- `World`와 `Game`은 API별 GPU 리소스를 직접 알지 않는다.

## Phase 7 - Debug UI Or Tooling

목표는 유지보수 가능한 개발 루프를 만든다.

우선순위는 다음과 같다.

1. C++ in-engine debug UI.
2. .NET CLI build or asset utility.
3. Avalonia 또는 웹 기반 scene helper.

초기에는 대형 에디터를 만들지 않는다. 도구는 "현재 엔진 구현을 빠르게 검증하는 보조 수단"으로 제한한다.

## Optional Phase 8 - Secondary Renderer Backend

목표는 렌더러 경계가 실제로 유효한지 검증하는 것이다.

구현 후보:

- Windows 전용 `D3D11Renderer`.
- Windows/Linux 공통 `VulkanRenderer`.

완료 기준:

- `World`와 `Game` 코드를 수정하지 않고 backend만 바꿔 같은 씬을 그린다.
- `RenderScene`과 asset handle이 API별 리소스를 직접 노출하지 않는다.
- backend별 debug log가 분리되어 문제 원인을 추적할 수 있다.

## Milestone Rule

각 Phase는 다음 질문에 답할 수 있어야 완료된다.

- 무엇을 구현했는가?
- 어떤 계층에 들어갔는가?
- 다른 계층과 어떤 데이터로 연결되는가?
- 실패하면 어떤 로그 또는 테스트로 알 수 있는가?
- 샘플 게임에서 실제로 쓰였는가?

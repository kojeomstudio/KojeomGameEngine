# Minimal Runtime Architecture

## Purpose

이 문서는 작은 3D 게임 엔진의 실제 코드 계층을 정의한다. 핵심은 기능 목록을 늘리는 것이 아니라, 기능들이 어떤 방향으로 의존하고 어떤 실행 흐름으로 맞물리는지 고정하는 것이다.

## Top Level Dependency Rule

의존성은 아래 방향으로만 흐른다.

```text
Game
  -> Engine
       -> World
       -> Renderer
       -> Assets
       -> Platform
       -> Core
```

`Core`는 어디에도 의존하지 않는다. `Game`은 모든 상위 사용처이지만 엔진 내부 모듈이 `Game`을 알아서는 안 된다.

플랫폼별 구현은 `Platform`과 `Renderer` 아래에만 위치한다. `World`, `Assets`, `Game`에 `#ifdef _WIN32` 또는 Linux 전용 분기가 늘어나면 아키텍처 경계가 무너진 것으로 판단한다.

## Runtime Flow

```text
main
  -> CommandLine::Parse(argc, argv)
  -> AppConfig 생성
  -> AppMode 선택
  -> Engine::Initialize(config)
       -> Core services
       -> Window
       -> Input
       -> Renderer
       -> AssetStore
       -> World
  -> AppMode::Run(engine)
       -> Platform::PumpMessages()
       -> Input::BeginFrame()
       -> Clock::Tick()
       -> World::Tick(deltaSeconds)
       -> RenderScene scene = World::BuildRenderScene()
       -> Renderer::Render(scene)
       -> Window::Present()
  -> Engine::Shutdown()
```

이 구조에서 `World`는 게임 상태를 소유하고, `Renderer`는 그 상태를 직접 수정하지 않는다. 렌더러는 `RenderScene`이라는 읽기 전용 스냅샷을 받아 그리는 쪽에 가깝게 둔다.

## Module Tree

```text
Source
  Core
    Assert.h
    Log.h
    FileSystem.h
    StringId.h
    Config.h
  Platform
    IWindow.h
    IInput.h
    GlfwWindow.h/.cpp
    GlfwInput.h/.cpp
    NativePlatform.h/.cpp
  App
    CommandLine.h/.cpp
    AppConfig.h
    AppMode.h
    Engine.h/.cpp
    GameApp.h/.cpp
    Clock.h/.cpp
  Math
    Vector.h
    Matrix.h
    Quaternion.h
    Transform.h
  World
    Entity.h
    Component.h
    TransformComponent.h
    CameraComponent.h
    MeshRendererComponent.h
    TerrainComponent.h
    SkeletalMeshComponent.h
    AnimatorComponent.h
    World.h/.cpp
  Animation
    Skeleton.h
    Pose.h
    AnimationClip.h
    Animator.h/.cpp
  Renderer
    Renderer.h/.cpp
    RendererBackend.h
    RenderScene.h
    OpenGL
      OpenGLContext.h/.cpp
      OpenGLShader.h/.cpp
      OpenGLBuffer.h/.cpp
      OpenGLTexture.h/.cpp
      OpenGLRenderer.h/.cpp
    D3D11Optional
      D3D11Device.h/.cpp
      D3D11Renderer.h/.cpp
  Assets
    AssetPath.h
    AssetStore.h/.cpp
    MeshAsset.h
    SkeletalMeshAsset.h
    SkeletonAsset.h
    AnimationClipAsset.h
    TerrainAsset.h
    TextureAsset.h
    ShaderAsset.h
    SceneAsset.h
  Game
    SampleGame.h/.cpp
```

## Core

`Core`는 엔진 전체에서 사용하는 최소 기반이다.

- `Log`: 콘솔, 파일, 디버그 출력으로 메시지를 보낸다.
- `Assert`: 개발 중 잘못된 상태를 빠르게 중단한다.
- `FileSystem`: UTF-8 경로, 바이너리 읽기, 텍스트 읽기, 존재 여부 확인을 담당한다.
- `StringId`: 문자열 비교 비용을 줄이고 에셋 키를 안정적으로 다루기 위한 선택 기능이다.
- `Config`: 초기에는 JSON 또는 INI 스타일의 단순 설정 파일을 읽는다.

`Core`에 게임 규칙, 렌더링 API, 윈도우 API를 넣지 않는다. 이 규칙을 지키면 엔진의 가장 아래 계층이 쉽게 테스트된다.

## Platform

`Platform`은 OS와 엔진 사이의 얇은 경계다. Windows/Linux를 동시에 목표로 하므로 초기에는 Win32/X11/Wayland를 직접 다루기보다 GLFW 또는 SDL2를 사용한다. 직접 OS API를 감싸는 학습은 후속 과제로 미룬다.

```text
IWindow
  - 창 생성/파괴
  - 크기 변경 감지
  - 닫기 요청 감지
  - native handle 조회

IInput
  - 키보드/마우스 이벤트를 프레임 상태로 변환
  - 현재 눌림, 이번 프레임 눌림, 이번 프레임 뗌 구분

GlfwWindow or SdlWindow
  - Windows에서는 Win32 메시지를 내부적으로 처리
  - Linux에서는 X11 또는 Wayland 경로를 내부적으로 처리
```

OS 메시지를 게임 코드에서 직접 읽지 않게 하는 것이 중요하다. 게임은 `InputState`만 보고, Win32, X11, Wayland 메시지 구조를 알 필요가 없어야 한다.

## App And Engine Loop

`App` 계층은 엔진을 실행 가능한 프로그램으로 묶는다.

```cpp
int main(int argc, char** argv)
{
    AppConfig config = CommandLine::Parse(argc, argv);
    SampleGame game;
    Engine engine;

    if (!engine.Initialize(config, game))
    {
        return 1;
    }

    int exitCode = CreateAppMode(config).Run(engine);
    engine.Shutdown();
    return exitCode;
}
```

`Engine`은 한 프레임을 실행하는 공통 기능을 제공하고, `AppMode`가 게임 실행인지 테스트 실행인지 결정한다. `Game`은 `OnStart`, `OnUpdate`, `OnStop` 같은 수명주기 훅을 제공한다.

CLI 테스트를 위해 `Engine::TickOneFrame()` 같은 단일 프레임 실행 함수를 제공하는 것이 좋다. 그러면 `GameMode`는 무한 루프를 돌고, `SmokeTestMode`나 `RenderTestMode`는 지정된 프레임 수만 실행한 뒤 결과를 저장하고 종료할 수 있다.

## World

`World`는 런타임 게임 상태의 소유자다.

최소 구조는 다음 정도로 충분하다.

```text
Entity
  EntityId
  TransformComponent
  optional CameraComponent
  optional MeshRendererComponent
  optional TerrainComponent
  optional SkeletalMeshComponent
  optional AnimatorComponent
  optional NativeBehavior
```

초기에는 완전한 ECS를 만들 필요가 없다. 학습 목적에서는 `Entity`가 컴포넌트를 직접 소유하는 단순 구조가 더 낫다. 나중에 성능 문제가 실제로 확인되면 데이터 지향 ECS로 이동한다.

`MeshRendererComponent`, `TerrainComponent`, `SkeletalMeshComponent`는 모두 렌더링 가능한 상태를 표현하지만 성격이 다르다. 스태틱 메시는 고정 vertex/index를 그리고, 지형은 height field에서 생성된 chunk mesh를 그리며, 스켈레탈 메시는 animation pose가 만든 bone matrix를 함께 사용한다. 따라서 세 기능을 하나의 거대한 mesh component로 합치기보다, 공통 asset handle과 transform 규칙은 공유하되 컴포넌트는 분리하는 편이 유지보수에 좋다.

## Renderer

렌더러는 그래픽 API 리소스와 드로우 호출을 관리한다. 초기 baseline은 OpenGL 4.5이며, Windows 전용 DirectX 11 backend는 후속 확장으로 둔다.

```text
Renderer
  -> RendererBackend
       -> OpenGLRenderer
       -> optional D3D11Renderer
  -> Window surface/context
  -> Shader cache
  -> Mesh buffer cache
  -> Texture cache
  -> Render(scene)
```

`World`가 `GLuint`, `ID3D11Buffer`, `ID3D11ShaderResourceView` 같은 API별 리소스를 직접 알면 안 된다. 게임 상태와 그래픽 API 리소스가 강하게 결합되면 Windows/Linux 지원, 테스트, 백엔드 교체가 어려워진다.

초기 backend 선택 규칙은 단순하게 둔다.

```text
Windows
  default: OpenGLRenderer
  optional: D3D11Renderer

Linux
  default: OpenGLRenderer
```

이 규칙은 DirectX를 포기한다는 뜻이 아니다. 먼저 공통 렌더링 경로를 안정화한 뒤, 같은 `RenderScene` 입력을 D3D11 backend가 소비하도록 확장하면 그래픽 API 간 차이를 더 선명하게 학습할 수 있다.

렌더러가 받는 입력은 다음처럼 단순해야 한다.

```text
RenderScene
  CameraData
  LightData
  StaticDrawCommand[]
  SkinnedDrawCommand[]
  TerrainDrawCommand[]

StaticDrawCommand
  mesh handle
  material handle
  world matrix

SkinnedDrawCommand
  skeletal mesh handle
  material handle
  world matrix
  bone matrix handle/span

TerrainDrawCommand
  terrain chunk handle
  material handle
  world matrix
```

## Assets

초기 에셋 시스템은 언리얼의 패키지 시스템처럼 복잡할 필요가 없다.

```text
AssetPath
  "Meshes/Cube.mesh"
  "Textures/Brick.dds"
  "Shaders/Lit.hlsl"
  "Scenes/TestScene.json"

AssetStore
  LoadMesh(path)
  LoadSkeletalMesh(path)
  LoadSkeleton(path)
  LoadAnimationClip(path)
  LoadTerrain(path)
  LoadTexture(path)
  LoadShader(path)
  LoadScene(path)
```

핵심은 파일 경로를 게임 코드 전체에 흩뿌리지 않는 것이다. `AssetStore`가 경로, 캐시, 실패 로그를 한곳에서 관리한다.

## How Systems Interlock

```text
Input
  -> Game behavior reads input
  -> World state changes
  -> World builds render scene
  -> Renderer uploads constants and draws
  -> Present shows final frame
```

예를 들어 WASD 이동은 다음 흐름으로 동작한다.

```text
GLFW key callback or SDL keyboard event
  -> Platform input records key state
  -> Engine frame begins
  -> SamplePlayerController reads InputState
  -> TransformComponent position changes
  -> World emits DrawCommand with new world matrix
  -> RendererBackend updates uniform/constant buffer
  -> mesh appears at new position
```

이처럼 모든 기능은 "상태를 바꾸는 계층"과 "상태를 소비하는 계층"을 분리해서 설명할 수 있어야 한다.

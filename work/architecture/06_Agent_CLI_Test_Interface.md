# Agent CLI Test Interface

## Purpose

AI Agent가 엔진 기능을 구현한 뒤 직접 검증하려면 GUI를 사람이 열어 확인하는 방식만으로는 부족하다. 작은 3D 엔진이라도 테스트가 수동 실행에만 의존하면 기능 추가가 느려지고 회귀를 발견하기 어렵다.

따라서 엔진 실행 파일은 게임 실행뿐 아니라 테스트와 검증을 위한 CLI 모드를 제공해야 한다.

## Why CLI Matters

CLI 테스트 인터페이스가 필요한 이유는 다음과 같다.

- AI Agent가 기능 구현 후 즉시 명령어로 검증할 수 있다.
- CI에서 Windows/Linux 양쪽 빌드와 smoke test를 자동화할 수 있다.
- GUI 상호작용 없이 정해진 프레임 수만 실행하고 종료할 수 있다.
- 실패를 exit code, log, JSON result로 명확히 표현할 수 있다.
- 렌더링 결과는 screenshot 또는 render scene dump로 비교할 수 있다.

GUI는 개발자에게 필요하지만, 테스트의 기본 인터페이스는 CLI여야 한다.

## Command Line Shape

초기 CLI는 복잡할 필요가 없다.

```text
Simple3DEngine
  --help
  --version
  --project <path>
  --scene <path>
  --backend opengl
  --mode game|smoke|validate-assets|render-test|scene-dump|benchmark
  --frames <count>
  --headless
  --hidden-window
  --screenshot <path>
  --result-json <path>
  --log-file <path>
```

예시:

```text
Simple3DEngine --mode smoke --project Samples/MiniGame --scene Scenes/TestScene.json --frames 120 --result-json Saved/TestResults/smoke.json
```

```text
Simple3DEngine --mode validate-assets --project Samples/MiniGame --scene Scenes/TestScene.json --result-json Saved/TestResults/assets.json
```

```text
Simple3DEngine --mode render-test --scene Scenes/Cube.json --frames 3 --screenshot Saved/Screenshots/cube.png --result-json Saved/TestResults/render_cube.json
```

## Runtime Architecture

CLI는 엔진 바깥의 얇은 parsing 기능이 아니라 실행 모드를 결정하는 앱 계층 기능이어야 한다.

```text
main
  -> CommandLine::Parse(argc, argv)
  -> AppConfig
       mode
       project path
       scene path
       backend
       frame limit
       output paths
  -> Engine::Initialize(config)
  -> IAppMode::Run(engine)
       GameMode
       SmokeTestMode
       AssetValidationMode
       RenderTestMode
       SceneDumpMode
  -> TestResult written
  -> process exit code
```

`Engine` 자체가 CLI 문자열을 직접 해석하지 않게 한다. CLI parsing은 `App` 계층에서 `AppConfig`로 변환하고, 엔진 내부는 구조화된 설정만 받는다.

## Suggested Source Layout

```text
Source
  App
    CommandLine.h/.cpp
    AppConfig.h
    AppMode.h
    GameMode.h/.cpp
    SmokeTestMode.h/.cpp
    AssetValidationMode.h/.cpp
    RenderTestMode.h/.cpp
    SceneDumpMode.h/.cpp
    TestResult.h/.cpp
```

`AppMode`는 게임 루프를 완전히 복제하지 않는다. 대신 `Engine`이 제공하는 frame pump를 이용해 정해진 횟수만 실행한다.

```text
SmokeTestMode
  -> Engine.Initialize
  -> Load scene
  -> for N frames
       -> Engine.TickOneFrame()
  -> collect stats
  -> write TestResult
  -> return exit code
```

## Exit Code Contract

AI Agent와 CI가 결과를 쉽게 판단하려면 exit code 규칙이 고정되어야 한다.

| Exit Code | Meaning |
|---:|---|
| 0 | success |
| 1 | command line parse error |
| 2 | project or scene load failure |
| 3 | asset validation failure |
| 4 | renderer initialization failure |
| 5 | runtime smoke failure |
| 6 | screenshot or result output failure |
| 10+ | unexpected engine error |

로그만 보고 성공/실패를 추론하게 만들면 자동화가 약해진다. 프로세스 exit code가 1차 판단 기준이고, JSON result와 log가 상세 근거다.

## JSON Result

테스트 결과는 사람이 읽을 수 있고 기계가 파싱 가능한 JSON으로 남긴다.

```json
{
  "success": true,
  "mode": "smoke",
  "backend": "opengl",
  "scene": "Scenes/TestScene.json",
  "frames": 120,
  "durationSeconds": 2.0,
  "averageFrameMs": 16.6,
  "entities": 12,
  "drawCalls": 8,
  "loadedAssets": 6,
  "errors": [],
  "warnings": []
}
```

AI Agent는 이 파일을 읽어 기능이 실제로 동작했는지 판단할 수 있다.

## Headless And Hidden Window

렌더링 테스트는 완전한 headless가 어렵다. OpenGL은 플랫폼별 context 생성 문제가 있기 때문이다.

초기 권장 방식:

- `--hidden-window`: GLFW/SDL hidden window로 OpenGL context를 만든다.
- `--frames N`: 지정 프레임만 실행하고 자동 종료한다.
- `--screenshot path`: framebuffer를 읽어 PNG로 저장한다.

후속 고급 방식:

- Linux CI에서 Xvfb 사용.
- EGL 또는 OSMesa 기반 offscreen context 검토.
- Vulkan backend 도입 시 offscreen 테스트 구조 재검토.

미니멀 엔진에서는 처음부터 완전한 headless renderer를 만들 필요는 없다. 숨겨진 창과 정해진 프레임 실행만으로도 AI Agent 테스트 자동화에는 충분하다.

## Test Modes

### `validate-assets`

씬 파일과 에셋 참조를 검증한다.

검증 항목:

- scene JSON parse.
- asset path normalize.
- referenced mesh/texture/shader 존재 여부.
- static mesh vertex/index count.
- terrain heightmap 존재 여부.
- skeletal mesh skeleton 참조.
- animation clip skeleton 호환성.

### `smoke`

엔진을 실제로 초기화하고 짧게 실행한다.

검증 항목:

- window/context 생성.
- renderer 초기화.
- scene load.
- N frame tick.
- render command 생성.
- 정상 shutdown.

### `render-test`

정해진 씬을 몇 프레임 렌더링하고 screenshot을 저장한다.

초기에는 픽셀 완전 비교보다 다음을 먼저 검증한다.

- screenshot 파일 생성.
- 이미지 크기 일치.
- 완전 검은 화면이 아닌지 확인.
- draw call count가 0이 아닌지 확인.

### `scene-dump`

렌더링 전에 `World`가 만든 `RenderScene`을 JSON으로 저장한다.

```json
{
  "staticDrawCommands": 3,
  "skinnedDrawCommands": 1,
  "terrainDrawCommands": 4,
  "camera": {
    "valid": true
  }
}
```

이 모드는 GPU 결과와 무관하게 `World -> RenderScene` 변환이 맞는지 검증하는 데 유용하다.

## Agent Workflow

AI Agent가 기능을 구현할 때 권장되는 검증 흐름은 다음과 같다.

```text
1. Build
   cmake --build build --config Debug

2. Asset validation
   Simple3DEngine --mode validate-assets --scene Scenes/TestScene.json --result-json Saved/TestResults/assets.json

3. Runtime smoke
   Simple3DEngine --mode smoke --scene Scenes/TestScene.json --frames 60 --hidden-window --result-json Saved/TestResults/smoke.json

4. Render test
   Simple3DEngine --mode render-test --scene Scenes/Cube.json --frames 3 --hidden-window --screenshot Saved/Screenshots/cube.png --result-json Saved/TestResults/render_cube.json
```

이 흐름이 있으면 AI Agent는 "코드를 작성했다"에서 멈추지 않고 "실행해 확인했다"까지 수행할 수 있다.

## Final Recommendation

CLI 테스트 인터페이스는 선택 기능이 아니라 초기 아키텍처에 포함하는 것이 좋다. 특히 이 엔진은 AI Agent가 구현을 이어갈 가능성이 높기 때문에, 실행 가능한 검증 표면을 빨리 만드는 것이 GUI 도구보다 우선이다.

GUI 도구는 개발 편의성이고, CLI는 품질 유지 장치다.

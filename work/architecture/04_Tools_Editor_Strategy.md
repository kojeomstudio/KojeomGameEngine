# Tools And Editor Strategy

## Purpose

이 문서는 C++ 엔진 런타임과 C# 도구를 어디까지 나눌지 정의한다. 목표는 유지보수를 쉽게 하되, 작은 엔진의 핵심 학습 범위를 흐리지 않는 것이다.

## Recommendation

초기 엔진은 다음 구성을 권장한다.

```text
C++ Runtime
  - window
  - input
  - renderer
  - world
  - assets
  - game sample

C++ Debug UI
  - runtime inspection
  - simple sliders
  - render stats

C# Tools
  - project launcher
  - asset conversion helper
  - scene JSON helper
  - build/package helper

Engine CLI
  - smoke test
  - asset validation
  - render test
  - scene dump
```

C#을 엔진 런타임에 직접 섞기보다는, 파일 기반 또는 CLI 기반 외부 도구로 두는 편이 단순하다. Windows/Linux를 모두 목표로 하면 WPF는 Windows 전용이므로 초기 공통 도구는 `.NET CLI` 또는 `Avalonia UI`가 더 적합하다.

엔진 자체도 CLI를 제공해야 한다. 외부 도구는 이 CLI를 호출하고 결과 JSON과 로그를 읽는 방식으로 연결한다. 이렇게 하면 GUI 도구가 없어도 AI Agent와 CI가 엔진 기능을 검증할 수 있다.

도구 선택 기준은 다음과 같다.

| Tool Type | Recommended | Reason |
|---|---|---|
| build helper | .NET CLI | Windows/Linux 공통 실행과 자동화에 유리 |
| asset validator | .NET CLI | UI 없이 CI와 로컬 검증에 사용 가능 |
| scene helper | Avalonia or web UI | WPF보다 Linux 대응이 쉽다 |
| runtime debug | C++ in-engine UI | 실제 엔진 상태를 즉시 관찰 가능 |
| Windows-only helper | WPF | Windows 전용 생산성 도구가 필요할 때만 사용 |

## Why Not Full C++ Editor First

대형 C++ 에디터를 먼저 만들면 다음 문제가 생긴다.

- 게임 엔진보다 에디터 프레임워크를 더 많이 만들게 된다.
- UI 상태와 런타임 상태 동기화 문제가 빠르게 커진다.
- 플랫폼/렌더러 학습과 월드 구조 학습의 초점이 흐려진다.

초기에는 엔진 내부 디버그 UI와 외부 C# 보조 도구의 조합이 더 현실적이다.

## C# Tool Boundary

C# 도구는 다음처럼 엔진과 느슨하게 연결한다.

```text
.NET CLI or Avalonia Tool
  -> writes scene.json
  -> writes config.json
  -> invokes Engine executable --project SampleProject
  -> reads logs/output files

C++ Engine
  -> reads scene.json
  -> reads config.json
  -> loads assets
  -> runs game
```

이 구조는 언어 간 ABI, C++/CLI, 네이티브 DLL 바인딩을 피한다. 학습용 엔진에서는 이 단순함이 중요하다.

## First Tools To Build

가장 먼저 만들 가치가 있는 도구는 다음 순서다.

1. `ProjectLauncher`: 실행 파일, 프로젝트 경로, 씬 파일을 선택하고 실행한다.
2. `AssetValidator`: 씬 파일이 참조하는 mesh/texture/shader가 실제 존재하는지 검사한다.
3. `SceneJsonEditor`: 엔티티 이름, transform, mesh path를 편집한다.
4. `BuildHelper`: Debug/Development/Release 빌드와 출력 폴더 정리를 자동화한다.

Windows 전용 내부 도구가 필요할 때만 WPF를 사용한다. 공통 도구는 Linux 실행까지 고려해 CLI, Avalonia, 또는 브라우저 기반 로컬 UI를 우선한다.

이 도구들은 엔진 내부를 몰라도 파일 규약만 알면 동작한다.

## In-Engine Debug UI

런타임 내부 상태 확인은 C#보다 C++ debug UI가 더 적합하다.

```text
Debug UI
  - FPS
  - frame time
  - entity count
  - draw call count
  - loaded texture count
  - selected entity transform
```

이 기능은 게임 실행 중 실제 엔진 상태를 즉시 확인해야 하므로 외부 WPF 도구보다 인게임 UI가 낫다.

## Data Format Strategy

초기 데이터 형식은 사람이 읽을 수 있어야 한다.

```json
{
  "entities": [
    {
      "name": "Player",
      "transform": {
        "position": [0, 0, 0],
        "rotation": [0, 0, 0],
        "scale": [1, 1, 1]
      },
      "mesh": "Meshes/Cube.mesh",
      "texture": "Textures/Player.dds"
    }
  ]
}
```

바이너리 포맷은 나중에 필요할 때 추가한다. 초기에는 JSON이 디버깅, 리뷰, AI Agent 수정에 더 유리하다.

## When To Introduce A Real Editor

다음 조건을 만족하기 전에는 대형 에디터를 만들지 않는다.

- 월드/엔티티/컴포넌트 구조가 최소 1회 이상 리팩터링을 거쳤다.
- 씬 파일 포맷이 안정됐다.
- 렌더러와 에셋 로딩이 샘플 게임에서 검증됐다.
- 어떤 기능을 편집해야 하는지 실제 반복 작업으로 확인됐다.

즉, 에디터는 엔진 구조를 상상해서 만드는 것이 아니라 반복되는 불편함을 자동화하기 위해 만든다.

## Long Term Extension

후속 확장은 다음 순서가 합리적이다.

- C# tool에서 scene hierarchy와 inspector 제공.
- C++ engine에 hot reload 수준의 scene reload 추가.
- C# tool에서 engine process 실행/종료/로그 tail.
- asset import pipeline 분리.
- 필요할 경우 Windows용 DirectX 11/12 backend 또는 Linux/Windows용 Vulkan backend 실험.

고급 렌더링 API 전환은 도구보다 후순위다. 먼저 작은 게임을 실제로 만들 수 있는 공통 OpenGL backend가 있어야 한다.

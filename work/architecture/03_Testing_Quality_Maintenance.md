# Testing, Quality, And Maintenance

## Purpose

이 문서는 작은 엔진이 기능 추가를 반복해도 망가지지 않도록 하는 최소 품질 체계를 정의한다. 핵심은 많은 테스트가 아니라, 엔진 계층마다 실패를 빨리 발견하는 구조다.

## Test Layers

```text
Unit Tests
  -> Core, Math, AssetPath

Integration Tests
  -> AssetStore, Scene load, World to RenderScene

Runtime Smoke Tests
  -> Window, OpenGL context, clear, triangle, sample scene

CLI Mode Tests
  -> validate-assets, smoke, render-test, scene-dump

Manual Play Checklist
  -> sample game input, camera, render, shutdown
```

렌더링은 모든 것을 자동화하기 어렵다. 그래서 수학, 경로, 파싱, 월드 변환은 단위 테스트로 고정하고, GPU 출력은 smoke test와 스크린샷 비교를 후속으로 둔다.

## Platform Test Matrix

| Area | Windows | Linux |
|---|---|---|
| Build | CMake configure/build | CMake configure/build |
| Window | GLFW/SDL window, resize, close | GLFW/SDL window, resize, close |
| Input | keyboard/mouse state transition | keyboard/mouse state transition |
| Renderer | OpenGL debug context, optional D3D11 | OpenGL debug context |
| File Path | relative path, UTF-8 path smoke | case-sensitive path smoke |
| Tooling | .NET CLI, optional WPF/Avalonia | .NET CLI, Avalonia |

Linux에서는 파일 시스템 대소문자 구분이 Windows보다 엄격하다. 따라서 `AssetPath` 테스트는 Windows에서 통과하더라도 Linux에서 깨질 수 있는 경로 대소문자 문제를 반드시 포함해야 한다.

## Feature Test Matrix

| Feature | Required Test | Failure Signal |
|---|---|---|
| `Log` | 파일 생성, 메시지 레벨 출력 | 로그 파일 없음 또는 누락 |
| `Assert` | 개발 빌드에서 중단 확인 | 잘못된 상태가 계속 진행 |
| `Vector/Matrix` | dot, cross, inverse, view-projection | 카메라/렌더 위치 오류 |
| `IWindow/GlfwWindow` | Windows/Linux 생성, resize, close | 창 생성 실패 또는 플랫폼별 이벤트 누락 |
| `Input` | key down/up/held 구분 | 플레이어 입력 오동작 |
| `OpenGLContext` | context 생성, version 확인 | 렌더 시작 실패 |
| `RendererBackend` | backend 선택, clear, triangle | 플랫폼별 렌더 경로 분기 오류 |
| `Shader` | compile/load 실패 로그 | 검은 화면 원인 추적 불가 |
| `CommandLine` | mode/path/frame option parse | AI Agent가 실행 검증 불가 |
| `AppMode` | smoke/render-test 자동 종료 | 테스트가 GUI 수동 확인에 의존 |
| `AssetPath` | normalize, invalid path reject | 파일 참조가 전역적으로 흔들림 |
| `AssetStore` | 중복 로드 캐시 | 메모리 낭비 또는 중복 리소스 |
| `SceneAsset` | save/load roundtrip | 씬 편집 후 재현 실패 |
| `StaticMeshAsset` | vertex/index/bounds 검증 | 배치된 오브젝트 렌더링 오류 |
| `TerrainAsset` | heightmap/chunk/height query 검증 | 지형 충돌 또는 배치 오류 |
| `SkeletonAsset` | bone parent/inverse bind 검증 | 스킨 변형 오류 |
| `AnimationClip` | 특정 시간 pose sample 검증 | 애니메이션 떨림 또는 잘못된 자세 |
| `World` | entity 생성/삭제/tick | 게임 상태 누수 |
| `RenderScene` | world transform 반영 | 게임 상태와 렌더 출력 불일치 |

## Build Configurations

최소 빌드 구성은 3개면 충분하다.

- `Debug`: assert, graphics API debug layer/callback, 상세 로그 활성.
- `Development`: 최적화 일부 활성, 로그 유지, 디버그 UI 사용 가능.
- `Release`: 최적화 활성, 진단 최소화, 샘플 게임 실행 기준.

OpenGL 개발 중에는 가능한 한 debug context와 `KHR_debug` callback을 켠다. Windows 전용 DirectX 11 backend를 추가할 때는 `D3D11_CREATE_DEVICE_DEBUG`를 켠다. 그래픽 API는 실패 원인이 화면에 직접 드러나지 않는 경우가 많으므로 API debug 메시지를 로그와 함께 보는 것이 중요하다.

## Comment Policy

주석은 학습 비용을 낮추기 위한 설계 장치다.

좋은 주석:

```cpp
// The OpenGL context must be current on this thread before creating buffers.
// Without a current context, the generated handle is invalid for rendering.
```

나쁜 주석:

```cpp
// Create render target view.
```

함수 이름이 이미 말하는 내용을 반복하지 않는다. 대신 그래픽 API 리소스 생명주기, 현재 컨텍스트, 좌표계, 프레임 순서, 소유권을 설명한다.

## Ownership Rules

소유권 규칙을 단순하게 유지한다.

- 엔진 객체 소유는 우선 `std::unique_ptr`를 기본으로 한다.
- 공유가 필요한 리소스는 `std::shared_ptr`보다 명시적 handle 또는 cache owner를 우선한다.
- OpenGL 객체는 직접 `GLuint`를 흩뿌리지 말고 RAII 래퍼로 감싼다. DirectX COM 객체는 Windows backend 내부에서만 `Microsoft::WRL::ComPtr` 또는 동등한 RAII 래퍼를 사용한다.
- `World`는 게임 상태를 소유하고, `Renderer`는 GPU 리소스를 소유한다.
- `AssetStore`는 로드된 CPU/GPU 에셋의 접근 지점을 제공하되, 게임 로직이 파일 시스템을 직접 열지 않게 한다.

## Regression Checklist

기능을 추가하기 전후로 다음을 확인한다.

- 빈 창 실행.
- `--mode validate-assets` 실행.
- `--mode smoke --frames 60 --hidden-window` 실행.
- `--mode render-test --frames 3 --screenshot <path>` 실행.
- clear color 표시.
- textured cube 표시.
- 카메라 이동.
- JSON scene load.
- 샘플 게임 1분 플레이.
- Windows와 Linux 양쪽 실행 확인.
- 정상 종료 시 OpenGL debug callback 오류와 backend별 리소스 누수 경고 확인.

이 체크리스트는 작지만 강력하다. 작은 엔진에서는 복잡한 CI보다 매번 같은 최소 시나리오를 깨지 않는 것이 더 중요하다.

## Maintenance Rules For AI Agents

AI Agent가 코드를 구현할 때는 다음 순서를 따른다.

1. 변경할 계층을 먼저 명시한다.
2. 새 기능이 `Initialize`, `Tick`, `Render`, `Shutdown` 중 어디에 들어가는지 설명한다.
3. 테스트 또는 smoke 확인 방법을 같이 추가한다.
4. 그래픽 API 리소스를 만들면 해제 시점도 같은 변경에서 설명한다.
5. 샘플 게임에서 기능을 실제로 사용하게 한다.

문서와 코드가 다르면 코드가 우선이지만, 아키텍처 흐름이 바뀐 경우 문서도 같은 변경에서 갱신한다.

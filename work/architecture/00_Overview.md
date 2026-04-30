# Minimal 3D Game Engine Overview

## Purpose

이 문서 세트는 언리얼 엔진 클라이언트 프로그래머가 직접 작은 3D 게임 엔진을 제작하기 위한 최소 아키텍처 청사진이다.

목표는 언리얼 엔진을 복제하는 것이 아니다. 목표는 게임 엔진의 핵심 흐름을 직접 구현하면서, 실제 작은 3D 게임을 만들 수 있는 수준의 런타임, 렌더러, 월드, 에셋, 도구, 테스트 체계를 갖추는 것이다.

## Recommended Direction

초기 구현은 `Windows/Linux + C++20 + CMake + GLFW(or SDL2) + OpenGL 4.5`를 권장한다.

Windows/Linux를 동시에 목표로 하면 DirectX만으로는 엔진 목표를 만족할 수 없다. DirectX는 Windows 전용 그래픽 API이므로, 학습용 미니멀 엔진의 첫 렌더링 백엔드는 두 플랫폼에서 모두 실행 가능한 OpenGL 4.5 또는 유사한 공통 그래픽 API가 더 적합하다.

OpenGL 4.5 baseline을 먼저 선택하는 이유는 다음과 같다.

- 렌더링 학습의 본질인 디바이스, 스왑체인, 셰이더, 버퍼, 텍스처, 렌더 상태, 드로우 호출을 충분히 학습할 수 있다.
- Windows와 Linux 양쪽에서 동일한 렌더링 경로를 먼저 검증할 수 있다.
- Vulkan, DirectX 12의 명시적 동기화, 디스크립터, 커맨드 큐, PSO 관리가 초기 학습 범위를 과도하게 키우는 문제를 피할 수 있다.
- DirectX 11은 Windows 전용 추가 백엔드로 두면, 그래픽 API 차이를 학습하는 확장 과제로 자연스럽게 이어진다.
- Vulkan 또는 DirectX 12는 렌더러 구조가 안정된 뒤 `RendererBackend` 교체 과제로 진행하는 편이 좋다.

## Cross Platform Contract

Windows/Linux 지원은 "모든 코드를 플랫폼 독립으로 만든다"는 뜻이 아니다. 플랫폼 의존 코드를 명확한 경계 안에 가두는 것을 의미한다.

```text
Game/World/Assets
  -> platform independent

Platform
  -> GLFW/SDL wrapper
  -> native handle only when renderer needs it

Renderer
  -> OpenGL common backend first
  -> optional Windows-only D3D11 backend later

Tools
  -> .NET CLI/Avalonia first
  -> WPF only for Windows-specific helper tools
```

이 계약을 지키면 학습자는 Windows와 Linux의 차이를 엔진 전체에 퍼뜨리지 않고, 창 생성, 입력, 렌더링 context 생성, 파일 경로 차이 정도의 좁은 영역에서 다룰 수 있다.

## Document Map

이 폴더는 단일 거대 문서 대신 다음 문서로 나눈다.

- `00_Overview.md`: 목표, 범위, 기술 선택, 문서 읽기 순서.
- `01_Runtime_Architecture.md`: 실제 엔진 런타임 구조와 각 기능이 맞물리는 방식.
- `02_Feature_Roadmap.md`: 기능 구현 순서, 단계별 완료 조건, 작게 만들고 검증하는 방식.
- `03_Testing_Quality_Maintenance.md`: 기능별 테스트, 회귀 방지, 주석 및 유지보수 규칙.
- `04_Tools_Editor_Strategy.md`: C++ 런타임과 C# 도구/에디터를 어떻게 나눌지에 대한 전략.
- `05_Renderable_Content_Features.md`: 스태틱 메시, 스켈레탈 메시, 지형, 애니메이션 도입 검토.
- `06_Agent_CLI_Test_Interface.md`: AI Agent와 CI가 기능을 검증하기 위한 CLI 실행/테스트 인터페이스.

## Engine Scope

최소 엔진은 다음 기능을 목표로 한다.

```text
Simple3DEngine
  Core
    logging, assert, file, time, config
  Platform
    GLFW/SDL window, OS message pump boundary, keyboard/mouse input
  App
    engine loop, command line modes, game lifecycle, fixed/variable tick policy
  Math
    vector, matrix, quaternion, transform, camera math
  World
    entity id, transform, camera, static mesh, terrain, skeletal mesh, animator, native behavior
  Renderer
    backend interface, OpenGL 4.5 backend, optional D3D11 backend, shader, buffer, texture, material, draw command
  Assets
    file loading, virtual asset path, mesh/texture/shader/scene/skeleton/animation/terrain metadata
  Game
    sample gameplay code that proves the engine is usable
  Tools
    optional .NET CLI/Avalonia or in-engine debug tools
```

## Out Of Scope

초기 버전에서는 다음을 만들지 않는다.

- 언리얼식 리플렉션, UHT, 블루프린트, GC, 패키지 시스템.
- 모든 렌더링 API를 동시에 지원하는 대형 RHI.
- 대형 에디터, 비주얼 스크립팅, 복잡한 에셋 임포터.
- 물리 엔진 전체 구현.
- 네트워크 복제, 멀티플레이어, 월드 파티션.
- 언리얼 수준의 Landscape, Animation Blueprint, Montage, Control Rig.

이 기능들은 학습 가치가 없어서 제외하는 것이 아니라, 최소 엔진의 핵심 목적을 흐리기 때문에 후속 확장으로 미룬다.

## Core Architecture Principle

최소 엔진의 핵심 흐름은 다음 하나로 설명되어야 한다.

```text
main
  -> GameApp 생성
  -> EngineLoop 초기화
  -> Window/Input/Renderer/World/Assets 초기화
  -> while running
       -> Platform 메시지 처리
       -> Input 상태 갱신
       -> Time 계산
       -> World Tick
       -> RenderScene 추출
       -> Renderer Draw
       -> Backbuffer Present/SwapBuffers
  -> 역순 종료
```

학습자는 이 흐름을 기준으로 모든 기능을 배치해야 한다. 기능을 추가할 때는 "이 기능이 초기화, Tick, Render, Shutdown 중 어디에 걸리는가"를 먼저 판단한다.

## Naming And Comment Policy

코드는 학습자가 읽는 자료이기도 하므로 이름과 주석은 구현의 일부로 본다.

- 클래스 이름은 역할을 직접 드러낸다. 예: `OpenGLBuffer`, `RenderScene`, `MeshRendererComponent`.
- 추상화 이름은 미래 확장을 암시하되 과장하지 않는다. 예: 초기에는 `RendererBackend`와 `OpenGLRenderer` 정도면 충분하다.
- 주석은 "무엇을 대입한다"가 아니라 "왜 이 단계가 필요한가"를 설명한다.
- 그래픽 API 호출 주변에는 리소스 생명주기, 현재 컨텍스트, 실패 시 의미를 짧게 남긴다.

## First Target Game

엔진 검증용 첫 게임은 작고 명확해야 한다.

```text
Third Person Mini Scene
  - 플레이어 캡슐 또는 큐브 이동
  - 카메라 추적
  - 정적 메시 3~5개
  - 간단한 height field 지형 1개
  - 텍스처 머티리얼
  - 방향광 1개 또는 단순 램버트 조명
  - JSON 씬 로드
  - ESC 종료, WASD 이동, 마우스 카메라
```

이 샘플이 동작하면 엔진의 윈도우, 입력, 시간, 월드, 렌더러, 에셋, 게임 코드가 실제로 맞물린다는 것을 증명할 수 있다.

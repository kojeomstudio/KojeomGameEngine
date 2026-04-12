# KojeomGameEngine Documentation

KojeomGameEngine의 기술 문서, 개발 가이드라인, 규칙을 관리합니다.

## Directory Structure

```
docs/
├── README.md                        # 이 파일
├── EngineArchitecture.md            # 엔진 아키텍처 전체 개요
├── assets/                          # 에셋 시스템 문서
│   └── AssetSystem.md              # 정적/스켈레탈 메시, 애니메이션, 모델 로더
├── audio/                           # 오디오 시스템 문서
│   └── AudioSystem.md              # XAudio2 오디오, 3D 사운드
├── debugui/                         # 디버그 UI 문서
│   └── DebugUISystem.md            # ImGui 디버그 오버레이
├── editor/                          # 에디터 문서
│   └── EditorGuide.md              # WPF 에디터 아키텍처, MVVM, P/Invoke
├── input/                           # 입력 시스템 문서
│   └── InputSystem.md              # 키보드, 마우스, Raw Input, Action Mapping
├── physics/                         # 물리 시스템 문서
│   └── PhysicsSystem.md            # 리지드바디, 충돌 감지, Raycast
├── renderer/                        # 렌더러 문서
│   └── DirectX11Renderer.md        # DirectX 11 렌더러 기술 문서
├── scene/                           # 씬 시스템 문서
│   └── SceneSystem.md              # Actor-Component, 씬 관리, 시리얼라이제이션
├── serialization/                   # 시리얼라이제이션 문서
│   └── SerializationSystem.md      # 바이너리/JSON 아카이브
├── ui/                              # UI 시스템 문서
│   └── UISystem.md                 # 캔버스 기반 UI, 위젯, 레이아웃
├── utils/                           # 유틸리티 문서
│   └── UtilsSystem.md              # Common.h, Logger.h, Math.h
└── rules/                           # AI 에이전트 규칙 및 가이드라인
    └── AIAgentRules.md             # AI 에이전트 작업 규칙
```

## Quick Links

### For Developers

- [Engine Architecture Overview](EngineArchitecture.md) - 엔진 아키텍처 전체 개요 (모듈, 디자인 패턴, 빌드)
- [DirectX 11 Renderer Documentation](renderer/DirectX11Renderer.md) - 렌더링 시스템 기술 문서 (Forward/Deferred, PBR, 셰이더)
- [Scene System Documentation](scene/SceneSystem.md) - Actor-Component 시스템, 씬 관리
- [Asset System Documentation](assets/AssetSystem.md) - 정적/스켈레탈 메시, 애니메이션, 모델 로더
- [Input System Documentation](input/InputSystem.md) - 키보드, 마우스, Raw Input, Action Mapping
- [Audio System Documentation](audio/AudioSystem.md) - XAudio2 오디오, 3D 사운드
- [Physics System Documentation](physics/PhysicsSystem.md) - 리지드바디, 충돌 감지, Raycast
- [UI System Documentation](ui/UISystem.md) - 캔버스 기반 UI, 위젯, 레이아웃
- [Debug UI System Documentation](debugui/DebugUISystem.md) - ImGui 디버그 오버레이
- [Utility System Documentation](utils/UtilsSystem.md) - Common.h, Logger.h, Math.h
- [Serialization System Documentation](serialization/SerializationSystem.md) - 바이너리/JSON 아카이브
- [Editor Guide](editor/EditorGuide.md) - WPF 에디터 아키텍처, MVVM, P/Invoke

### For AI Agents

- [AI Agent Rules](rules/AIAgentRules.md) - AI 에이전트 필수 참고 문서 (코딩 규칙, 아키텍처 패턴, 금지 사항)
- `.gemini/GEMINI.md` - Gemini 전용 규칙
- `.claude/CLAUDE.md` - Claude 전용 규칙

## Engine Architecture

KojeomGameEngine은 12개 모듈로 구성된 모듈형 게임 엔진입니다:

### Engine Modules

| Module | Files | Lines | Description |
|--------|------:|------:|-------------|
| Graphics | 74 | 23,498 | DirectX 11 렌더링 (Forward/Deferred, PBR, Shadow, PostProcess, IBL, SSAO, SSR, TAA, SSGI, 20 서브시스템) |
| Assets | 19 | 5,180 | 정적/스켈레탈 메시, 스켈레톤, 애니메이션, 모델 로더 (FBX/OBJ/GLTF) |
| UI | 27 | 2,577 | 캔버스 기반 UI (Text, Button, Image, Slider, Checkbox, Layouts) |
| Physics | 6 | 1,138 | 리지드바디, 충돌 감지, Raycast |
| Core | 3 | 1,106 | KEngine - 윈도우 관리, 메인 루프, 서브시스템 소유, ISubsystem 인터페이스, KSubsystemRegistry |
| Audio | 6 | 1,071 | XAudio2 오디오, 3D 사운드 |
| Serialization | 4 | 1,092 | 바이너리/JSON 아카이브 |
| Input | 3 | 774 | 키보드, 마우스, Raw Input, Action Mapping |
| Scene | 4 | 791 | Actor-Component 시스템, 씬 관리 |
| DebugUI | 2 | 264 | ImGui 디버그 오버레이 |
| Utils | 3 | 301 | Common.h, Logger.h, Math.h |
| **Total** | **151** | **37,792** | |

### Editor

| Component | Files | Lines | Description |
|-----------|------:|------:|-------------|
| EngineInterop | 2 | 1,274 | C++ DLL (flat C API, 107 exported functions) for C# P/Invoke |
| KojeomEditor | 23 | 9,439 | WPF 에디터 (.NET 8.0) - Viewport, Scene Hierarchy, Properties, Material Editor, Renderer Settings, Content Browser (14 C# + 9 XAML) |

### Samples

16개 샘플 프로그램이 `samples/` 폴더에 위치합니다:
BasicRendering, Lighting, PBR, PostProcessing, Terrain, Water, Sky, Particles, SkeletalMesh, Gameplay, Physics, UI, UI/Layout, AnimationStateMachine, LOD, DebugRendering

## Document Standards

- Markdown 형식 사용
- 명확한 제목 구조 유지
- 코드 변경 시 문서 동기화
- 내부 참조는 상대 경로 사용

## Contributing

문서 추가 시:

1. 기술 문서 -> `docs/` 또는 적절한 하위 폴더
2. 규칙/가이드라인 -> `docs/rules/`

## License

See [LICENSE](../LICENSE) for license information.

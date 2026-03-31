# KojeomGameEngine Documentation

KojeomGameEngine의 기술 문서, 개발 가이드라인, 규칙을 관리합니다.

## Directory Structure

```
docs/
├── README.md                        # 이 파일
├── EngineArchitecture.md            # 엔진 아키텍처 전체 개요
├── renderer/                        # 렌더러 문서
│   └── DirectX11Renderer.md        # DirectX 11 렌더러 기술 문서
└── rules/                           # AI 에이전트 규칙 및 가이드라인
    └── AIAgentRules.md             # AI 에이전트 작업 규칙
```

## Quick Links

### For Developers

- [Engine Architecture Overview](EngineArchitecture.md) - 엔진 아키텍처 전체 개요 (모듈, 디자인 패턴, 빌드)
- [DirectX 11 Renderer Documentation](renderer/DirectX11Renderer.md) - 렌더링 시스템 기술 문서 (Forward/Deferred, PBR, 셰이더)

### For AI Agents

- [AI Agent Rules](rules/AIAgentRules.md) - AI 에이전트 필수 참고 문서 (코딩 규칙, 아키텍처 패턴, 금지 사항)
- `.gemini/GEMINI.md` - Gemini 전용 규칙
- `.claude/CLAUDE.md` - Claude 전용 규칙

## Engine Architecture

KojeomGameEngine은 12개 모듈로 구성된 모듈형 게임 엔진입니다:

### Engine Modules

| Module | Files | Lines | Description |
|--------|------:|------:|-------------|
| Graphics | 70 | 22,988 | DirectX 11 렌더링 (Forward/Deferred, PBR, Shadow, PostProcess, IBL, SSAO, SSR, TAA, SSGI, 20 서브시스템) |
| Assets | 16 | 4,786 | 정적/스켈레탈 메시, 스켈레톤, 애니메이션, 모델 로더 (FBX/OBJ/GLTF) |
| UI | 27 | 2,513 | 캔버스 기반 UI (Text, Button, Image, Slider, Checkbox, Layouts) |
| Core | 3 | 1,086 | KEngine - 윈도우 관리, 메인 루프, 서브시스템 소유 |
| Audio | 6 | 999 | XAudio2 오디오, 3D 사운드 |
| Physics | 6 | 1,122 | 리지드바디, 충돌 감지, Raycast |
| Serialization | 4 | 945 | 바이너리/JSON 아카이브 |
| Input | 3 | 774 | 키보드, 마우스, Raw Input, Action Mapping |
| Scene | 4 | 763 | Actor-Component 시스템, 씬 관리 |
| DebugUI | 2 | 264 | ImGui 디버그 오버레이 |
| Utils | 3 | 303 | Common.h, Logger.h, Math.h |
| **Total** | **147** | **36,543** | |

### Editor

| Component | Files | Lines | Description |
|-----------|------:|------:|-------------|
| EngineInterop | 2 | 1,150 | C++ DLL (flat C API, ~103 exported functions) for C# P/Invoke |
| KojeomEditor | 21 | 4,218 | WPF 에디터 (.NET 8.0) - Viewport, Scene Hierarchy, Properties, Material Editor, Content Browser |

### Samples

16개 샘플 프로그램이 `samples/` 폴더에 위치합니다:
BasicRendering, Lighting, PBR, PostProcessing, Terrain, Water, Sky, Particles, SkeletalMesh, Gameplay, Physics, UI, Layout, AnimationStateMachine, LOD, DebugRendering

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

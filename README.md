# KojeomEngine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows-blue.svg)](https://www.microsoft.com/windows)
[![Language: C++](https://img.shields.io/badge/Language-C++17-purple.svg)](https://isocpp.org/)
[![API: DirectX 11](https://img.shields.io/badge/API-DirectX%2011-green.svg)](https://docs.microsoft.com/en-us/windows/win32/direct3d11)

DirectX 11 기반의 C++ 게임 엔진입니다. 모듈성과 확장성을 중시하여 설계되었으며, WPF 기반 에디터를 포함합니다.

## Features

### Graphics
- **DirectX 11 Rendering**: Forward/Deferred 렌더링 파이프라인
- **PBR Materials**: Metallic-Roughness 워크플로우, 7개 텍스처 슬롯, 프리셋
- **Shadow System**: Shadow Map, Cascaded Shadow Maps
- **Post-Processing**: HDR, Bloom, Auto Exposure, DOF, Motion Blur, Lens Effects
- **Screen-Space Effects**: SSAO, SSR, TAA, SSGI
- **Environment**: Procedural Sky, Volumetric Fog, Water, Terrain
- **IBL**: Image-based Lighting (Irradiance, Prefiltered Env, BRDF LUT)
- **Optimization**: GPU Instancing, Frustum Culling, Occlusion Culling, LOD System
- **Particle System**: 파티클 이미터

### Core Systems
- **Actor-Component Architecture**: KActor, KActorComponent, KScene
- **Input System**: Keyboard, Mouse, Raw Input, Action Mapping
- **Audio System**: XAudio2, 3D Sound, Master/Sound/Music 볼륨 채널
- **Physics**: Rigid Body, Collision Detection (Sphere/Box), Raycast
- **Animation**: Skeletal Mesh, Animation State Machine, Blend, Notify

### Editor
- **WPF-based Editor** (.NET 8.0, C#)
- **3D Viewport**: DirectX 11 렌더링을 WPF에 임베드
- **Scene Hierarchy**: 씬 트리 뷰, Actor 추가/삭제/재정렬
- **Properties Panel**: Transform, Material, Light, Camera 속성 편집
- **Material Editor**: PBR 파라미터 편집
- **Content Browser**: 에셋 브라우저
- **Undo/Redo**: 액션 기반 실행 취소/다시 실행 시스템
- **Engine Interop**: C++ 엔진과 C# 에디터 간 P/Invoke 브릿지

### Serialization
- **Binary Archive**: 씬, 메시, 스켈레톤 데이터 직렬화
- **JSON Archive**: 커스텀 DOM 기반 파서

## Project Structure

```
KojeomEngine/
├── Engine/                     # Engine Core Library (C++17, Static Library)
│   ├── Core/                   # KEngine - 윈도우, 메인 루프
│   ├── Graphics/               # 렌더링 파이프라인
│   │   ├── Renderer.h/cpp      # 중앙 렌더링 오케스트레이터
│   │   ├── GraphicsDevice.h/cpp # DirectX 11 디바이스 관리
│   │   ├── Camera.h/cpp        # 카메라 시스템
│   │   ├── Shader.h/cpp        # 셰이더 관리
│   │   ├── Mesh.h/cpp          # 메시 시스템
│   │   ├── Material.h/cpp      # PBR 재질
│   │   ├── Texture.h/cpp       # 텍스처 관리
│   │   ├── Light.h             # 조명 구조체
│   │   ├── Shadow/             # 그림자 시스템
│   │   ├── Deferred/           # 디퍼드 렌더링
│   │   ├── PostProcess/        # 후처리 효과
│   │   ├── SSAO/               # Screen-Space AO
│   │   ├── SSR/                # Screen-Space Reflections
│   │   ├── TAA/                # Temporal AA
│   │   ├── SSGI/               # Screen-Space GI
│   │   ├── Sky/                # 프로시저럴 스카이
│   │   ├── Volumetric/         # 볼류메트릭 포그
│   │   ├── Water/              # 워터 렌더링
│   │   ├── Terrain/            # 터레인 렌더링
│   │   ├── Culling/            # 프러스텀/오클루전 컬링
│   │   ├── CommandBuffer/      # 커맨드 버퍼 렌더링
│   │   ├── Instanced/          # GPU 인스턴싱
│   │   ├── IBL/                # Image-based Lighting
│   │   ├── LOD/                # LOD 생성/관리
│   │   ├── Particle/           # 파티클 시스템
│   │   ├── Performance/        # GPU 타이머, 프레임 스탯
│   │   └── Debug/              # 디버그 렌더러
│   ├── Input/                  # 입력 시스템
│   ├── Audio/                  # 오디오 시스템 (XAudio2)
│   ├── Physics/                # 물리 엔진
│   ├── Scene/                  # Actor-Component 시스템
│   ├── Assets/                 # 메시, 스켈레톤, 애니메이션, 모델 로더
│   ├── Serialization/          # 바이너리/JSON 직렬화
│   ├── UI/                     # 캔버스 기반 UI 시스템
│   ├── DebugUI/                # ImGui 디버그 UI
│   └── Utils/                  # 유틸리티 (Common, Logger, Math)
├── Editor/
│   ├── EngineInterop/          # C#/C++ 브릿지 DLL
│   └── KojeomEditor/           # WPF 에디터 (.NET 8.0)
├── samples/                    # 샘플 프로젝트 (16개)
├── docs/                       # 문서
│   ├── rules/                  # AI 에이전트 규칙
│   └── renderer/               # 렌더러 기술 문서
├── KojeomEngine.sln            # Visual Studio 솔루션
└── LICENSE                     # MIT License
```

## Requirements

- **Compiler**: Visual Studio 2022 (v145 toolset, C++17)
- **Platform**: Windows 10/11, x64
- **SDK**: Windows SDK (DirectX 11, XAudio2)
- **.NET**: .NET 8.0 SDK (for Editor)
- **Optional**: Assimp (for model loading)

## Build

### Visual Studio

1. `KojeomEngine.sln`을 Visual Studio 2022에서 열기
2. Build Configuration 선택 (Debug/Release, x64)
3. 솔루션 빌드 (Ctrl+Shift+B)

### Command Line

```cmd
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64
msbuild KojeomEngine.sln /p:Configuration=Release /p:Platform=x64
```

### Editor

```cmd
cd Editor/KojeomEditor
dotnet build
```

## Code Style

| Type | Convention | Example |
|------|------------|---------|
| Classes | `K` prefix | `KRenderer`, `KActor` |
| Structs | `F` prefix | `FVertex`, `FDirectionalLight` |
| Enums | `E` prefix | `EKeyCode`, `ERenderPath` |
| Functions | PascalCase | `Initialize()`, `BeginFrame()` |
| Variables | camelCase | `graphicsDevice` |
| Member bools | `b` prefix | `bInitialized` |

## Documentation

- AI Agent 규칙: `docs/rules/AIAgentRules.md`
- 렌더러 문서: `docs/renderer/DirectX11Renderer.md`
- Claude 규칙: `.claude/CLAUDE.md`
- Gemini 규칙: `.gemini/GEMINI.md`

## License

MIT License - see [LICENSE](LICENSE) for details.

Made with love by Kojeom

# Claude AI Agent Rules for KojeomGameEngine

KojeomGameEngine 프로젝트에서 작업 시 반드시 준수해야 할 규칙입니다. 자세한 내용은 `docs/rules/AIAgentRules.md`를 참조하십시오.

## Project Overview

C++17 / DirectX 11 기반 게임 엔진입니다. WPF 기반 에디터(Editor/KojeomEditor)를 포함하며, C#/C++ 인터페이스는 Editor/EngineInterop DLL을 통해 이루어집니다.

## Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Classes | `K` prefix | `KRenderer`, `KActor`, `KScene` |
| Structs | `F` prefix | `FVertex`, `FDirectionalLight`, `FConstantBuffer` |
| Enums | `E` prefix | `EKeyCode`, `EColliderType`, `ERenderPath` |
| Functions | PascalCase | `Initialize()`, `BeginFrame()` |
| Variables | camelCase | `graphicsDevice`, `currentCamera` |
| Member booleans | `b` prefix | `bInFrame`, `bInitialized` |
| Parameters | `In` prefix | `InGraphicsDevice`, `InMesh` |
| Type Aliases | PascalCase | `ActorPtr`, `ComPtr<T>` |
| Template Params | PascalCase | `T`, `FuncType` |

## DirectX 11 Rules

- ComPtr for all COM objects
- Check HRESULT on all D3D11 calls
- Constant buffers must be 16-byte aligned
- Shader Model 5.0 (`vs_5_0`, `ps_5_0`)

## Project Structure

```
Engine/
├── Core/           # KEngine - 윈도우, 메인 루프, 서브시스템 관리, ISubsystem 인터페이스, KSubsystemRegistry
├── Graphics/       # 렌더링 파이프라인 (Forward/Deferred, PBR, Shadow, PostProcess, IBL, 등)
├── Input/          # 키보드, 마우스, 액션 매핑
├── Audio/          # XAudio2 기반 오디오, 3D 사운드
├── Physics/        # 리지드바디, 충돌 감지, 물리 월드
├── Scene/          # Actor-Component 시스템, 씬 관리
├── Assets/         # 정적/스켈레탈 메시, 애니메이션, 모델 로더(Assimp)
├── Serialization/  # 바이너리/JSON 아카이브
├── UI/             # 캔버스 기반 UI 시스템
├── DebugUI/        # ImGui 디버그 오버레이
└── Utils/          # Common.h, Logger.h, Math.h

Editor/
├── EngineInterop/  # C API DLL (extern "C" P/Invoke)
└── KojeomEditor/   # C# WPF 에디터 (.NET 8.0)

samples/            # 15개 샘플 프로젝트
```

## Shader Architecture

엔진은 두 클래스로 구성된 셰이더 시스템을 사용합니다:
- **`KShader`** — 개별 컴파일된 셰이더(버텍스 또는 픽셀), 인라인 HLSL 소스 문자열에서 `KShader::CompileFromString()`으로 생성
- **`KShaderProgram`** — 버텍스 + 픽셀 셰이더 페어와 입력 레이아웃, 상수 버퍼 리플렉션을 결합
- **`EShaderType`** enum: `Vertex`, `Pixel`
- **중요**: 저장소에 `.hlsl` 셰이더 파일이 존재하지 않습니다. 모든 셰이더는 인라인 C++ 문자열 리터럴로 임베드되어 런타임에 컴파일됩니다. 새 셰이더를 추가할 때는 별도 `.hlsl` 파일이 아닌 인라인 문자열 리터럴로 정의하십시오.

## Architecture Patterns

- **Entity-Component**: `KActor`가 `KActorComponent` 소유 (`GetComponent<T>()`)
  - 엔진 컴포넌트: `KStaticMeshComponent`, `KSkeletalMeshComponent`, `KTerrainComponent`, `KWaterComponent`
  - `EComponentType`: Base(0), StaticMesh(1), SkeletalMesh(2), Water(3), Terrain(4)
- **Singletons**: `KEngine`, `KInputManager`, `KAudioManager`, `KPhysicsWorld`, `KDebugUI`
  - `KAudioManager`와 `KPhysicsWorld`는 `ISubsystem` 어댑터 래퍼도 제공:
    - `KAudioSubsystem`이 `KAudioManager`를 래핑
    - `KPhysicsSubsystem`이 `KPhysicsWorld`를 래핑
    - `KEngine::GetSubsystem<T>()`으로 싱글톤 `GetInstance()` 대신 접근 가능
  - `ESubsystemState` enum: `Uninitialized`, `Initialized`, `Running`, `Shutdown`
- **C#/C++ Interop**: `EngineInterop.dll` flat C API (107 functions) -> C# P/Invoke

## Git Commit Format

```
[Category] Brief description

- Change list
```

Categories: `[Core]`, `[Graphics]`, `[Input]`, `[Audio]`, `[Physics]`, `[Scene]`, `[UI]`, `[Editor]`, `[Docs]`, `[Build]`, `[Refactor]`, `[Fix]`, `[Feature]`

## Mandatory Rules

1. 기존 코드를 먼저 읽고 패턴을 파악한 후 변경할 것
2. 기존 네이밍 컨벤션을 엄격하게 따를 것
3. API 변경 시 EngineInterop.h/.cpp 및 C# 에디터 바인딩도 함께 업데이트할 것
4. 리소스 소유 클래스는 복사 생성자/대입 연산자를 삭제할 것
5. 원시 포인터로 리소스를 소유하지 말 것 (ComPtr, smart pointer 사용)
6. 컴파일러 경고를 무시하지 말 것
7. 서드파티 라이브러리 코드(`Engine/third_party/`)를 수정하지 말 것
8. 변경 사항은 문서(docs/)에 반영할 것

## C# Editor Code Style

- File-scoped namespaces: `namespace KojeomEditor;`
- Classes/properties/methods: PascalCase
- Private fields: `_camelCase` with underscore prefix (`_engine`, `_viewModel`)
- Local variables: camelCase
- MVVM pattern with `INotifyPropertyChanged` and `RelayCommand`
- P/Invoke via `DllImport` with `CallingConvention.Cdecl`
- `IDisposable` for native resource management

## Documentation

- 모든 문서는 Markdown 형식으로 `docs/` 폴더 아래에 위치
- 규칙 문서: `docs/rules/`
- 기술 문서: `docs/` 하위 폴더
- 코드 주석: 공개 API에 Doxygen 스타일

## Prohibited Actions

- 서드파티 코드(`Engine/third_party/`) 수정
- main 브랜치에 디버그/테스트 코드 커밋
- 기존 API 파괴적 변경 없이 사용처 업데이트 누락
- 승인 없는 새로운 의존성 추가
- `.hlsl` 셰이더 파일 생성 (모든 셰이더는 인라인 C++ 문자열 리터럴로 정의하고 `KShader::CompileFromString()`으로 런타임 컴파일해야 함)
- `EngineAPI.h`/`EngineAPI.cpp`에 새 C API 함수 추가 시 `Editor/KojeomEditor/Services/EngineInterop.cs`에 해당 C# `DllImport` 선언 누락 (현재 107개 C API 함수 모두 C# 바인딩이 완료되어 있음)

## Build Verification

변경 후 반드시 빌드 테스트를 수행하십시오:

```bash
# C++ 전체 빌드
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64

# C# 에디터 빌드
dotnet build Editor/KojeomEditor/KojeomEditor.csproj
```

빌드 성공 후 커밋하고 오리진에 반영하십시오.

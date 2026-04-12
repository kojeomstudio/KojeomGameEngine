# 시작하기

## 필수 조건

| 요구사항 | 버전 |
|---------|------|
| Visual Studio | 2022 (v143 빌드 도구) |
| MSVC 컴파일러 | v143 |
| C++ 표준 | C++17 |
| 플랫폼 | x64 전용 |
| .NET SDK | 8.0 이상 (에디터용) |
| Windows SDK | 10.0 이상 |
| DirectX | 11 (Shader Model 5.0) |
| Git | 최신 버전 (소스 체크아웃용) |

## 프로젝트 구조

```
KojeomEngine/
├── KojeomEngine.sln          # 메인 솔루션 파일
├── AGENTS.md                  # AI 에이전트 가이드
├── Engine/                    # 코어 엔진 (정적 라이브러리, .lib)
│   ├── Core/                  # KEngine, ISubsystem, KSubsystemRegistry
│   ├── Graphics/              # 렌더링 시스템 (DX11)
│   ├── Input/                 # 입력 관리
│   ├── Audio/                 # 오디오 (XAudio2)
│   ├── Physics/               # 물리 엔진
│   ├── Scene/                 # 액터-컴포넌트, 씬 관리
│   ├── Assets/                # 에셋 로딩 (Assimp), 애니메이션
│   ├── Serialization/         # 바이너리/JSON 직렬화
│   ├── UI/                    # 캔버스 UI 시스템
│   ├── DebugUI/               # ImGui 디버그 오버레이
│   ├── Utils/                 # 공통 유틸리티
│   ├── third_party/           # 서드파티 라이브러리
│   └── Engine.vcxproj         # 엔진 프로젝트 파일
├── Editor/
│   ├── EngineInterop/         # C++/C# 브릿지 DLL
│   │   ├── EngineAPI.h        # Flat C API 헤더
│   │   ├── EngineAPI.cpp      # API 구현
│   │   └── EngineInterop.vcxproj
│   └── KojeomEditor/          # C# WPF 에디터
│       ├── KojeomEditor.csproj
│       ├── Services/          # P/Invoke 바인딩
│       ├── ViewModels/        # MVVM 뷰모델
│       ├── Views/             # XAML 뷰
│       └── Styles/            # XAML 스타일
├── samples/                   # 샘플 프로젝트 (16개)
│   ├── BasicRendering/        # 기본 렌더링
│   ├── Lighting/              # 조명
│   ├── PBR/                   # PBR 재질
│   ├── PostProcessing/        # 후처리 효과
│   ├── Sky/                   # 하늘 렌더링
│   ├── Water/                 # 물 렌더링
│   ├── Terrain/               # 지형 렌더링
│   ├── SkeletalMesh/          # 스키널 메시
│   ├── AnimationStateMachine/ # 애니메이션 상태 머신
│   ├── Particles/             # 파티클 시스템
│   ├── Physics/               # 물리 시뮬레이션
│   ├── UI/                    # UI 시스템
│   ├── DebugRendering/        # 디버그 렌더링
│   ├── Gameplay/              # 게임플레이
│   └── LOD/                   # LOD 시스템
├── docs/                      # 문서
└── lib/                       # 외부 라이브러리
```

## 빌드 방법

### 전체 솔루션 빌드

```bash
# Debug 빌드
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64

# Release 빌드
msbuild KojeomEngine.sln /p:Configuration=Release /p:Platform=x64
```

또는 Visual Studio 2022에서 `KojeomEngine.sln`을 열고 `Ctrl+Shift+B`로 빌드합니다.

### 개별 프로젝트 빌드

```bash
# 엔진 라이브러리만 빌드
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64 /t:Engine

# 특정 샘플 빌드
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64 /t:BasicRenderingSample
```

### C# 에디터 빌드

```bash
# Debug 빌드
dotnet build Editor/KojeomEditor/KojeomEditor.csproj

# Release 빌드
dotnet build Editor/KojeomEditor/KojeomEditor.csproj -c Release
```

> **주의**: 에디터를 실행하려면 먼저 `EngineInterop` 프로젝트가 빌드되어 있어야 합니다. 솔루션 빌드 시 자동으로 빌드됩니다.

## 샘플 프로젝트 실행

### Visual Studio에서 실행
1. `KojeomEngine.sln`을 엽니다.
2. 솔루션 탐색기에서 실행할 샘플 프로젝트를 **시작 프로젝트로 설정**합니다. (우클릭 → 시작 프로젝트로 설정)
3. `F5` 키를 눌러 디버그 실행합니다.

### 명령줄에서 실행
```bash
# 샘플 빌드 후 생성된 실행 파일 경로
# samples/<SampleName>/x64/Debug/<SampleName>.exe
# samples/<SampleName>/x64/Release/<SampleName>.exe
```

### 디버그 콘솔
Debug 빌드에서는 자동으로 디버그 콘솔이 생성됩니다. `LOG_INFO`, `LOG_WARNING`, `LOG_ERROR` 매크로의 출력이 콘솔과 Visual Studio 출력 창에 표시됩니다.

### 메모리 누수 감지
Debug 빌드에서는 `_CrtSetDbgFlag`를 통해 메모리 누수 감지가 활성화됩니다. 프로그램 종료 시 누수가 감지되면 Visual Studio 출력 창에 리포트됩니다.

## 빌드 설정

| 설정 | 값 |
|-----|-----|
| 플랫폼 | x64 (필수, x86 지원 안 함) |
| 구성 | Debug / Release |
| C++ 표준 | ISO C++17 Standard (/std:c++17) |
| 문자셋 | 유니코드 |
| 셰이더 모델 | 5.0 (vs_5_0, ps_5_0) |
| Windows SDK | 10.0 |
| .NET | 8.0 (에디터) |

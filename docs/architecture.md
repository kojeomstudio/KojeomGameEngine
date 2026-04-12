# KojeomGameEngine 아키텍처

## 전체 엔진 아키텍처

KojeomGameEngine은 C++17 / DirectX 11 기반의 게임 엔진으로, 모듈형 서브시스템 아키텍처를 채택하고 있습니다. WPF 기반의 C# 에디터를 통해 시각적 편집이 가능하며, 엔진 코어는 정적 라이브러리(.lib)로 빌드됩니다.

```
┌─────────────────────────────────────────────────────┐
│                    애플리케이션                        │
│              (KEngine 상속 클래스)                     │
├─────────────────────────────────────────────────────┤
│              KEngine (코어)                           │
│  ┌───────────┐ ┌────────────┐ ┌──────────────────┐  │
│  │ KGraphics  │ │ KRenderer  │ │ KSceneManager    │  │
│  │ Device     │ │            │ │                  │  │
│  └───────────┘ └────────────┘ └──────────────────┘  │
│  ┌───────────┐ ┌────────────┐ ┌──────────────────┐  │
│  │ KCamera    │ │ KInputMgr  │ │ KSubsystemReg.  │  │
│  └───────────┘ └────────────┘ └──────────────────┘  │
├─────────────────────────────────────────────────────┤
│              모듈형 서브시스템 (ISubsystem)            │
│  ┌──────────────┐ ┌──────────────────┐               │
│  │ KAudioSubsys. │ │ KPhysicsSubsys.  │               │
│  └──────────────┘ └──────────────────┘               │
├─────────────────────────────────────────────────────┤
│              DirectX 11 / Win32                       │
└─────────────────────────────────────────────────────┘
```

## 모듈 설명

### Core (`Engine/Core/`)

엔진의 핵심 모듈로, 메인 루프, 윈도우 관리, 하위 시스템 소유 및 초기화를 담당합니다.

- **KEngine**: 엔진 메인 클래스. 초기화, 메인 루프, 종료를 관리합니다. 싱글턴 패턴으로 전역 접근을 제공합니다. `RunApplication<T>()` 템플릿 메서드로 애플리케이션 실행을 간소화합니다.
- **ISubsystem**: 모든 모듈형 서브시스템의 베이스 인터페이스. `Initialize()`, `Tick()`, `Shutdown()` 생명주기를 정의합니다.
- **KSubsystemRegistry**: 서브시스템 등록, 타입 기반 조회, 순차적 초기화/종료(역순)를 관리하는 레지스트리입니다.

### Graphics (`Engine/Graphics/`)

DirectX 11 기반의 렌더링 시스템으로, 가장 큰 모듈입니다.

| 서브모듈 | 설명 |
|---------|------|
| `GraphicsDevice` | DX11 디바이스, 컨텍스트, 스왑체인 관리 |
| `Renderer` | 프레임 관리, 렌더링 파이프라인 오케스트레이션 |
| `Camera` | 투영/직교 투영, 뷰/프로젝션 행렬 |
| `Shader` | 셰이더 로딩, 컴파일, 관리 (VS, PS, GS 등) |
| `Mesh` | 정점/인덱스 버퍼, 기본 프리미티브 생성 |
| `Material` | PBR 재질 파라미터 및 텍스처 관리 |
| `Texture` | 텍스처 로딩, 샘플러 상태, 텍스처 캐시 |
| `Light` | 방향광, 점광원, 스포트라이트 정의 |
| `Shadow/` | 섀도우 맵, 섀도우 렌더러 |
| `Deferred/` | G-Buffer, 디퍼드 렌더러 |
| `PostProcess/` | HDR, 블룸, FXAA, 컬러 그레이딩, 톤매핑, 오토 익스포저, 컬러 그레이딩, 모션 블러, DOF, 렌즈 효과 |
| `SSAO/` | 스크린 공간 앰비언트 오클루전 |
| `SSR/` | 스크린 공간 반사 |
| `SSGI/` | 스크린 공간 글로벌 일루미네이션 |
| `TAA/` | 시간적 안티앨리어싱 |
| `Sky/` | 절차적 대기 렌더링 (Rayleigh/Mie 산란) |
| `Volumetric/` | 볼류메트릭 포그 |
| `Water/` | 물 렌더링 (파동, 반사/굴절, 폼) |
| `Terrain/` | 지형 렌더링 (하이트맵, 스플랫맵, LOD, 펄린 노이즈) |
| `IBL/` | 이미지 기반 조명 (조사도 맵, 프리필터드 환경맵, BRDF LUT) |
| `Culling/` | 절두체 컬링, 오클루전 쿼리 |
| `Instanced/` | GPU 인스턴스드 렌더링 |
| `LOD/` | LOD 생성 및 LOD 시스템 (화면 커버리지 기반) |
| `Particle/` | 파티클 시스템 (빌보드, 텍스처 아틀라스) |
| `Performance/` | GPU 타이머, 프레임 통계 |
| `CommandBuffer/` | 디퍼드 커맨드 버퍼 (정렬, 컬링) |
| `Debug/` | 디버그 렌더러 |

### Input (`Engine/Input/`)

입력 시스템으로, 키보드, 마우스, Raw Input을 지원합니다.

- **KInputManager**: 키 상태 추적, 마우스 위치/델타, 액션 매핑, 콜백 시스템
- **InputTypes**: 키코드, 마우스 버튼, 입력 상태 정의

### Audio (`Engine/Audio/`)

XAudio2 기반의 오디오 시스템입니다.

- **KAudioManager**: 사운드 로딩, 재생, 일시정지, 3D 사운드 지원
- **KAudioSubsystem**: ISubsystem 어댑터
- **KSound**: 개별 사운드 리소스 (WAV 파싱)
- **AudioTypes**: 오디오 설정, 사운드 파라미터, 리스너 파라미터

### Physics (`Engine/Physics/`)

자체 구현된 물리 엔진입니다.

- **KPhysicsWorld**: 물리 시뮬레이션 월드, 충돌 감지(Broad/Narrow Phase), 레이캐스트
- **KPhysicsSubsystem**: ISubsystem 어댑터
- **KRigidBody**: 강체 (구, 박스, 캡슐 콜라이더)
- **PhysicsTypes**: 충돌 타입, 물리 재질, 충돌 정보

### Scene (`Engine/Scene/`)

액터-컴포넌트 시스템과 씬 관리를 담당합니다.

- **KActor**: 게임 오브젝트. 컴포넌트 소유, 부모-자식 계층 구조, 변환 관리
- **KActorComponent**: 컴포넌트 베이스 클래스. `Tick()`, `Render()` 가상 메서드
- **KScene**: 액터 컨테이너. 생성/삭제/조회, 직렬화 지원
- **KSceneManager**: 씬 관리자. 여러 씬 관리, 액티브 씬 전환

### Assets (`Engine/Assets/`)

에셋 로딩 및 관리 모듈입니다.

- **KModelLoader**: Assimp 기반 모델 로더 (OBJ, GLTF, FBX 파싱 포함)
- **KStaticMesh / KStaticMeshComponent**: 정적 메시 및 컴포넌트
- **KSkeletalMesh / KSkeletalMeshComponent**: 스키널 메시 및 컴포넌트 (최대 4개 본 영향, 256개 스키닝 본)
- **KSkeleton**: 본 계층 구조, 바인드 포즈, 역바인드 포즈
- **KAnimation**: 애니메이션 채널 (위치/회전/스케일 키프레임 보간)
- **KAnimationInstance**: 애니메이션 재생 인스턴스
- **KAnimationStateMachine**: 애니메이션 상태 머신 (상태 전이, 블렌딩, 파라미터, 노티파이)

### Serialization (`Engine/Serialization/`)

데이터 직렬화 시스템입니다.

- **KBinaryArchive**: 바이너리 아카이브 (읽기/쓰기, `<<`/`>>` 연산자 오버로딩, 기본 타입 및 XMFLOAT 지원)
- **KJsonArchive**: 커스텀 JSON 파서 (DOM 기반, 직렬화/역직렬화)

### UI (`Engine/UI/`)

DirectX 11 기반의 캔버스 UI 시스템입니다.

- **KUICanvas**: UI 컨테이너, 배칭 렌더링, 입력 이벤트 처리
- **KUIElement**: UI 요소 베이스 클래스 (위치, 크기, 앵커, 정렬, 히트 테스트)
- **UIButton, UIText, UICheckbox, UISlider, UIImage, UIPanel**: 구체적인 UI 위젯
- **UILayout, UIGridLayout, UIHorizontalLayout, UIVerticalLayout**: 레이아웃 시스템
- **KUIFont**: 폰트 렌더링

### DebugUI (`Engine/DebugUI/`)

ImGui 기반의 디버그 오버레이입니다.

- **KDebugUI**: ImGui 초기화, 프레임 통계 오버레이, 디버그 패널 등록 시스템

### Utils (`Engine/Utils/`)

유틸리티 모듈입니다.

- **Common.h**: 타입 별칭 (uint8, int32 등), ComPtr, 매크로 (SAFE_RELEASE, CHECK_HRESULT, LOG_*)
- **Math.h**: FBoundingBox, FBoundingSphere, FTransform, FMaterialSlot
- **Logger.h**: KLogger 클래스 (Info/Warning/Error, OutputDebugString 출력)

## 에디터 아키텍처

### EngineInterop 브릿지

C# WPF 에디터와 C++ 엔진 간의 통신 브릿지입니다. `EngineAPI.h/cpp`에 정의된 flat C API(`extern "C"`)를 제공하여, C#에서 P/Invoke로 호출합니다.

```
┌──────────────────────────┐     P/Invoke     ┌──────────────────────┐
│   KojeomEditor (WPF)     │ ◄──────────────► │   EngineInterop.dll  │
│   C# / .NET 8.0          │  (CallingConv.   │   extern "C" API     │
│   MVVM 패턴              │   Cdecl)         │   void* 기반 인터페이스│
└──────────────────────────┘                  └──────────┬───────────┘
                                                       │
                                                       ▼
                                              ┌──────────────────┐
                                              │   Engine.lib     │
                                              │   KEngine API    │
                                              └──────────────────┘
```

### WPF 에디터 구조

| 디렉토리 | 설명 |
|---------|------|
| `ViewModels/` | MVVM ViewModel (MainViewModel, SceneViewModel, PropertiesViewModel, ComponentViewModel) |
| `Views/` | XAML 뷰 |
| `Services/` | EngineInterop (P/Invoke 바인딩), UndoRedoService |
| `Styles/` | XAML 스타일 |

## 핵심 디자인 패턴

### 1. 싱글턴 패턴
- `KEngine::GetInstance()`: 전역 엔진 인스턴스
- `KInputManager::GetInstance()`: 전역 입력 관리자
- `KAudioManager::GetInstance()`: 전역 오디오 관리자
- `KDebugUI::GetInstance()`: 전역 디버그 UI

### 2. 모듈형 서브시스템 패턴
- `ISubsystem` 인터페이스: `Initialize()`, `Tick()`, `Shutdown()` 생명주기
- `KSubsystemRegistry`: 타입 기반 등록/조회, 순차적 초기화/역순 종료
- `KAudioSubsystem`, `KPhysicsSubsystem`: ISubsystem 어댑터

### 3. 엔티티-컴포넌트 패턴
- `KActor`: 컴포넌트 컨테이너 (AddComponent, GetComponent<T>)
- `KActorComponent`: 컴포넌트 베이스 (Tick, Render, Serialize/Deserialize)
- `KStaticMeshComponent`, `KSkeletalMeshComponent`, `KWaterComponent`, `KTerrainComponent`

### 4. 커맨드 버퍼 패턴
- `KCommandBuffer`: 렌더 커맨드를 버퍼링, 정렬(Shader→Texture→Material→Depth), 컬링 후 일괄 실행

### 5. 상태 머신 패턴
- `KAnimationStateMachine`: 애니메이션 상태, 전이 조건, 블렌딩, 노티파이

### 6. MVVM 패턴 (에디터)
- ViewModel: `INotifyPropertyChanged`, `RelayCommand`
- View: XAML 데이터 바인딩
- Service: P/Invoke 브릿지

## 렌더링 파이프라인 개요

### 포워드 렌더링 파이프라인
1. 섀도우 패스 (옵션)
2. 볼류메트릭 포그 패스 (옵션)
3. 불투명 오브젝트 렌더링 (Basic / Phong / PBR 셰이더)
4. SSAO 패스 (옵션, G-Buffer 필요시 디퍼드)
5. SSR 패스 (옵션)
6. SSGI 패스 (옵션)
7. HDR 후처리 파이프라인 (블룸 → 톤매핑 → FXAA → 컬러 그레이딩)
8. 모션 블러 (옵션)
9. 피사계 심도 (옵션)
10. 렌즈 효과 (옵션)
11. TAA (옵션)
12. 투명 오브젝트 포워드 패스
13. 디버그 UI (ImGui)

### 디퍼드 렌더링 파이프라인
1. 섀도우 패스 (옵션)
2. 지오메트리 패스 → G-Buffer (Normal, Albedo, Position, Depth)
3. SSAO 패스
4. 조명 패스 (Fullscreen Quad, 다중 광원)
5. SSR, SSGI 패스
6. HDR 후처리 파이프라인
7. 투명 오브젝트 포워드 패스
8. 디버그 UI

### 렌더 패스 전환
`KRenderer::SetRenderPath(ERenderPath::Forward)` 또는 `ERenderPath::Deferred`로 전환 가능합니다.

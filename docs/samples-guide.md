# 샘플 프로젝트 가이드

KojeomEngine은 엔진의 각 기능을 시연하기 위한 15개의 샘플 프로젝트를 제공합니다. 모든 샘플은 `samples/` 디렉토리에 위치하며, `KEngine`을 상속받아 구현됩니다.

## 샘플 목록

| 샘플 | 디렉토리 | 설명 |
|------|---------|------|
| BasicRenderingSample | `samples/BasicRendering/` | 기본 렌더링 |
| LightingSample | `samples/Lighting/` | 조명 시스템 |
| PBRSample | `samples/PBR/` | PBR 재질 |
| PostProcessingSample | `samples/PostProcessing/` | 후처리 효과 |
| SkySample | `samples/Sky/` | 하늘 렌더링 |
| WaterSample | `samples/Water/` | 물 렌더링 |
| TerrainSample | `samples/Terrain/` | 지형 렌더링 |
| SkeletalMeshSample | `samples/SkeletalMesh/` | 스키널 메시 애니메이션 |
| AnimationStateMachineSample | `samples/AnimationStateMachine/` | 애니메이션 상태 머신 |
| ParticlesSample | `samples/Particles/` | 파티클 시스템 |
| PhysicsSample | `samples/Physics/` | 물리 시뮬레이션 |
| UISample | `samples/UI/` | UI 시스템 |
| LayoutSample | `samples/UI/Layout/` | UI 레이아웃 |
| DebugRenderingSample | `samples/DebugRendering/` | 디버그 렌더링 |
| GameplaySample | `samples/Gameplay/` | 게임플레이 |
| LODSample | `samples/LOD/` | LOD 시스템 |

## 각 샘플 상세 설명

### BasicRenderingSample
**위치**: `samples/BasicRendering/`

엔진의 기본 렌더링 기능을 시연합니다.
- `CreateCubeMesh()`, `CreateSphereMesh()` 등 프리미티브 생성
- 기본 색상 셰이더와 조명 셰이더 사용
- 카메라 이동 (WASD + 마우스)
- 프레임 레이트 표시

### LightingSample
**위치**: `samples/Lighting/`

다중 광원 시스템을 시연합니다.
- 방향광 설정 및 조정
- 최대 8개 점광원 추가/제거
- 최대 4개 스포트라이트
- 섀도우 맵핑
- 광원 볼륨 디버그 렌더링

### PBRSample
**위치**: `samples/PBR/`

PBR (물리 기반 렌더링) 재질 시스템을 시연합니다.
- FPBRMaterialParams 설정 (Albedo, Metallic, Roughness, AO)
- 내장 재질 프리셋 (Metal, Plastic, Rubber, Gold, Silver, Copper)
- PBR 텍스처 슬롯 (Albedo, Normal, Metallic, Roughness, AO)
- IBL (이미지 기반 조명) 환경맵 적용

### PostProcessingSample
**위치**: `samples/PostProcessing/`

후처리 효과를 시연합니다.
- HDR 렌더링
- 블룸 (밝은 영역 추출, 가우시안 블러, 합성)
- 톤매핑 (Reinhard, ACES 등)
- FXAA 안티앨리어싱
- 오토 익스포저
- 컬러 그레이딩 (LUT)
- 모션 블러
- 피사계 심도
- 렌즈 효과 (색수차, 비네팅, 필름 그레인)

### SkySample
**위치**: `samples/Sky/`

절차적 하늘 렌더링을 시연합니다.
- Rayleigh / Mie 산란 기반 대기 렌더링
- 태양 방향 및 강도 설정
- 시간에 따른 하늘 변화 (`SetTimeOfDay`)
- 대기 파라미터 조정 (산란 계수, 높이, 이방성)
- 클라우드 렌더링

### WaterSample
**위치**: `samples/Water/`

물 렌더링 시스템을 시연합니다.
- Gerstner 파동 (최대 4파)
- 평면 반사
- 법선 기반 굴절 (DuDv 맵)
- 프레넬 효과
- 폼 렌더링
- 노멀 맵 블렌딩
- `KWaterComponent`를 통한 액터-컴포넌트 연동

### TerrainSample
**위치**: `samples/Terrain/`

지형 렌더링 시스템을 시연합니다.
- Perlin 노이즈 하이트맵 생성
- RAW 파일 / 이미지 파일에서 하이트맵 로딩
- 스플랫 맵 기반 다중 텍스처 레이어
- LOD (Level of Detail) 시스템
- 지형 위 카메라 이동
- `KTerrainComponent`를 통한 액터-컴포넌트 연동

### SkeletalMeshSample
**위치**: `samples/SkeletalMesh/`

스키널 메시 애니메이션을 시연합니다.
- Assimp를 통한 스키널 모델 로딩
- 본 계층 구조 및 바인드 포즈
- 스키닝 (최대 256개 본, 최대 4개 본 영향)
- 애니메이션 재생 (재생/일시정지/정지)
- `KSkeletalMeshComponent`를 통한 액터-컴포넌트 연동

### AnimationStateMachineSample
**위치**: `samples/AnimationStateMachine/`

애니메이션 상태 머신을 시연합니다.
- 애니메이션 상태 정의 및 관리
- 상태 전이 (조건부, ExitTime 기반)
- 블렌딩 (크로스페이드)
- 플로트/불 파라미터 기반 전이 조건
- 애니메이션 노티파이
- `KAnimationStateMachine` 클래스 사용

### ParticlesSample
**위치**: `samples/Particles/`

파티클 시스템을 시연합니다.
- 다양한 이미터 형태 (Point, Sphere, Cone, Box)
- 텍스처 아틀라스 지원
- 빌보드 렌더링
- 입자 수명, 속도, 크기, 색상 보간
- 중력 영향
- 버스트 방출
- Additive 블렌딩

### PhysicsSample
**위치**: `samples/Physics/`

물리 시뮬레이션을 시연합니다.
- 강체 생성 (구, 박스, 캡슐 콜라이더)
- 중력 및 질량 설정
- 힘/임펄스 적용
- 충돌 감지 및 응답
- 레이캐스트
- 물리 재질 (마찰, 반발)
- `KPhysicsSubsystem`을 통한 서브시스템 연동

### UISample
**위치**: `samples/UI/`

캔버스 UI 시스템을 시연합니다.
- `KUICanvas` 초기화 및 렌더링
- `KUIElement` 기반 위젯 (Button, Text, Checkbox, Slider, Image, Panel)
- 히트 테스트 및 클릭 이벤트
- 마우스 호버/클릭 콜백

### LayoutSample
**위치**: `samples/UI/Layout/`

UI 레이아웃 시스템을 시연합니다.
- `UIGridLayout` 그리드 레이아웃
- `UIHorizontalLayout` 수평 레이아웃
- `UIVerticalLayout` 수직 레이아웃
- 앵커 및 정렬 시스템
- 중첩 레이아웃

### DebugRenderingSample
**위치**: `samples/DebugRendering/`

디버그 렌더링 기능을 시연합니다.
- 그리드 그리기 (`DebugRenderer_DrawGrid`)
- 좌표축 그리기 (`DebugRenderer_DrawAxis`)
- 디버그 모드 (와이어프레임) 토글
- 광원 볼륨 디버그 렌더링
- ImGui 디버그 UI 패널

### GameplaySample
**위치**: `samples/Gameplay/`

게임플레이 시나리오를 시연합니다.
- 여러 엔진 기능의 통합 사용
- 액터-컴포넌트 패턴 활용
- 입력 기반 상호작용

### LODSample
**위치**: `samples/LOD/`

LOD (Level of Detail) 시스템을 시연합니다.
- 화면 커버리지 기반 LOD 선택
- 거리 기반 LOD 선택
- LOD 블렌딩 (부드러운 전환)
- LOD 바이어스 설정
- `KLODSystem` 및 `KLODGenerator` 사용

## 빌드 및 실행 방법

### 전체 샘플 빌드

```bash
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64
```

### 개별 샘플 빌드

```bash
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64 /t:BasicRenderingSample
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64 /t:PBRSample
```

### 실행

1. Visual Studio 2022에서 `KojeomEngine.sln`을 엽니다.
2. 솔루션 탐색기에서 원하는 샘플 프로젝트를 **시작 프로젝트로 설정**합니다.
3. `F5` (디버그 시작) 또는 `Ctrl+F5` (디버그 없이 시작)를 누릅니다.

### 실행 파일 위치

빌드 후 각 샘플의 실행 파일은 다음 경로에 생성됩니다:

```
samples/<SampleName>/x64/Debug/<SampleName>.exe
samples/<SampleName>/x64/Release/<SampleName>.exe
```

### 공통 조작법

모든 샘플에서 기본적으로 다음 조작이 가능합니다:
- **WASD**: 카메라 전후좌우 이동
- **마우스 우클릭 + 드래그**: 카메라 회전
- **마우스 휠**: 줌 인/아웃
- **ESC**: 종료
- **F1**: 디버그 UI 토글 (지원하는 샘플)

# 렌더링 파이프라인

## 포워드 렌더링 파이프라인

KojeomEngine의 기본 렌더링 파이프라인은 포워드 렌더링입니다. 각 오브젝트를 순차적으로 렌더링하며, 셰이더 내에서 조명 계산을 수행합니다.

```
┌───────────────────────────────────────────┐
│         프레임 시작 (BeginFrame)          │
├───────────────────────────────────────────┤
│  1. 섀도우 패스 (옵션)                    │
│     └─ 광원 시점에서 깊이 전용 렌더링       │
├───────────────────────────────────────────┤
│  2. 스카이 렌더링 (옵션)                   │
│     └─ 절차적 대기 렌더링 (Rayleigh/Mie)   │
├───────────────────────────────────────────┤
│  3. HDR 렌더 타겟으로 렌더링 시작          │
├───────────────────────────────────────────┤
│  4. 볼류메트릭 포그 (옵션)                 │
│     └─ Ray-marching 기반 볼류메트릭 포그    │
├───────────────────────────────────────────┤
│  5. 불투명 오브젝트 렌더링                 │
│     ├─ Basic 셰이더 (단색)                │
│     ├─ Phong 셰이더 (광원 1개)             │
│     └─ PBR 셰이더 (다중 광원 + PBR)        │
├───────────────────────────────────────────┤
│  6. 스크린 공간 효과 (옵션)               │
│     ├─ SSAO                               │
│     ├─ SSR                                │
│     └─ SSGI                               │
├───────────────────────────────────────────┤
│  7. 후처리 파이프라인                      │
│     ├─ 블룸 (밝은 영역 추출 → 블러 → 합성) │
│     ├─ 오토 익스포저                       │
│     ├─ 톤매핑                              │
│     ├─ FXAA                               │
│     ├─ 컬러 그레이딩                       │
│     ├─ 모션 블러                          │
│     ├─ 피사계 심도                         │
│     └─ 렌즈 효과 (CA, 비네팅, 필름 그레인)   │
├───────────────────────────────────────────┤
│  8. TAA (옵션)                            │
├───────────────────────────────────────────┤
│  9. 투명 오브젝트 포워드 패스               │
├───────────────────────────────────────────┤
│  10. 디버그 UI (ImGui)                     │
├───────────────────────────────────────────┤
│         프레임 종료 (EndFrame)             │
└───────────────────────────────────────────┘
```

## 디퍼드 렌더링 파이프라인

디퍼드 렌더링은 지오메트리 정보를 G-Buffer에 먼저 렌더링한 후, 별도의 조명 패스에서 화면 공간에서 조명을 계산합니다.

### G-Buffer 구성

```
G-Buffer RT0: Normal (R16G16B16A16_FLOAT)
G-Buffer RT1: Albedo + Metallic + Roughness (R8G8B8A8_UNORM)
G-Buffer RT2: World Position (R32G32B32A32_FLOAT)
Depth:        DepthStencil (D24_UNORM_S8_UINT)
```

### 디퍼드 파이프라인 흐름

```
1. 섀도우 패스
2. 지오메트리 패스 → G-Buffer에 프래그먼트 데이터 기록
   - 불투명 오브젝트만 G-Buffer에 렌더링
3. SSAO 패스 (G-Buffer Normal + Position 활용)
4. 조명 패스
   - Fullscreen Quad 렌더링
   - G-Buffer + 광원 데이터로 조명 계산
   - 방향광 + 최대 8개 점광원 + 최대 4개 스포트라이트
5. SSR, SSGI 패스
6. HDR 후처리 파이프라인
7. 투명 오브젝트 포워드 패스 (디퍼드 대상 제외)
8. 디버그 UI
```

## PBR 재질과 조명

### PBR 파라미터 (`FPBRMaterialParams`)

| 파라미터 | 기본값 | 설명 |
|---------|--------|------|
| `AlbedoColor` | (1, 1, 1, 1) | 알베도 색상 |
| `Metallic` | 0.0 | 금속성 (0=비금속, 1=금속) |
| `Roughness` | 0.5 | 거칠기 (0=완전 반사, 1=완전 확산) |
| `AO` | 1.0 | 앰비언트 오클루전 |
| `EmissiveIntensity` | 0.0 | 자발광 강도 |
| `EmissiveColor` | (0, 0, 0, 1) | 자발광 색상 |
| `NormalIntensity` | 1.0 | 노멀 맵 강도 |
| `HeightScale` | 0.0 | 패럴랙스 오프셋 스케일 |

### 텍스처 슬롯 (`EMaterialTextureSlot`)

| 슬롯 | 인덱스 | 설명 |
|------|--------|------|
| `Albedo` | 0 | 알베도/디퓨즈 텍스처 |
| `Normal` | 1 | 노멀 맵 |
| `Metallic` | 2 | 금속성 맵 |
| `Roughness` | 3 | 거칠기 맵 |
| `AO` | 4 | 앰비언트 오클루전 맵 |
| `Emissive` | 5 | 자발광 맵 |
| `Height` | 6 | 높이 맵 |

### 재질 프리셋

```cpp
FPBRMaterialParams::Default()   // 기본값
FPBRMaterialParams::Metal()     // 금속 (M=1.0, R=0.3)
FPBRMaterialParams::Plastic()   // 플라스틱 (M=0.0, R=0.4)
FPBRMaterialParams::Rubber()    // 고무 (M=0.0, R=0.9)
FPBRMaterialParams::Gold()      // 금 (알베도 + M=1.0, R=0.3)
FPBRMaterialParams::Silver()    // 은 (알베도 + M=1.0, R=0.3)
FPBRMaterialParams::Copper()    // 구리 (알베도 + M=1.0, R=0.35)
```

### 조명 시스템

- **방향광** (`FDirectionalLight`): 방향, 강도, 색상, 환경색. 1개만 지원.
- **점광원** (`FPointLight`): 위치, 강도, 색상, 반경, 감쇠. 최대 8개.
- **스포트라이트** (`FSpotLight`): 위치, 방향, 색상, 내/외부 원뿔, 반경, 감쇠. 최대 4개.

## 섀도우 맵핑

### KShadowMap (`Engine/Graphics/Shadow/ShadowMap.h`)

- 기본 해상도: 2048x2048
- Depth Bias와 Slope Scaled Depth Bias 지원
- PCF (Percentage Closer Filtering) 소프트 섀도우

### KShadowRenderer (`Engine/Graphics/Shadow/ShadowRenderer.h`)

```cpp
struct FShadowConfig
{
    UINT32 Resolution = 2048;
    float DepthBias = 0.0001f;
    float SlopeScaledDepthBias = 2.0f;
    UINT32 PCFKernelSize = 3;
    bool bEnabled = true;
};
```

1. `BeginShadowPass()`: 섀도우 맵 DSV를 렌더 타겟으로 설정
2. `RenderShadowCasters()`: 광원 시점에서 깊이 전용 렌더링 (깊이 전용 셰이더)
3. `EndShadowPass()`: 이전 렌더 타겟 복원
4. 섀도우 맵은 셰이더 레지스터 `t3`에 바인딩

## 후처리 효과

### HDR 렌더링

`KPostProcessor`는 HDR(16비트 부동소수점) 렌더 타겟으로 렌더링을 시작합니다.

### 블룸 (`KPostProcessor`)

1. **밝은 영역 추출**: HDR 텍스처에서 임계값(기본 1.0) 이상의 밝은 픽셀 추출
2. **가우시안 블러**: Ping-Pong 버퍼로 수평/수직 블러 반복 (기본 5회, 시그마 2.0)
3. **합성**: 원본 HDR + 블러된 블룸 텍스처 합성

```cpp
struct FPostProcessParams
{
    float Exposure = 1.0f;
    float Gamma = 2.2f;
    float BloomThreshold = 1.0f;
    float BloomIntensity = 0.5f;
    float BloomBlurSigma = 2.0f;
    UINT32 BloomBlurIterations = 5;
    bool bBloomEnabled = true;
    bool bTonemappingEnabled = true;
    bool bFXAAEnabled = true;
    bool bColorGradingEnabled = false;
    float Saturation = 1.0f;
    float Contrast = 1.0f;
};
```

### 오토 익스포저 (`KAutoExposure`)

- 히스토그램 기반 평균 휘도 계산
- 다중 해상도 다운샘플링 (128x128 → 1x1)
- 적응 속도 (밝아질 때: 3.0, 어두워질 때: 1.0)
- 휘도 범위: 0.1 ~ 10.0

### 피사계 심도 (`KDepthOfField`)

- 초점 거리 및 초점 범위 설정
- CoC (Circle of Confusion) 계산
- 근거리/원거리 블러 개별 제어
- 최대 블러 샘플: 8

### 모션 블러 (`KMotionBlur`)

- 이전/현재 뷰-프로젝션 행렬로 속도 벡터 계산
- 최대 샘플: 16
- 최소/최대 속도 임계값 설정

### 렌즈 효과 (`KLensEffects`)

- **색수차 (Chromatic Aberration)**: RGB 채널 오프셋
- **비네팅 (Vignette)**: 화면 모서리 어두움 (강도, 부드러움, 둥근 정도)
- **필름 그레인 (Film Grain)**: 노이즈 오버레이 (강도, 속도)

## SSAO (스크린 공간 앰비언트 오클루전)

`KSSAO` (`Engine/Graphics/SSAO/SSAO.h`)

- 반구 내 샘플링 기반
- 커널 크기: 16개 샘플 (헤미스피어 방향으로 랜덤 분포)
- 노이즈 텍스처 (4x4)로 밴딩 아티팩트 감소
- 선택적 바이래터럴 블러 (기본 2회 반복)

```cpp
struct FSSAOParams
{
    float Radius = 0.5f;
    float Bias = 0.025f;
    float Power = 2.0f;
    UINT32 KernelSize = 16;
    bool bBlurEnabled = true;
    UINT32 BlurIterations = 2;
};
```

## SSR (스크린 공간 반사)

`KSSR` (`Engine/Graphics/SSR/SSR.h`)

- Ray-marching 기반 반사 추적
- 최대 거리, 두께, 스텝 수 설정
- 프레넬 효과 기반 반사 강도
- 에지 페이드로 경계 아티팩트 감소
- 원본 이미지와 반사 이미지 합성

```cpp
struct FSSRParams
{
    float MaxDistance = 100.0f;
    float StepCount = 32.0f;
    float Thickness = 0.5f;
    float EdgeFade = 0.1f;
    float FresnelPower = 3.0f;
    float Intensity = 1.0f;
};
```

## SSGI (스크린 공간 글로벌 일루미네이션)

`KSSGI` (`Engine/Graphics/SSGI/SSGI.h`)

- 스크린 공간에서 간접 조명 근사
- 반구 내 샘플링으로 간접 조명 계산
- 선택적 시간적 안정화 (Temporal Blend)
- 양방향 블러
- 노이즈 텍스처로 밴딩 감소

## TAA (시간적 안티앨리어싱)

`KTAA` (`Engine/Graphics/TAA/TAA.h`)

- 이전 프레임과 현재 프레임 혼합
- 모션 벡터 기반 재투영
- 이력 버퍼 (2개 교차 사용)
- 선택적 샤프닝 필터

```cpp
struct FTAAParams
{
    float BlendFactor = 0.1f;
    float FeedbackMin = 0.88f;
    float FeedbackMax = 0.97f;
    bool bSharpenEnabled = true;
    float SharpenStrength = 0.5f;
};
```

## 하늘 렌더링

`KSkySystem` (`Engine/Graphics/Sky/SkySystem.h`)

### 절차적 대기 (Preetham 모델)

Rayleigh 산란과 Mie 산란 기반의 물리 기반 대기 렌더링입니다.

```cpp
struct FAtmosphereParams
{
    XMFLOAT3 SunDirection;
    float SunIntensity = 1.0f;
    XMFLOAT3 RayleighCoefficients = { 5.8e-6f, 13.5e-6f, 33.1e-6f };
    float RayleighHeight = 8000.0f;
    XMFLOAT3 MieCoefficients = { 21.0e-6f, 21.0e-6f, 21.0e-6f };
    float MieHeight = 1200.0f;
    float MieAnisotropy = 0.758f;
    float Exposure = 1.0f;
};
```

- `SetTimeOfDay(float Hour, float Latitude)`: 시간에 따른 태양 위치 자동 계산
- 구형 스카이돔 메시 + 커스텀 스카이 셰이더
- 클라우드 렌더링 지원 (밀도, 커버리지 설정)
- 깊이 버퍼가 없는 렌더 상태 (항상 배경에 표시)

## 볼류메트릭 포그

`KVolumetricFog` (`Engine/Graphics/Volumetric/VolumetricFog.h`)

- Ray-marching 기반 볼류메트릭 포그 시뮬레이션
- 높이 기반 밀도 감쇠
- 단일 산란/소멸 모델
- 광원 산란 지원 (Scattering)
- 장면 색상과 포그 합성

```cpp
struct FVolumetricFogParams
{
    float Density = 0.01f;
    float HeightFalloff = 0.1f;
    float Scattering = 0.5f;
    float StepCount = 64.0f;
    float MaxDistance = 100.0f;
    XMFLOAT3 FogColor = { 0.5f, 0.5f, 0.6f };
};
```

## 물 렌더링

`KWater` / `KWaterComponent` (`Engine/Graphics/Water/Water.h`)

### 파동 시스템

- 최대 4개 파동의 중첩 (Gerstner Wave)
- 각 파동: 진폭, 주파수, 속도, 기울기, 방향

```cpp
struct FWaveParameters
{
    float Amplitude = 1.0f;
    float Frequency = 0.5f;
    float Speed = 1.0f;
    float Steepness = 1.0f;
    XMFLOAT2 Direction = { 1.0f, 0.0f };
};
```

### 물 효과

| 효과 | 설명 |
|------|------|
| 반사 | 평면 반사 렌더링 (반사 텍스처 바인딩) |
| 굴절 | 법선 기반 UV 왜곡 (DuDv 맵) |
| 프레넬 | 시야 각도 기반 반사/굴절 비율 |
| 폼 | 파동 높이 기반 거품 표시 |
| 노멀 맵 | 2개 노멀 맵을 시간에 따라 블렌딩 |

## 지형 렌더링

`KTerrain` / `KTerrainComponent` (`Engine/Graphics/Terrain/Terrain.h`)

### 하이트맵 시스템

`KHeightMap` 클래스에서 지형 높이 데이터를 관리합니다.

```cpp
class KHeightMap
{
public:
    HRESULT LoadFromRawFile(const std::wstring& Path, UINT32 Width, UINT32 Height);
    HRESULT LoadFromImage(const std::wstring& Path);
    HRESULT GeneratePerlinNoise(UINT32 Width, UINT32 Height, float Scale, int32 Octaves, float Persistence);
    HRESULT GenerateFlat(UINT32 Width, UINT32 Height, float FlatHeight = 0.0f);
    float GetHeightInterpolated(float X, float Z) const;
    XMFLOAT3 GetNormal(UINT32 X, UINT32 Z) const;
    void Smooth(int32 Iterations = 1);
};
```

### 스플랫 맵 기반 텍스처링

- 베이스 텍스처 + 스플랫 맵 (R/G/B/A 채널별 레이어)
- 각 레이어: 디퓨즈 텍스처 + 노멀 텍스처 + 텍스처 스케일

### LOD 시스템

```cpp
struct FTerrainConfig
{
    UINT32 Resolution = 128;
    float HeightScale = 50.0f;
    UINT32 LODCount = 4;
    float LODDistances[4] = { 50.0f, 100.0f, 200.0f, 400.0f };
};
```

## GPU 인스턴싱

`KInstancedRenderer` (`Engine/Graphics/Instanced/InstancedRenderer.h`)

- 동일 메시를 여러 월드 행렬로 한 번에 렌더링
- 최대 1024개 인스턴스 (설정 가능)
- 인스턴스 버퍼: `FInstanceData` (XMMATRIX WorldMatrix)

```cpp
class KInstancedRenderer
{
public:
    HRESULT Initialize(ID3D11Device* Device, UINT32 MaxInstances = 1024);
    void BeginFrame();
    void AddInstance(const XMMATRIX& WorldMatrix);
    void Render(ID3D11DeviceContext* Context, KMesh* Mesh, KShaderProgram* Shader,
                const XMMATRIX& View, const XMMATRIX& Projection);
};
```

## 절두체 및 오클루전 컬링

### 절두체 컬링 (`KFrustum`)

- 카메라 뷰-프로젝션 행렬에서 절두체 평면 추출
- 구 및 AABB에 대한 교차 테스트
- `KRenderer::SetFrustumCullingEnabled(bool)`로 토글

### 오클루전 쿼리 (`KOcclusionCuller`)

- GPU 오클루전 쿼리 (D3D11_QUERY_OCCLUSION) 사용
- 쿼리 이름 기반 가시성 확인
- `KRenderer::SetOcclusionCullingEnabled(bool)`로 토글

### 커맨드 버퍼 (`KCommandBuffer`)

- 렌더 커맨드 버퍼링 및 정렬
- 정렬 키: Shader, Texture, Material, Depth, ShaderThenTexture, MaterialThenDepth
- 절두체 컬링과 함께 사용
- 상태 변경 최소화 자동 추적

```cpp
class KCommandBuffer
{
public:
    void AddDrawCommand(std::shared_ptr<KMesh> Mesh, std::shared_ptr<KShaderProgram> Shader,
                        const XMMATRIX& WorldMatrix, std::shared_ptr<KTexture> Texture = nullptr);
    void AddPBRCommand(std::shared_ptr<KMesh> Mesh, KMaterial* Material, const XMMATRIX& WorldMatrix);
    void SetSortKey(ERenderSortKey SortKey);
    void Execute(ID3D11DeviceContext* Context);
    void ExecuteWithFrustumCulling(ID3D11DeviceContext* Context, const KFrustum& Frustum);
};
```

## IBL (이미지 기반 조명)

`KIBLSystem` (`Engine/Graphics/IBL/IBLSystem.h`)

### IBL 텍스처

```cpp
struct FIBLTextures
{
    std::shared_ptr<KTexture> IrradianceMap;    // 조사도 환경맵 (32x32 큐브맵)
    std::shared_ptr<KTexture> PrefilteredMap;   // 프리필터드 환경맵 (128x128, 5 밉레벨)
    std::shared_ptr<KTexture> BRDFLUT;          // BRDF 적분 LUT (2D 텍스처)
};
```

### 생성 파이프라인

1. **HDR 환경맵 로딩**: HDR 포맷 이미지 로드
2. **BRDF LUT 생성**: 2D 텍스처로 프레넬/거칠기에 대한 BRDF 적분 값 저장
3. **조사도 큐브맵 생성**: 환경맵을 저해상도(32x32)로 컨볼루션 → 확산 조명
4. **프리필터드 큐브맵 생성**: 환경맵을 roughness별로 미리 필터링(5 밉레벨) → 스페큘러 조명

### 사용

```cpp
HRESULT Initialize(KGraphicsDevice* InDevice, uint32 InIrradianceSize = 32, uint32 InPrefilterSize = 128);
HRESULT LoadEnvironmentMap(const std::wstring& HDRPath);
HRESULT GenerateIBLTextures();
void Bind(ID3D11DeviceContext* Context, uint32 StartSlot = 10);  // t10~t12
```

## GPU 타이머

`KGPUTimer` (`Engine/Graphics/Performance/GPUTimer.h`)

- D3D11 타이머 쿼리 기반 GPU 시간 측정
- 이름 기반 타이머 생성/조회
- 프레임별 통계 (GPU 시간, 프레임 시간, 커스텀 타이머 결과)

```cpp
class KGPUTimer
{
public:
    HRESULT Initialize(ID3D11Device* Device, UINT32 MaxTimers = 16);
    HRESULT CreateTimer(const std::string& Name);
    void BeginFrame(ID3D11DeviceContext* Context);
    void EndFrame(ID3D11DeviceContext* Context);
    void StartTimer(ID3D11DeviceContext* Context, const std::string& Name);
    void EndTimer(ID3D11DeviceContext* Context, const std::string& Name);
    float GetTimerResult(const std::string& Name) const;
    const FFrameStats& GetFrameStats() const;
};
```

# 코딩 표준

## C++ 명명 규칙

| 유형 | 규칙 | 접두사 | 예시 |
|------|------|--------|------|
| 클래스 | 파스칼 케이스 | `K` | `KEngine`, `KRenderer`, `KActor`, `KGraphicsDevice` |
| 구조체 | 파스칼 케이스 | `F` | `FVertex`, `FDirectionalLight`, `FTransform`, `FBoundingBox` |
| 열거형 | 파스칼 케이스 | `E` | `ERenderPath`, `EKeyCode`, `EColliderType` (enum class 선호) |
| 함수/메서드 | 파스칼 케이스 | - | `Initialize()`, `BeginFrame()`, `CreateCubeMesh()` |
| 지역 변수 | 카멜 케이스 | - | `graphicsDevice`, `currentTime`, `exitCode` |
| 멤버 변수 | 카멜 케이스 | `b` (bool) | `GraphicsDevice`, `bIsRunning`, `bInFrame` |
| 상수 | 파스칼 케이스 | - | `EngineConstants::DefaultFOV`, `Colors::CornflowerBlue` |
| 네임스페이스 | 파스칼 케이스 | - | `KojeomEngine::KDebugUI`, `EKeyCode`, `EMouseButton` |
| 타입 별칭 | 파스칼 케이스 | - | `ActorPtr`, `ComponentPtr`, `ComPtr<T>` |
| 템플릿 파라미터 | 파스칼 케이스 | - | `T`, `FuncType` |
| 함수 파라미터 | 카멜 케이스 | `In` (입력) | `InGraphicsDevice`, `InMesh`, `InName` |

## C++ 타입 별칭

`Engine/Utils/Common.h`에 정의된 커스텀 타입 별칭을 사용합니다. `<cstdint>` 타입을 직접 사용하지 마세요.

```cpp
// 올바른 사용
uint8  data;
int32  count;
uint32 width;

// 잘못된 사용
uint8_t data;
int32_t count;
uint32_t width;
```

## C++ 코드 스타일

### Include 가이드

```cpp
#pragma once  // 항상 #pragma once 사용 (#ifndef 가드 사용 금지)

#include "../Utils/Common.h"  // 첫 번째 include

// 프로젝트 헤더 → 외부 헤더 → 표준 라이브러리
#include "../Graphics/GraphicsDevice.h"
#include <d3d11.h>
#include <memory>
#include <vector>
```

### 헤더 파일 템플릿

```cpp
#pragma once

#include "../Utils/Common.h"
#include "../Utils/Logger.h"

#include <d3d11.h>
#include <memory>
#include <vector>

class KClassName
{
public:
    KClassName() = default;
    ~KClassName() = default;

    KClassName(const KClassName&) = delete;
    KClassName& operator=(const KClassName&) = delete;

    HRESULT Initialize(KGraphicsDevice* InDevice);
    void Cleanup();

    bool IsInitialized() const { return bInitialized; }

private:
    KGraphicsDevice* Device = nullptr;
    ComPtr<ID3D11Buffer> VertexBuffer;
    bool bInitialized = false;
};
```

### 리소스 관리

| 리소스 타입 | 관리 방식 | 예시 |
|------------|----------|------|
| COM 객체 | `ComPtr<T>` | `ComPtr<ID3D11Buffer> VertexBuffer;` |
| 독점 소유 | `std::unique_ptr` | `std::unique_ptr<KGraphicsDevice> GraphicsDevice;` |
| 공유 소유 | `std::shared_ptr` | `std::shared_ptr<KMesh> Mesh;` |
| 관찰용 (비소유) | 원시 포인터 | `KGraphicsDevice* Device = nullptr;` |

- 리소스 소유에는 절대 원시 포인터를 사용하지 마세요.
- 리소스 소유 클래스는 복사 생성자/대입을 삭제하세요.

### 에러 처리

```cpp
// HRESULT 반환 함수
HRESULT hr = Device->CreateBuffer(&desc, &data, &buffer);
if (FAILED(hr))
{
    LOG_ERROR("Buffer creation failed");
    return hr;
}

// 매크로 사용
CHECK_HRESULT(CreateResource());

// 로깅
LOG_INFO("Initialization completed");
LOG_WARNING("Optional feature not available");
LOG_ERROR("Critical failure occurred");

// 안전한 해제
SAFE_RELEASE(p);       // COM 해제
SAFE_DELETE(p);        // 포인터 삭제
SAFE_DELETE_ARRAY(p);  // 배열 삭제
```

### DirectX 11 규칙

- **셰이더 모델**: 5.0 (`vs_5_0`, `ps_5_0`)
- 모든 D3D11 호출의 HRESULT를 확인하세요.
- 리소스 생성 순서: Device → Context → SwapChain → RenderTargetView → DepthStencilView
- 자주 업데이트되는 버퍼: `Map/Unmap`
- 드물게 업데이트되는 버퍼: `UpdateSubresource`
- 상수 버퍼는 16바이트 정렬 필수 (`static_assert(sizeof(...) % 16 == 0)`)
- 셰이더 레지스터: `b0` = 변환, `b1` = 조명, `b2+` = 재질/커스텀; `t0` = 디퓨즈, `t1+` = 기타 텍스처

### Doxygen 스타일 주석

```cpp
/**
 * @brief 함수에 대한 간략한 설명
 * @param InDevice 디바이스 포인터
 * @param InWidth 버퍼 너비
 * @return 성공 시 S_OK
 */
HRESULT CreateBuffer(KGraphicsDevice* InDevice, UINT32 InWidth);
```

## C# 에디터 코드 스타일

### 명명 규칙

| 유형 | 규칙 | 예시 |
|------|------|------|
| 네임스페이스 | 파일 스코프 | `namespace KojeomEditor;` |
| 클래스/속성/메서드 | 파스칼 케이스 | `MainViewModel`, `SceneName`, `LoadScene()` |
| private 필드 | `_camelCase` | `_engine`, `_viewModel`, `_isInitialized` |
| 지역 변수 | 카멜 케이스 | `sceneName`, `actorCount` |

### 패턴

- **MVVM**: `INotifyPropertyChanged` + `RelayCommand`
- **P/Invoke**: `DllImport` + `CallingConvention.Cdecl`
- **리소스 관리**: `IDisposable` 패턴 (네이티브 리소스 해제)
- **Nullable**: Nullable reference types 활성화

### P/Invoke 예시

```csharp
[DllImport("EngineInterop.dll, CallingConvention = CallingConvention.Cdecl)]
private static extern IntPtr Engine_Create();

[DllImport("EngineInterop.dll, CallingConvention = CallingConvention.Cdecl)]
private static extern void Engine_Destroy(IntPtr engine);
```

## Git 커밋 포맷

```
[Category] 간결한 설명

상세 설명이 필요한 경우.

- 변경 목록
```

### 카테고리

| 카테고리 | 설명 |
|---------|------|
| `[Core]` | 코어 엔진 |
| `[Graphics]` | 그래픽스/렌더링 |
| `[Input]` | 입력 시스템 |
| `[Audio]` | 오디오 |
| `[Physics]` | 물리 |
| `[Scene]` | 씬 관리 |
| `[UI]` | UI 시스템 |
| `[Editor]` | 에디터 |
| `[Docs]` | 문서 |
| `[Build]` | 빌드 시스템 |
| `[Refactor]` | 리팩토링 |
| `[Fix]` | 버그 수정 |
| `[Feature]` | 새 기능 |

### 커밋 예시

```
[Graphics] SSAO 파이프라인에 블러 패스 추가

SSAO 결과의 노이즈를 줄이기 위해 양방향 블러 패스를 추가.

- BlurShader 컴파일 (수평/수직)
- 블러 렌더 타겟 생성
- FSSAOParams에 BlurIterations 추가
```

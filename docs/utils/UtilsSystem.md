# Utility System

## Overview

KojeomGameEngine provides core utility headers under `Engine/Utils/`. These are foundational headers included by virtually every engine module.

Source files: `Engine/Utils/Common.h`, `Engine/Utils/Logger.h`, `Engine/Utils/Math.h`

---

## Common.h

**File:** `Engine/Utils/Common.h`

Central include file that must be the first include in every engine source file. Provides:

### Platform and SDK Includes

```cpp
#pragma once
#define NOMINMAX
#include <windows.h>
#include <d3d11_1.h>
#include <directxcolors.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
```

### Standard Library Includes

```cpp
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <iostream>
#include <crtdbg.h>
```

### Custom Type Aliases

All engine code uses these type aliases instead of `<cstdint>` types:

```cpp
using int8   = int8_t;
using int16  = int16_t;
using int32  = int32_t;
using int64  = int64_t;
using uint8  = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
```

### DirectX Helpers

```cpp
using namespace DirectX;

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
```

`ComPtr<T>` is the engine-wide alias for COM smart pointers, used for all D3D11 interfaces.

### Safety Macros

```cpp
#define SAFE_RELEASE(p)    do { if((p)) { (p)->Release(); (p) = nullptr; } } while(0)
#define SAFE_DELETE(p)     do { if((p)) { delete (p); (p) = nullptr; } } while(0)
#define SAFE_DELETE_ARRAY(p) do { if((p)) { delete[] (p); (p) = nullptr; } } while(0)
```

### Error Handling Macros

```cpp
#define CHECK_HRESULT(hr)  do { if(FAILED(hr)) return (hr); } while(0)
#define LOG_ERROR(msg)     KLogger::Error(msg)
#define LOG_INFO(msg)      KLogger::Info(msg)
#define LOG_WARNING(msg)   KLogger::Warning(msg)
```

### Forward Declarations

Forward declares core engine classes to break circular dependencies:

```cpp
class KEngine;
class KGraphicsDevice;
class KRenderer;
class KCamera;
class KLogger;
```

---

## Logger.h

**File:** `Engine/Utils/Logger.h`

Lightweight logging system using `KLogger` static class.

### Log Levels

```cpp
enum class ELevel { Info, Warning, Error };
```

### API

```cpp
static void Info(const std::string& Message);
static void Warning(const std::string& Message);
static void Error(const std::string& Message);
static void HResultError(HRESULT Result, const std::string& Context);
```

### Behavior

- **Debug builds (`_DEBUG` defined):** Outputs formatted messages with `[INFO]`, `[WARN]`, `[ERROR]` prefixes to both `std::cout` and `OutputDebugStringA` (Visual Studio output window)
- **Release builds:** No output (compiled out for zero overhead)
- Log macros in `Common.h` (`LOG_INFO`, `LOG_WARNING`, `LOG_ERROR`) provide a concise interface

---

## Math.h

**File:** `Engine/Utils/Math.h`

Provides mathematical structures used throughout the engine. All structs use DirectXMath types internally.

### FBoundingBox

Axis-aligned bounding box for frustum culling, LOD, and collision:

```cpp
struct FBoundingBox
{
    XMFLOAT3 Min;
    XMFLOAT3 Max;

    void Expand(const XMFLOAT3& Point);
    void Expand(const FBoundingBox& Other);
    XMFLOAT3 GetCenter() const;
    XMFLOAT3 GetExtent() const;
    float GetRadius() const;
    bool IsValid() const;
    void Reset();
};
```

Default construction initializes to an inverted box (`Min=FLT_MAX`, `Max=-FLT_MAX`) for incremental expansion.

### FBoundingSphere

Bounding sphere derived from `FBoundingBox`:

```cpp
struct FBoundingSphere
{
    XMFLOAT3 Center;
    float Radius;

    void FromBoundingBox(const FBoundingBox& Box);
};
```

### FTransform

Position, rotation (quaternion), and scale transform used by `KActor` and components:

```cpp
struct FTransform
{
    XMFLOAT3 Position;
    XMFLOAT4 Rotation;
    XMFLOAT3 Scale;

    XMMATRIX ToMatrix() const;
    static FTransform Identity();
    void Reset();
};
```

`ToMatrix()` computes the world matrix as Scale * Rotation * Translation.

### FMaterialSlot

Maps a mesh section to a material index, used by mesh and model systems:

```cpp
struct FMaterialSlot
{
    std::string Name;
    uint32 StartIndex;
    uint32 IndexCount;
    int32 MaterialIndex;
};
```

---

## Include Order Convention

Every engine source file must follow this include order:

1. `#include "../Utils/Common.h"` (always first)
2. Project headers
3. External headers (D3D11, DirectXMath, etc.)
4. Standard library headers

`Common.h` pulls in Windows SDK, DirectX, STL, and all utility definitions, so subsequent includes can freely use `ComPtr`, `uint32`, `LOG_INFO`, etc.

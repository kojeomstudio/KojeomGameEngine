# Coding Guidelines for KojeomGameEngine

이 문서는 코딩 컨벤션, 안정성, 유지보수, 보안에 대한 실질적 가이드라인을 정의합니다.
아키텍처 구조 및 구현 가이드는 `work/architecture/`를 참조하십시오.

## C++ Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Classes | PascalCase with `K` prefix | `KEngine`, `KRenderer`, `KActor` |
| Structs | PascalCase with `F` prefix | `FVertex`, `FDirectionalLight`, `FTransform` |
| Enums | `E` prefix (prefer `enum class`) | `ERenderPath`, `EKeyCode`, `EColliderType` |
| Functions/Methods | PascalCase | `Initialize()`, `BeginFrame()` |
| Local variables | camelCase | `graphicsDevice`, `currentTime` |
| Member variables | camelCase, `b` prefix for booleans | `GraphicsDevice`, `bIsRunning` |
| Constants | ALL_CAPS inside namespace | `EngineConstants::DEFAULT_FOV` |
| Parameters | camelCase, `In` prefix for inputs | `InGraphicsDevice`, `InMesh` |
| Type aliases | PascalCase | `ActorPtr`, `ComPtr<T>` |

## C# Editor Conventions

- File-scoped namespaces: `namespace KojeomEditor;`
- Classes/properties/methods: PascalCase
- Private fields: `_camelCase` (`_engine`, `_viewModel`)
- Nullable reference types enabled
- MVVM: `INotifyPropertyChanged` + `RelayCommand`
- P/Invoke: `DllImport` with `CallingConvention.Cdecl`
- Native resources: `IDisposable`

## File Structure

- `#pragma once` (no include guards)
- Include order: `#include "../Utils/Common.h"` -> project headers -> external headers -> std library
- Paired files: `ClassName.h` / `ClassName.cpp`
- Use `uint8`, `uint32`, `int32` etc. from `Engine/Utils/Common.h` (NOT `<cstdint>`)

## Resource Management

- `ComPtr<T>` for all COM objects (D3D11 interfaces)
- `std::unique_ptr` for exclusive ownership
- `std::shared_ptr` for shared resources (meshes, textures, actors)
- Raw pointers only for non-owning observation
- Delete copy constructor/assignment for resource-owning classes
- `static_assert` for 16-byte alignment on constant buffer structs

## Error Handling

- Return `HRESULT`; check with `FAILED(hr)`
- `CHECK_HRESULT(hr)` for early-return on failure
- `LOG_INFO()`, `LOG_WARNING()`, `LOG_ERROR()` macros
- Validate pointers at function entry
- `SAFE_RELEASE(p)`, `SAFE_DELETE(p)`, `SAFE_DELETE_ARRAY(p)`

## DirectX 11 Rules

- Shader Model 5.0 (`vs_5_0`, `ps_5_0`)
- Resource creation order: Device -> Context -> SwapChain -> RTV -> DSV
- `Map/Unmap` for frequent updates, `UpdateSubresource` for rare updates
- NO `.hlsl` files; all shaders are inline C++ string literals via `KShader::CompileFromString()`
- Shader registers: `b0` transform, `b1` light, `b2+` material; `t0` diffuse, `t1+` other textures

## Interop Rules

- `EngineAPI.h`/`EngineAPI.cpp`: `extern "C"` flat C API
- C# `DllImport` in `Editor/KojeomEditor/Services/EngineInterop.cs`
- Adding C API function requires adding matching C# DllImport
- String returns use `thread_local std::string` buffers

## Stability

- Build after every change: `msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64`
- Do not ignore compiler warnings
- Do not break existing API without updating all call sites (headers, interop, editor)
- Keep changes focused: one feature/fix per commit
- Always read existing code before making changes

## Security

- Validate all file paths against traversal attacks (no `..`, no absolute paths from user input)
- Bounds check all buffer operations (vertex, index, constant, texture)
- No hardcoded secrets, API keys, or credentials in source code
- No use-after-free; verify resource lifecycle in shutdown path
- Check `HRESULT` on all D3D11 calls; never assume success

## Prohibited Actions

- Do not modify `Engine/third_party/`
- Do not commit debug/test code to main branch
- Do not add new dependencies without explicit approval
- Do not create `.hlsl` shader files
- Do not use raw pointers for owning resources
- Do not add C API functions without matching C# DllImport

## Git Commit Format

```
[Category] Brief description

- Change detail
```

Categories: `[Core]`, `[Graphics]`, `[Input]`, `[Audio]`, `[Physics]`, `[Scene]`, `[UI]`, `[Editor]`, `[Docs]`, `[Build]`, `[Refactor]`, `[Fix]`, `[Feature]`, `[Security]`

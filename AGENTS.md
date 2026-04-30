# AGENTS.md — KojeomGameEngine

Instructions for AI coding agents working in this repository.

## Project Overview

C++17 / DirectX 11 game engine with a WPF-based editor (C# / .NET 8.0). Supports Forward and Deferred rendering, PBR, skeletal animation, physics, audio, and a canvas UI system.

- **Solution:** `KojeomEngine.sln` (Visual Studio 2022, MSVC v143, C++17, x64 only)
- **Core Engine:** `Engine/` (static library, .lib)
- **Editor Bridge:** `Editor/EngineInterop/` (C++ DLL exposing flat C API for P/Invoke)
- **Editor UI:** `Editor/KojeomEditor/` (C# WPF, .NET 8.0, x64)
- **Samples:** `samples/` (16 sample projects)

## Build Commands

```bash
# Build entire solution (from repo root)
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64
msbuild KojeomEngine.sln /p:Configuration=Release /p:Platform=x64

# Build a single project
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64 /t:Engine

# Build the C# editor
dotnet build Editor/KojeomEditor/KojeomEditor.csproj
```

## Testing

There is **no automated test framework** in this project. Validation is done through:
- 16 sample projects under `samples/`
- `LOG_INFO`, `LOG_WARNING`, `LOG_ERROR` macros for debug output
- `_CrtSetDbgFlag` for debug memory leak detection

## Linting / Formatting

No linter or formatter is configured. Code follows Visual Studio defaults (4-space indentation). Ensure new code matches the style of existing files in the same module.

## Coding Guidelines

코딩 컨벤션, 안정성, 유지보수, 보안에 대한 상세 가이드라인은 `docs/rules/AIAgentRules.md`를 참조하십시오.

## Architecture

엔진 아키텍처, 모듈 구조, 구현 가이드는 `work/architecture/`를 참조하십시오.

## Feature Documentation

기능 분석 문서는 `docs/feature/`에 있습니다.

## Git Commit Format

```
[Category] Brief description

- List of changes
```

Categories: `[Core]`, `[Graphics]`, `[Input]`, `[Audio]`, `[Physics]`, `[Scene]`, `[UI]`, `[Editor]`, `[Docs]`, `[Build]`, `[Refactor]`, `[Fix]`, `[Feature]`, `[Security]`

## Critical Rules

1. Read existing code before making changes to understand established patterns
2. When modifying engine APIs, also update `Editor/EngineInterop/EngineAPI.h`, `EngineAPI.cpp`, and C# editor bindings in `Editor/KojeomEditor/Services/`
3. Never use raw pointers for owning resources
4. Never modify third-party library code under `Engine/third_party/`
5. Never commit debug/test code to main branch
6. Never add new dependencies without explicit approval
7. Never break existing APIs without updating all call sites

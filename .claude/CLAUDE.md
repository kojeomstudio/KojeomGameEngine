# Claude AI Agent Rules for KojeomGameEngine

C++17 / DirectX 11 기반 게임 엔진 프로젝트입니다. WPF 에디터(C# / .NET 8.0)를 포함합니다.

## Guidelines

코딩 컨벤션, 안정성, 유지보수, 보안 가이드라인은 `docs/rules/AIAgentRules.md`를 참조하십시오.

## Architecture

엔진 아키텍처 및 구현 가이드는 `work/architecture/`를 참조하십시오. (우선순위 최상위)

## Feature Docs

기능 분석 문서는 `docs/feature/`에 있습니다.

## Build

```bash
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64
dotnet build Editor/KojeomEditor/KojeomEditor.csproj
```

변경 후 반드시 빌드 테스트를 수행하십시오.

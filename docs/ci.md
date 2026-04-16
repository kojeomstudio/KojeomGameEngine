# CI/CD Pipeline

## Overview

The project uses GitHub Actions for continuous integration. The pipeline is defined in `.github/workflows/build.yml`.

## Triggers

- **Push** to `main` branch
- **Pull requests** targeting `main` branch

## Jobs

### 1. Build Engine + Interop (`build-engine`)

- **Runner:** `windows-latest`
- **Strategy:** Debug + Release configurations
- **Steps:**
  1. Checkout repository
  2. Setup MSBuild
  3. Build Engine and EngineInterop targets only (`/t:Engine;EngineInterop`)
  4. Upload artifacts (Engine.lib, EngineInterop.dll)

### 2. Build Samples (`build-samples`)

- **Runner:** `windows-latest`
- **Depends on:** `build-engine` (downloads artifacts)
- **Strategy:** 16 samples x 2 configurations = 32 combinations
- **Samples:** BasicRendering, Lighting, PBR, PostProcessing, DebugRendering, Terrain, Water, Sky, Particles, SkeletalMesh, AnimationStateMachine, Gameplay, Physics, UI, Layout, LOD
- **Build method:** Each sample is built directly via its `.vcxproj` file (not through the solution), so downloaded artifacts are used instead of rebuilding the engine

### 3. Build C# Editor (`build-csharp`)

- **Runner:** `windows-latest`
- **Depends on:** `build-engine` (downloads artifacts)
- **Strategy:** Debug + Release configurations
- **Steps:** .NET 8.0 SDK setup, restore, build

## Security

- **Permissions:** `contents: read` only (least privilege)
- **Action pinning:** All GitHub Actions are pinned to full commit SHAs (no mutable tags)
- **No untrusted inputs:** Matrix values are hardcoded literals

## Adding New Samples

1. Create the sample project (`.vcxproj`) under `samples/`
2. Add the sample project to `KojeomEngine.sln`
3. Add the sample name and vcxproj path to the `build-samples` job matrix `include` section in `.github/workflows/build.yml`
4. Verify the sample builds locally: `msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64 /t:YourSampleName`

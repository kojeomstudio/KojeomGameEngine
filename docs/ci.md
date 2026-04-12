# CI/CD Pipeline

## Overview

The project uses GitHub Actions for continuous integration. The pipeline is defined in `.github/workflows/build.yml`.

## Triggers

- **Push** to `main` branch
- **Pull requests** targeting `main` branch

## Jobs

### 1. Build C++ (`build-cpp`)

- **Runner:** `windows-latest`
- **Strategy:** Debug + Release configurations
- **Steps:**
  1. Checkout repository
  2. Setup MSBuild
  3. Build entire solution (`KojeomEngine.sln`)
  4. Upload artifacts (Engine.lib, EngineInterop.dll, .pdb files)

### 2. Build Samples (`build-samples`)

- **Runner:** `windows-latest`
- **Depends on:** `build-cpp` (downloads artifacts)
- **Strategy:** 16 samples x 2 configurations = 32 combinations
- **Samples:** BasicRendering, Lighting, PBR, PostProcessing, DebugRendering, Terrain, Water, Sky, Particles, SkeletalMesh, AnimationStateMachine, Gameplay, Physics, UI, Layout, LOD

### 3. Build C# Editor (`build-csharp`)

- **Runner:** `windows-latest`
- **Depends on:** `build-cpp` (downloads artifacts)
- **Strategy:** Debug + Release configurations
- **Steps:** .NET 8.0 SDK setup, restore, build

## Security

- **Permissions:** `contents: read` only (least privilege)
- **Action pinning:** All GitHub Actions are pinned to full commit SHAs (no mutable tags)
- **No untrusted inputs:** Matrix values are hardcoded literals

## Adding New Samples

1. Add the sample project to `KojeomEngine.sln`
2. Add the sample name to the `build-samples` job matrix in `.github/workflows/build.yml`
3. Verify the sample builds locally: `msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64 /t:YourSampleName`

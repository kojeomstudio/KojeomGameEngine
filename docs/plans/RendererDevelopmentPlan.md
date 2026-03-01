# Renderer Development Plan

## Document Information

- **Created**: 2026-03-01
- **Author**: AI Agent
- **Status**: Phase 3 Completed
- **Base Commit**: 2e13736
- **Last Updated**: 2026-03-02

## Overview

This document outlines the development plan for enhancing the DirectX 11 renderer in KojeomGameEngine. The plan is divided into phases with specific tasks and commit tracking.

## Current State Analysis

### Existing Components

| Component | Status | Description |
|-----------|--------|-------------|
| KGraphicsDevice | ✅ Complete | Device, Context, SwapChain management |
| KRenderer | ✅ Complete | Basic rendering pipeline |
| KCamera | ✅ Complete | View/Projection matrix management |
| KShaderProgram | ✅ Complete | Basic and Phong shaders with multi-light support |
| KMesh | ✅ Complete | Basic primitive shapes |
| KTexture | ✅ Complete | Texture loading and sampling |
| Lighting | ✅ Enhanced | Directional + Point + Spot lights |

### Current Capabilities

1. Forward rendering with multiple light types
2. Up to 8 point lights and 4 spot lights
3. Light volume visualization for debugging
4. Distance-based light attenuation

## Development Phases

### Phase 1: Enhanced Lighting System

**Status**: ✅ Completed  
**Target Completion**: 2026-03-02  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| Point light implementation | ✅ | | FPointLight struct, shader integration |
| Spot light implementation | ✅ | | FSpotLight struct, cone attenuation |
| Multiple light support | ✅ | | FMultipleLightBuffer (8 point, 4 spot) |
| Light volume visualization | ✅ | | DebugDrawLightVolumes, wireframe spheres |
| Light uniform optimization | ✅ | | Distance-based attenuation in shader |

#### Implementation Details

**Modified Files:**
- `Engine/Graphics/Light.h` - Added FPointLight, FSpotLight, FMultipleLightBuffer structs
- `Engine/Graphics/Renderer.h` - Added light management methods and debug drawing
- `Engine/Graphics/Renderer.cpp` - Implemented light buffer update and debug visualization
- `Engine/Graphics/Shader.cpp` - Updated Phong shader to support multiple lights
- `Engine/Utils/Common.h` - Added NOMINMAX for std::min/max compatibility

**New Features:**
- `AddPointLight()`, `RemovePointLight()`, `ClearPointLights()` methods
- `AddSpotLight()`, `RemoveSpotLight()`, `ClearSpotLights()` methods
- `DebugDrawLightVolumes()` for visualizing light influence areas
- Wireframe rasterizer state for debug rendering

#### Technical Details

- Maximum 8 point lights, 4 spot lights (configurable via MAX_POINT_LIGHTS, MAX_SPOT_LIGHTS)
- Constant buffer b1 contains all light data (704 bytes)
- Per-pixel light calculation with distance attenuation
- Spot light cone angle falloff using inner/outer cone angles

---

### Phase 2: Shadow Mapping

**Status**: ✅ Completed  
**Target Completion**: 2026-03-02  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| Shadow map render target | ✅ | | KShadowMap class with depth texture |
| Shadow shader | ✅ | | Depth-only vertex/pixel shader |
| Shadow pass renderer | ✅ | | KShadowRenderer with scene bounds |
| PCF filtering | ✅ | | 3x3 PCF in pixel shader |
| Shadow constant buffer | ✅ | | FShadowDataBuffer (b2) |
| Shadow bias configuration | ✅ | | Configurable depth and slope bias |

#### Implementation Details

**New Files:**
- `Engine/Graphics/Shadow/ShadowMap.h/cpp` - Shadow map depth texture management
- `Engine/Graphics/Shadow/ShadowRenderer.h/cpp` - Shadow pass coordination

**Modified Files:**
- `Engine/Graphics/Renderer.h/cpp` - Shadow system integration
- `Engine/Graphics/Shader.h/cpp` - Shadow shader and PhongShadowShader
- `Engine/Graphics/Light.h` - FShadowDataBuffer structure

**Features:**
- 2048x2048 shadow map resolution (configurable)
- PCF (Percentage Closer Filtering) with configurable kernel size
- Automatic shadow scene bounds calculation
- Shadow sampler state with border color for out-of-bounds

---

### Phase 3: Deferred Rendering

**Status**: ✅ Completed  
**Target Completion**: 2026-03-02  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| G-Buffer design | ✅ | | KGBuffer class with 3 render targets |
| G-Buffer render targets | ✅ | | RT0: Albedo+Metallic, RT1: Normal+Roughness, RT2: Position+AO |
| Geometry pass | ✅ | | KDeferredRenderer::BeginGeometryPass/RenderGeometry/EndGeometryPass |
| Lighting pass | ✅ | | Full-screen quad deferred lighting shader |
| Forward+ transparency | 🔲 | | Pending |

#### Implementation Details

**New Files:**
- `Engine/Graphics/Deferred/GBuffer.h/cpp` - G-Buffer management with multiple render targets
- `Engine/Graphics/Deferred/DeferredRenderer.h/cpp` - Deferred rendering pipeline

**Modified Files:**
- `Engine/Graphics/Renderer.h/cpp` - Added deferred rendering support and render path switching

**Features:**
- KGBuffer class with 3 render targets (AlbedoMetallic, NormalRoughness, PositionAO)
- Separate depth texture with shader resource view access
- Geometry pass shader writing to multiple render targets
- Deferred lighting pass with full-screen quad
- Support for directional, point, and spot lights in deferred pass
- Render path switching between Forward and Deferred modes
- ERenderPath enum for render path selection

#### Technical Details

**G-Buffer Layout**:
- RT0: Albedo (RGB) + Metallic (A) - R8G8B8A8_UNORM
- RT1: Normal (RGB) + Roughness (A) - R8G8B8A8_UNORM
- RT2: Position (RGB) + AO (A) - R16G16B16A16_FLOAT
- Depth: 24-bit depth + 8-bit stencil (R24G8_TYPELESS)

---

### Phase 4: PBR Materials

**Status**: ✅ Completed  
**Target Completion**: 2026-03-02  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| PBR shader implementation | ✅ | | Cook-Torrance BRDF with GGX distribution |
| Material system | ✅ | | FPBRMaterialParams struct, KMaterial class |
| IBL support | ✅ | | KIBLSystem with BRDF LUT generation |
| Material editor UI | 🔲 | | Pending for C# editor |

#### Implementation Details

**New Files:**
- `Engine/Graphics/Material.h/cpp` - PBR material system with FPBRMaterialParams
- `Engine/Graphics/IBL/IBLSystem.h/cpp` - Image-based lighting support

**Modified Files:**
- `Engine/Graphics/Shader.h/cpp` - Added CreatePBRShader method
- `Engine/Graphics/Renderer.h/cpp` - Added RenderMeshPBR, PBRShader, MaterialSamplerState
- `Engine/Graphics/Texture.h/cpp` - Added SetFromExisting method for IBL
- `Engine/Core/Engine.h` - Fixed NOMINMAX for std::min/max
- `Engine/Engine.vcxproj` - Added new files

**Features:**
- Metalness/Roughness workflow
- Cook-Torrance microfacet BRDF
- GGX distribution function
- Fresnel-Schlick approximation
- Geometry function (Smith's method)
- Support for material textures (Albedo, Normal, Metallic, Roughness, AO, Emissive, Height)
- BRDF LUT generation for IBL
- HDR tonemapping (Reinhard) in shader
- Gamma correction

#### Technical Details

- **Material Buffer (b4)**: FPBRMaterialParams with Albedo, Metallic, Roughness, AO, Emissive properties
- **Texture Slots**:
  - t0: Albedo map
  - t1: Normal map  
  - t2: Metallic map
  - t3: Roughness map
  - t4: AO map
  - t5: Emissive map
  - t6: Height map
- **IBL System**: BRDF LUT (512x512 R16G16_FLOAT), Irradiance/Prefiltered cubemap support
- **Pre-defined Materials**: Default, Metal, Plastic, Rubber, Gold, Silver, Copper

#### PBR Shader Features

```hlsl
// Core PBR functions implemented:
float DistributionGGX(float3 N, float3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(float3 N, float3 V, float3 L, float roughness);
float3 fresnelSchlick(float cosTheta, float3 F0);
float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness);

// Normal mapping with tangent space transformation
float3 GetNormalFromMap(PS_INPUT input, float3 normal);

// Integrated lighting calculation
float3 CalculatePBRLighting(float3 N, float3 V, float3 albedo, 
                            float metallic, float roughness, float ao, float shadow);
```

---

### Phase 5: Post-Processing

**Status**: 🔲 Not Started  
**Target Completion**: TBD  
**Commit Hash**: (Fill when starting)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| Post-process framework | 🔲 | | |
| HDR tonemapping | 🔲 | | |
| Bloom effect | 🔲 | | |
| FXAA/TAA | 🔲 | | |
| Color grading | 🔲 | | |
| Screen-space effects | 🔲 | | |

---

### Phase 6: Performance Optimization

**Status**: 🔲 Not Started  
**Target Completion**: TBD  
**Commit Hash**: (Fill when starting)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| Frustum culling | 🔲 | | |
| Occlusion culling | 🔲 | | |
| Instanced rendering | 🔲 | | |
| GPU query timers | 🔲 | | |
| Command buffer optimization | 🔲 | | |

---

## Progress Tracking

### Commit History Template

When making commits related to this plan, update the relevant task with the commit hash:

```
| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| Point light implementation | ✅ | abc1234 | Completed 2026-03-02 |
```

### Status Legend

- 🔲 Not Started
- 🔄 In Progress
- ✅ Completed
- ⏸️ Paused
- ❌ Cancelled

## Architecture Decisions

### Render Pipeline Selection

**Decision**: Implement both Forward+ and Deferred rendering

**Rationale**:
- Deferred: Best for many lights, complex scenes
- Forward+: Better for transparency, MSAA support
- Allow runtime switching based on scene requirements

### Resource Management

**Decision**: Use resource pools and lazy initialization

**Rationale**:
- Reduce loading stutter
- Memory efficiency
- Easy resource hot-reloading

## Dependencies

### External Libraries (Planned)

| Library | Purpose | Status |
|---------|---------|--------|
| DirectXTK | Texture loading, utilities | 🔲 |
| ImGUI | Debug UI | 🔲 |
| Assimp | Model loading | 🔲 |

### Internal Dependencies

- ~~All phases depend on Phase 1 (Enhanced Lighting)~~ ✅ Phase 1 Complete
- ~~Phase 4 (PBR) depends on Phase 3 (Deferred)~~ ✅ Phase 4 Complete
- Phase 5 (Post-Processing) can use PBR output
- Phase 6 (Optimization) can be done in parallel with other phases

## Risk Assessment

| Risk | Impact | Mitigation |
|------|--------|------------|
| Performance regression | High | Profile after each change |
| Memory overhead | Medium | Implement resource budgeting |
| API breaking changes | Medium | Maintain backward compatibility |
| Shader complexity | Low | Use shader includes and modularity |

## Testing Strategy

1. **Unit Tests**: Test individual components in isolation
2. **Visual Tests**: Compare screenshots before/after changes
3. **Performance Tests**: Benchmark with standard scenes
4. **Memory Tests**: Check for leaks with debug layer

## Documentation Requirements

Each completed task should include:

1. Code documentation (Doxygen comments)
2. API usage examples
3. Update to renderer documentation
4. Changelog entry

## Notes

- This is a living document and should be updated as development progresses
- Commit hashes should be filled in as work is completed
- Phase priorities may change based on project needs
- Consider creating separate branches for each phase

---

## Appendix: File Structure

```
docs/
├── renderer/
│   └── DirectX11Renderer.md    # Technical renderer documentation
├── rules/
│   └── AIAgentRules.md         # AI agent guidelines
└── plans/
    └── RendererDevelopmentPlan.md  # This document

Engine/Graphics/
├── Renderer.h/cpp              # Main renderer
├── GraphicsDevice.h/cpp        # Device management
├── Camera.h/cpp                # Camera system
├── Shader.h/cpp                # Shader management
├── Mesh.h/cpp                  # Mesh geometry
├── Texture.h/cpp               # Texture management
├── Light.h                     # Light structures
├── Material.h/cpp              # PBR material system (Implemented)
├── Shadow/                     # Shadow mapping system
│   ├── ShadowMap.h/cpp
│   └── ShadowRenderer.h/cpp
├── Deferred/                   # Deferred rendering (Implemented)
│   ├── GBuffer.h/cpp
│   └── DeferredRenderer.h/cpp
├── IBL/                        # Image-based lighting (Implemented)
│   └── IBLSystem.h/cpp
└── PostProcess/                # (Planned) Post-processing
    ├── PostProcessor.h/cpp
    ├── BloomEffect.h/cpp
    └── Tonemapper.h/cpp
```

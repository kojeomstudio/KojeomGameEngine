# Renderer Development Plan

## Document Information

- **Created**: 2026-03-01
- **Author**: AI Agent
- **Status**: Phase 1 Completed
- **Base Commit**: dd9ad0f
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

**Status**: 🔲 Not Started  
**Target Completion**: TBD  
**Commit Hash**: (Fill when starting)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| Directional light shadows | 🔲 | | |
| Shadow map rendering pass | 🔲 | | |
| PCF filtering | 🔲 | | |
| Cascade shadow maps | 🔲 | | |
| Shadow bias configuration | 🔲 | | |

#### Technical Details

- Shadow map resolution: 2048x2048 (configurable)
- Support for PCF 2x2, 3x3, 5x5 filtering
- 3-4 cascades for directional shadows

---

### Phase 3: Deferred Rendering

**Status**: 🔲 Not Started  
**Target Completion**: TBD  
**Commit Hash**: (Fill when starting)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| G-Buffer design | 🔲 | | |
| G-Buffer render targets | 🔲 | | |
| Geometry pass | 🔲 | | |
| Lighting pass | 🔲 | | |
| Forward+ transparency | 🔲 | | |

#### Technical Details

**G-Buffer Layout**:
- RT0: Albedo (RGB) + Metallic (A)
- RT1: Normal (RGB) + Roughness (A)
- RT2: Position (RGB) + AO (A)
- Depth: 24-bit depth + 8-bit stencil

---

### Phase 4: PBR Materials

**Status**: 🔲 Not Started  
**Target Completion**: TBD  
**Commit Hash**: (Fill when starting)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| PBR shader implementation | 🔲 | | |
| Material system | 🔲 | | |
| IBL support | 🔲 | | |
| Material editor UI | 🔲 | | |

#### Technical Details

- Metalness/Roughness workflow
- Image-based lighting with pre-filtered cubemaps
- Material parameter serialization

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
- Phase 4 (PBR) depends on Phase 3 (Deferred)
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
├── PostProcess/                # (Planned) Post-processing
│   ├── PostProcessor.h/cpp
│   ├── BloomEffect.h/cpp
│   └── Tonemapper.h/cpp
└── Deferred/                   # (Planned) Deferred rendering
    ├── GBuffer.h/cpp
    └── DeferredLighting.h/cpp
```

# Renderer Development Plan

## Document Information

- **Created**: 2026-03-01
- **Author**: AI Agent
- **Status**: Phase 15 Completed
- **Base Commit**: fe09afb
- **Last Updated**: 2026-03-07

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
| Forward+ transparency | ✅ | | FForwardTransparentRenderData, RenderForwardTransparentPass |

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

**Status**: ✅ Completed  
**Target Completion**: 2026-03-02  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| Post-process framework | ✅ | | KPostProcessor class with HDR render target |
| HDR tonemapping | ✅ | | ACES Filmic tonemapper with exposure control |
| Bloom effect | ✅ | | Bright extraction, Gaussian blur, combination |
| FXAA | ✅ | | Fast approximate anti-aliasing |
| Color grading | ✅ | | 3D LUT (32x32x32), saturation, contrast, color filter |
| Screen-space effects | 🔲 | | Pending |

#### Implementation Details

**New Files:**
- `Engine/Graphics/PostProcess/PostProcessor.h/cpp` - Post-processing framework with HDR pipeline

**Modified Files:**
- `Engine/Graphics/Renderer.h/cpp` - Added PostProcessor integration
- `Engine/Graphics/Mesh.h/cpp` - Added InitializeFromBuffer method
- `Engine/Engine.vcxproj` - Added PostProcessor files

**Features:**
- HDR render target (R16G16B16A16_FLOAT)
- Bright extraction shader for bloom
- Dual-pass Gaussian blur
- ACES Filmic tonemapper
- FXAA anti-aliasing
- Configurable parameters (exposure, gamma, bloom threshold/intensity)

#### Technical Details

**Render Targets:**
- HDR Texture: R16G16B16A16_FLOAT
- Bloom Extract: Half resolution, R16G16B16A16_FLOAT
- Bloom Blur: Ping-pong buffers for separable blur
- Intermediate: Full resolution, R16G16B16A16_FLOAT

**PostProcess Parameters (b0):**
- Exposure: HDR exposure multiplier
- Gamma: Output gamma correction
- BloomThreshold: Bright extraction threshold
- BloomIntensity: Bloom blend factor
- BloomBlurIterations: Blur pass count
- FXAA settings: SpanMax, ReduceMin, ReduceMul

**Shader Pipeline:**
1. BeginHDRPass - Render to HDR target
2. ExtractBrightAreas - Threshold extraction
3. BlurBloomTexture - Separable Gaussian blur
4. ApplyBloom - Combine scene + bloom
5. ApplyTonemapping - ACES + gamma
6. ApplyFXAA - Anti-aliasing pass

---

### Phase 6: Performance Optimization

**Status**: ✅ Completed  
**Target Completion**: 2026-03-02  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| Frustum culling | ✅ | | KFrustum class with plane extraction |
| Occlusion culling | ✅ | | KOcclusionQuery and KOcclusionCuller with hardware queries |
| Instanced rendering | ✅ | | KInstancedRenderer for batch rendering |
| GPU query timers | ✅ | | KGPUTimer with timestamp queries |
| Command buffer optimization | ✅ | | KCommandBuffer for batch rendering with state caching |

#### Implementation Details

**New Files:**
- `Engine/Graphics/Culling/Frustum.h/cpp` - Frustum culling with plane extraction
- `Engine/Graphics/Culling/OcclusionQuery.h/cpp` - Hardware occlusion queries
- `Engine/Graphics/CommandBuffer/CommandBuffer.h/cpp` - Command buffer for batch rendering
- `Engine/Graphics/Instanced/InstancedRenderer.h/cpp` - Instanced rendering support
- `Engine/Graphics/Performance/GPUTimer.h/cpp` - GPU performance queries

**Modified Files:**
- `Engine/Graphics/Renderer.h/cpp` - Added frustum culling, occlusion culling, command buffer, instanced renderer, GPU timer integration
- `Engine/Engine.vcxproj` - Added new files

**Features:**
- KFrustum: 6-plane frustum extraction from view-projection matrix
- KOcclusionQuery: Hardware occlusion queries with D3D11_QUERY_OCCLUSION
- KOcclusionCuller: Occlusion culling system with named queries
- Sphere and box intersection tests
- KInstancedRenderer: Batch rendering with instance buffer
- KCommandBuffer: Render command batching with sort keys (None, Shader, Texture, Material, Depth, ShaderThenTexture, MaterialThenDepth)
- KGPUTimer: GPU timestamp queries for performance profiling
- Frame stats tracking

#### Technical Details

**Frustum Planes:**
- Extract 6 planes (Left, Right, Top, Bottom, Near, Far) from VP matrix
- Normalized plane equations for accurate distance tests
- 8 corner points computation for debug visualization

**Instanced Rendering:**
- Dynamic instance buffer (default 1024 instances)
- FInstanceData struct with world matrix
- DrawInstanced/DrawIndexedInstanced support

**GPU Timer:**
- D3D11_QUERY_TIMESTAMP and D3D11_QUERY_TIMESTAMP_DISJOINT
- Multiple named timers per frame
- Frame stats with per-timer results

---

---

### Phase 7: Screen Space Ambient Occlusion

**Status**: ✅ Completed  
**Target Completion**: 2026-03-02  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| SSAO kernel generation | ✅ | | 64 samples with hemisphere distribution |
| Noise texture generation | ✅ | | 4x4 random rotation texture |
| SSAO shader | ✅ | | SSAO computation with bias and radius |
| Blur shader | ✅ | | Separable Gaussian blur for SSAO |
| Renderer integration | ✅ | | KSSAO class with Deferred rendering support |

#### Implementation Details

**New Files:**
- `Engine/Graphics/SSAO/SSAO.h/cpp` - SSAO system with kernel and noise generation

**Modified Files:**
- `Engine/Graphics/Renderer.h/cpp` - Added SSAO integration
- `Engine/Engine.vcxproj` - Added SSAO files

**Features:**
- 64-sample SSAO kernel with hemisphere distribution
- 4x4 random rotation noise texture
- Configurable radius, bias, and power parameters
- Separable Gaussian blur for smoothing
- Integration with Deferred rendering pipeline
- Real-time toggle and parameter adjustment

#### Technical Details

**SSAO Parameters:**
- Radius: 0.5f (sample search radius)
- Bias: 0.025f (depth bias to prevent self-shadowing)
- Power: 2.0f (AO contrast power)
- KernelSize: 16 (number of samples to use)
- BlurIterations: 2 (blur pass count)

**Render Targets:**
- SSAO Texture: R16_FLOAT format
- Blur Output: R16_FLOAT format (ping-pong)

**Shader Pipeline:**
1. Compute SSAO using Normal, Position, Depth from G-Buffer
2. Apply separable Gaussian blur (horizontal + vertical)
3. Output to lighting pass or apply to final image

---

### Phase 8: Screen Space Reflections

**Status**: ✅ Completed  
**Target Completion**: 2026-03-02  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| SSR ray marching | ✅ | | Ray marching with depth buffer |
| Fresnel effect | ✅ | | Fresnel-Schlick approximation |
| Edge fade | ✅ | | Screen edge fade factor |
| Renderer integration | ✅ | | KSSR class with Deferred rendering support |

#### Implementation Details

**New Files:**
- `Engine/Graphics/SSR/SSR.h/cpp` - SSR system with ray marching

**Modified Files:**
- `Engine/Graphics/Renderer.h/cpp` - Added SSR integration
- `Engine/Graphics/Deferred/DeferredRenderer.h/cpp` - Added lighting output texture
- `Engine/Engine.vcxproj` - Added SSR files

**Features:**
- Ray marching based reflections
- Fresnel effect for realistic reflections
- Edge fade for smooth screen boundaries
- Configurable parameters (MaxDistance, Intensity, EdgeFade, FresnelPower)
- Integration with Deferred rendering pipeline

#### Technical Details

**SSR Parameters:**
- MaxDistance: 100.0f (maximum ray march distance)
- Resolution: 0.5f (resolution scale)
- Thickness: 0.5f (object thickness for hit detection)
- StepCount: 32 (ray march steps)
- MaxSteps: 64 (maximum iterations)
- EdgeFade: 0.1f (screen edge fade factor)
- FresnelPower: 3.0f (fresnel effect power)
- Intensity: 1.0f (reflection intensity)

**Render Targets:**
- SSR Texture: R16G16B16A16_FLOAT format
- Combined Texture: R16G16B16A16_FLOAT format

---

### Phase 9: Temporal Anti-Aliasing

**Status**: ✅ Completed  
**Target Completion**: 2026-03-02  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| History buffer | ✅ | | Double-buffered history textures |
| TAA shader | ✅ | | Temporal reprojection and blending |
| YCoCg clipping | ✅ | | Color space clipping for ghosting reduction |
| Sharpening | ✅ | | Optional sharpening pass |

#### Implementation Details

**New Files:**
- `Engine/Graphics/TAA/TAA.h/cpp` - TAA system with history management

**Modified Files:**
- `Engine/Graphics/Renderer.h/cpp` - Added TAA integration
- `Engine/Engine.vcxproj` - Added TAA files

**Features:**
- Double-buffered history for temporal accumulation
- YCoCg color space clipping to reduce ghosting
- Velocity buffer support for motion-aware blending
- Optional sharpening pass
- Configurable blend factor and feedback parameters

#### Technical Details

**TAA Parameters:**
- BlendFactor: 0.1f (base blend factor)
- FeedbackMin: 0.88f (minimum feedback)
- FeedbackMax: 0.97f (maximum feedback)
- MotionBlurScale: 1.0f (motion blur scale)
- SharpenStrength: 0.5f (sharpening strength)

**Render Targets:**
- History Textures: 2x R16G16B16A16_FLOAT format (double-buffered)
- Output Texture: R16G16B16A16_FLOAT format

---

### Phase 10: Volumetric Fog

**Status**: ✅ Completed  
**Target Completion**: 2026-03-02  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| Height fog | ✅ | | Exponential height-based fog |
| Volumetric lighting | ✅ | | In-scattering with light contribution |
| Ray marching | ✅ | | Step-based fog computation |
| Henyey-Greenstein phase | ✅ | | Anisotropic scattering |

#### Implementation Details

**New Files:**
- `Engine/Graphics/Volumetric/VolumetricFog.h/cpp` - Volumetric fog system

**Modified Files:**
- `Engine/Graphics/Renderer.h/cpp` - Added VolumetricFog integration
- `Engine/Engine.vcxproj` - Added VolumetricFog files

**Features:**
- Height-based exponential fog density
- Volumetric light in-scattering
- Henyey-Greenstein phase function for anisotropic scattering
- Configurable fog color, density, and height falloff
- Point light integration for light shafts

#### Technical Details

**Fog Parameters:**
- Density: 0.01f (base fog density)
- HeightFalloff: 0.1f (height-based density falloff)
- HeightBase: 0.0f (base height for fog)
- Scattering: 0.5f (in-scattering coefficient)
- Extinction: 0.01f (extinction coefficient)
- Anisotropy: 0.2f (phase function anisotropy)
- StepCount: 64 (ray march steps)
- MaxDistance: 100.0f (maximum fog distance)

**Render Targets:**
- Fog Texture: R16G16B16A16_FLOAT format
- Fog Output: R16G16B16A16_FLOAT format
- Combined Texture: R16G16B16A16_FLOAT format

---

### Phase 11: Cascaded Shadow Maps

**Status**: ✅ Completed  
**Target Completion**: 2026-03-02  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| CSM texture array | ✅ | | KCascadedShadowMap with up to 4 cascades |
| Cascade split calculation | ✅ | | Practical/Logarithmic split scheme |
| CSM renderer | ✅ | | KCascadedShadowRenderer with per-cascade rendering |
| Frustum-aligned cascade bounds | ✅ | | Automatic cascade bounds from camera frustum |
| Shadow map array SRV | ✅ | | Single SRV for all cascade shadow maps |

#### Implementation Details

**New Files:**
- `Engine/Graphics/Shadow/CascadedShadowMap.h/cpp` - CSM texture array management
- `Engine/Graphics/Shadow/CascadedShadowRenderer.h/cpp` - CSM rendering pipeline

**Features:**
- Up to 4 cascades for distance-based shadow quality
- Practical/Logarithmic split scheme with configurable lambda
- Frustum-aligned cascade bounds calculation
- Per-cascade light view-projection matrices
- Texture array for efficient shader binding
- Configurable cascade resolution, depth bias, PCF kernel

#### Technical Details

**CSM Parameters:**
- CascadeCount: 4 (default, max 4)
- CascadeResolution: 1024 (per cascade)
- SplitLambda: 0.5f (blend between practical and logarithmic splits)
- DepthBias: 0.0001f
- PCFKernelSize: 3

**Render Targets:**
- Cascade Textures: R24G8_TYPELESS (per cascade)
- Cascade Array SRV: R24_UNORM_X8_TYPELESS

**Cascade Calculation:**
- Frustum corner extraction from camera
- Per-cascade tight bounds fitting
- Light direction-aligned orthographic projection

---

### Phase 12: Screen Space Global Illumination

**Status**: ✅ Completed  
**Target Completion**: 2026-03-03  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| SSGI kernel generation | ✅ | | 16 samples with hemisphere distribution |
| Noise texture generation | ✅ | | 4x4 random rotation texture |
| SSGI shader | ✅ | | Ray marching with indirect lighting |
| Blur shader | ✅ | | Separable Gaussian blur for SSGI |
| Renderer integration | ✅ | | KSSGI class with Deferred rendering support |

#### Implementation Details

**New Files:**
- `Engine/Graphics/SSGI/SSGI.h/cpp` - SSGI system with kernel and noise generation

**Modified Files:**
- `Engine/Graphics/Renderer.h/cpp` - Added SSGI integration
- `Engine/Engine.vcxproj` - Added SSGI files

**Features:**
- 16-sample SSGI kernel with hemisphere distribution
- 4x4 random rotation noise texture
- Configurable radius, intensity, and falloff parameters
- Separable Gaussian blur for smoothing
- Integration with Deferred rendering pipeline
- Real-time toggle and parameter adjustment

#### Technical Details

**SSGI Parameters:**
- Radius: 0.5f (sample search radius)
- Intensity: 1.0f (indirect lighting intensity)
- SampleCount: 16 (number of samples to use)
- MaxSteps: 32 (maximum ray march steps)
- Thickness: 0.5f (object thickness for hit detection)
- Falloff: 1.0f (distance falloff power)
- TemporalBlend: 0.9f (temporal accumulation blend)
- BlurStrength: 1.0f (blur intensity)

**Render Targets:**
- SSGI Texture: R16G16B16A16_FLOAT format
- Blurred Output: R16G16B16A16_FLOAT format (ping-pong)
- History Texture: R16G16B16A16_FLOAT format (temporal)

**Shader Pipeline:**
1. Compute SSGI using Normal, Position, Depth, Albedo from G-Buffer
2. Apply separable Gaussian blur (horizontal + vertical)
3. Output indirect lighting contribution

---

### Phase 13: Particle System

**Status**: ✅ Completed  
**Target Completion**: 2026-03-03  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| Particle structure | ✅ | | FParticle struct with position, velocity, color, size |
| Particle emitter | ✅ | | KParticleEmitter with emission shapes |
| Particle shader | ✅ | | Billboard rendering with additive blending |
| Texture atlas support | ✅ | | Animated textures support |
| Renderer integration | ✅ | | Works with existing render pipeline |

#### Implementation Details

**New Files:**
- `Engine/Graphics/Particle/ParticleEmitter.h/cpp` - Particle system with emission shapes

**Modified Files:**
- `Engine/Engine.vcxproj` - Added Particle files

**Features:**
- Multiple emission shapes (Point, Sphere, Cone, Box)
- Configurable particle parameters (lifetime, size, color over time)
- Gravity and velocity simulation
- Billboard rendering with rotation
- Additive blending for fire/smoke effects
- Texture atlas support for animated particles
- Burst emission support

#### Technical Details

**Particle Parameters:**
- MaxParticles: 1000 (configurable)
- EmitRate: 10 particles per second
- Lifetime: Min/Max range
- Speed: Min/Max range
- Size: Min/Max range with interpolation
- Color: Start/End color interpolation
- Gravity: Physics simulation
- SpreadAngle: Direction variation

**Emitter Shapes:**
- Point: Single emission point
- Sphere: Volume or shell emission
- Cone: Directional cone shape
- Box: 3D box volume

**Rendering:**
- Dynamic vertex buffer for particles
- Billboard geometry in vertex shader
- Additive blending state
- Depth test without depth write

---

### Phase 14: Skeletal Mesh Rendering

**Status**: ✅ Completed  
**Target Completion**: 2026-03-07  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| SkeletalMeshComponent | ✅ | | FSkinnedVertex struct, KSkeletalMesh class |
| GPU skinning shader | ✅ | | Skinned vertex shader with bone matrix palette |
| Bone matrix constant buffer | ✅ | | FBoneMatrixBuffer (b5) with 256 bones |
| Renderer integration | ✅ | | RenderSkeletalMesh method in KRenderer |
| Animation playback | ✅ | | PlayAnimation, StopAnimation, PauseAnimation |

#### Implementation Details

**New Files:**
- `Engine/Assets/SkeletalMeshComponent.h/cpp` - Skeletal mesh component with GPU skinning

**Modified Files:**
- `Engine/Graphics/Shader.h/cpp` - Added CreateSkinnedShader method
- `Engine/Graphics/Renderer.h/cpp` - Added RenderSkeletalMesh method and SkinnedShader
- `Engine/Engine.vcxproj` - Added SkeletalMeshComponent files

**Features:**
- FSkinnedVertex with 4 bone influences and weights
- GPU skinning in vertex shader (bone matrix palette)
- Support for up to 256 bones per mesh
- Animation playback with multiple play modes (Once, Loop, PingPong)
- Bone matrix constant buffer (b5)
- Integration with existing animation system (KSkeleton, KAnimation, KAnimationInstance)

#### Technical Details

**Skinned Vertex Structure:**
- Position (float3)
- Color (float4)
- Normal (float3)
- TexCoord (float2)
- Tangent (float3)
- Bitangent (float3)
- BoneIndices (uint4) - Up to 4 bone influences
- BoneWeights (float4) - Weights for each bone

**Bone Matrix Buffer (b5):**
- Array of 256 XMMATRIX
- Transposed for HLSL

**Shader Pipeline:**
1. VS: Apply bone transforms based on weights
2. Transform position, normal, tangent, bitangent
3. Apply world/view/projection transforms
4. PS: Standard Phong lighting

**Animation Support:**
- Uses KAnimationInstance for runtime playback
- Automatic bone matrix computation
- Supports all animation play modes

---

### Phase 15: Enhanced Model Loading with FBX/GLTF Support

**Status**: ✅ Completed  
**Target Completion**: 2026-03-07  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| Fix OBJ loader bug | ✅ | | Mesh vertices/indices now properly set |
| Full Assimp integration | ✅ | | Complete Assimp implementation for FBX/GLTF |
| Skeleton extraction from FBX | ✅ | | BuildSkeletonRecursive with bind poses |
| Animation extraction | ✅ | | Full animation channel extraction |
| Bone weight extraction | ✅ | | Vertex bone weights with normalization |
| GLTF fallback loader | ✅ | | Basic GLTF/JSON parsing without Assimp |
| StaticMesh LOD methods | ✅ | | SetLODData, AddLOD methods |

#### Implementation Details

**Modified Files:**
- `Engine/Assets/ModelLoader.h/cpp` - Complete rewrite with Assimp integration
- `Engine/Assets/StaticMesh.h/cpp` - Added SetLODData, AddLOD methods

**Features:**
- Complete Assimp integration with USE_ASSIMP compile flag
- Automatic skeleton extraction with bone hierarchy
- Animation channel extraction with position/rotation/scale keys
- Vertex bone weight extraction with 4-bone limit and normalization
- Inverse bind pose matrix extraction
- Basic GLTF fallback parser (requires Assimp for full support)
- Improved OBJ loader with n-gon triangulation support

#### Technical Details

**Assimp Processing Flags:**
- aiProcess_Triangulate - Convert all faces to triangles
- aiProcess_JoinIdenticalVertices - Optimize vertex cache
- aiProcess_LimitBoneWeights - Limit to 4 bone influences
- aiProcess_CalcTangentSpace - Generate tangents for normal mapping
- aiProcess_GenSmoothNormals - Generate smooth normals if missing
- aiProcess_FlipUVs - Flip UV coordinates (configurable)

**Bone Processing:**
- Recursive skeleton building from aiNode hierarchy
- Automatic bind pose and inverse bind pose calculation
- Bone name to index mapping for fast lookups

**Animation Processing:**
- Position keys (aiVectorKey)
- Rotation keys (aiQuatKey with SLERP interpolation)
- Scale keys (aiVectorKey)
- Ticks per second conversion

**File Format Support:**
- FBX (with Assimp)
- GLTF/GLB (with Assimp)
- OBJ (built-in parser)
- DAE/Collada (with Assimp)
- 3DS (with Assimp)
- Blend (with Assimp)

---

### Phase 16: Advanced Post-Processing Effects

**Status**: ✅ Completed  
**Target Completion**: 2026-03-07  
**Commit Hash**: (pending commit)

#### Tasks

| Task | Status | Commit Hash | Notes |
|------|--------|-------------|-------|
| Motion Blur system | ✅ | | KMotionBlur with velocity-based blur |
| Depth of Field system | ✅ | | KDepthOfField with CoC calculation |
| Chromatic Aberration effect | ✅ | | KLensEffects combined class |
| Vignette effect | ✅ | | KLensEffects combined class |
| Film Grain effect | ✅ | | KLensEffects combined class |
| Renderer integration | ✅ | | Added to KRenderer with enable/disable methods |

#### Implementation Details

**New Files:**
- `Engine/Graphics/PostProcess/MotionBlur.h/cpp` - Motion blur with velocity buffer
- `Engine/Graphics/PostProcess/DepthOfField.h/cpp` - DoF with Circle of Confusion
- `Engine/Graphics/PostProcess/LensEffects.h/cpp` - Combined lens effects (CA, Vignette, Film Grain)

**Features:**
- Motion Blur: Velocity-based blur with configurable intensity and samples
- Depth of Field: Focus distance, range, and blur radius control
- Chromatic Aberration: RGB channel separation based on distance from center
- Vignette: Adjustable screen edge darkening
- Film Grain: Animated noise with configurable intensity

#### Technical Details

**Motion Blur Parameters:**
- Intensity: 1.0f (blur strength)
- MaxSamples: 16 (quality/performance trade-off)
- MinVelocity: 1.0f (minimum velocity to trigger blur)
- MaxVelocity: 100.0f (maximum velocity for blur calculation)

**Depth of Field Parameters:**
- FocusDistance: 10.0f (distance to focal plane)
- FocusRange: 5.0f (depth of field range)
- BlurRadius: 5.0f (maximum blur radius)
- MaxBlurSamples: 8 (quality setting)

**Lens Effects Parameters:**
- ChromaticAberrationStrength: 0.005f
- VignetteIntensity: 0.5f
- VignetteSmoothness: 0.5f
- FilmGrainIntensity: 0.1f

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
│   ├── ShadowRenderer.h/cpp
│   ├── CascadedShadowMap.h/cpp
│   └── CascadedShadowRenderer.h/cpp
├── Deferred/                   # Deferred rendering (Implemented)
│   ├── GBuffer.h/cpp
│   └── DeferredRenderer.h/cpp
├── IBL/                        # Image-based lighting (Implemented)
│   └── IBLSystem.h/cpp
├── PostProcess/                # Post-processing (Implemented)
│   └── PostProcessor.h/cpp
├── Culling/                    # Culling systems (Implemented)
│   ├── Frustum.h/cpp
│   └── OcclusionQuery.h/cpp
├── CommandBuffer/              # Command buffer (Implemented)
│   └── CommandBuffer.h/cpp
├── Instanced/                  # Instanced rendering (Implemented)
│   └── InstancedRenderer.h/cpp
├── SSAO/                       # Screen Space Ambient Occlusion (Implemented)
│   └── SSAO.h/cpp
├── SSR/                        # Screen Space Reflections (Implemented)
│   └── SSR.h/cpp
├── TAA/                        # Temporal Anti-Aliasing (Implemented)
│   └── TAA.h/cpp
├── Volumetric/                 # Volumetric Fog (Implemented)
│   └── VolumetricFog.h/cpp
├── SSGI/                       # Screen Space Global Illumination (Implemented)
│   └── SSGI.h/cpp
├── Particle/                   # Particle System (Implemented)
│   └── ParticleEmitter.h/cpp
└── Performance/                # GPU performance (Implemented)
    └── GPUTimer.h/cpp
```

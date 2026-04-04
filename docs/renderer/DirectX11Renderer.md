# DirectX 11 Renderer Documentation

## Overview

This document describes the DirectX 11 renderer architecture for KojeomGameEngine. The renderer provides a modern, flexible rendering pipeline with both Forward and Deferred rendering paths, PBR materials, and 20 advanced rendering sub-systems.

## Architecture

### Core Components

#### KGraphicsDevice
The graphics device manager handles all DirectX 11 core objects:

- **ID3D11Device**: Device for creating GPU resources
- **ID3D11DeviceContext**: Device context for rendering commands
- **IDXGISwapChain**: Swap chain for presenting frames
- **ID3D11RenderTargetView**: Render target view for the back buffer
- **ID3D11DepthStencilView**: Depth stencil view for depth testing

```cpp
KGraphicsDevice GraphicsDevice;
HRESULT hr = GraphicsDevice.Initialize(hWnd, 1280, 720, true);
```

#### KRenderer
The central rendering orchestrator manages the full rendering pipeline:

- Frame management (`BeginFrame`/`EndFrame`)
- Dual render paths: Forward and Deferred
- Built-in mesh creation (Triangle, Quad, Cube, Sphere)
- Lighting integration (Directional, Point, Spot)
- Shadow mapping
- Post-processing pipeline
- All 20+ sub-system orchestration

```cpp
KRenderer Renderer;
Renderer.Initialize(&GraphicsDevice);

// Forward rendering
Renderer.SetRenderPath(ERenderPath::Forward);
Renderer.BeginFrame(&Camera);
Renderer.RenderMeshLit(CubeMesh, WorldMatrix, Texture);
Renderer.EndFrame(true);

// Deferred rendering
Renderer.SetRenderPath(ERenderPath::Deferred);
Renderer.BeginFrame(&Camera);
Renderer.RenderMeshPBR(CubeMesh, WorldMatrix, Material);
Renderer.EndFrame(true);
```

#### KCamera
Camera system with perspective and orthographic projection:

- View matrix calculation with position/rotation
- Perspective and orthographic projection modes
- Dirty flag optimization for matrix updates
- Mouse look and WASD movement support

#### KShader / KShaderProgram
HLSL shader compilation and management:

- `KShader`: Individual shader wrapper (vertex, pixel, geometry, hull, domain, compute)
- `KShaderProgram`: Linked shader program with input layout and constant buffers
- Compile from file or embedded string source
- Input layout creation
- Constant buffer management
- Built-in shader programs: Basic, Phong, PhongShadow, PBR, Skinned, Water, DepthOnly

#### KMesh
GPU mesh geometry handling:

- Vertex buffer, index buffer, constant buffer
- Factory methods: `CreateTriangle()`, `CreateQuad()`, `CreateCube()`, `CreateSphere()`
- Transform constant buffer updates

#### KMaterial / FPBRMaterialParams
PBR material system with Metallic-Roughness workflow:

- 7 texture slots: Albedo, Normal, Metallic, Roughness, AO, Emissive, Height
- Constant buffer with material parameters
- Material presets: Metal, Plastic, Rubber, Gold, Silver, Copper

#### KTexture / KTextureManager
Texture resource management with caching:

- File loading from disk
- Solid color and checkerboard generation
- Sampler state management
- TextureManager with path-based caching

## Rendering Pipeline

### Forward Rendering Path

```
┌─────────────────────────────────────────────────────────────┐
│                    Forward Frame Cycle                        │
├─────────────────────────────────────────────────────────────┤
│  1. BeginFrame()                                            │
│     ├── Clear Back Buffer & Depth Buffer                    │
│     ├── Update Camera Matrices                              │
│     └── Update Light Constant Buffer                        │
│                                                             │
│  2. Shadow Pass                                             │
│     ├── ShadowMap / CascadedShadowMap rendering             │
│     └── Depth-only pass from light perspective              │
│                                                             │
│  3. Opaque Pass (per object)                                │
│     ├── Frustum/Occlusion culling check                     │
│     ├── Bind Shader (Phong/PBR/Skinned)                     │
│     ├── Bind Material/Textures                              │
│     ├── Update Transform Constant Buffer                    │
│     └── DrawIndexed                                         │
│                                                             │
│  4. Post-Processing                                         │
│     ├── HDR -> Bloom -> Auto Exposure                       │
│     ├── DOF, Motion Blur, Lens Effects                      │
│     └── Tone mapping -> LDR output                          │
│                                                             │
│  5. EndFrame()                                              │
│     └── Present (with optional V-Sync)                      │
└─────────────────────────────────────────────────────────────┘
```

### Deferred Rendering Path

```
┌─────────────────────────────────────────────────────────────┐
│                   Deferred Frame Cycle                        │
├─────────────────────────────────────────────────────────────┤
│  1. Geometry Pass (G-Buffer via KGBuffer)                   │
│     ├── World Position (RT0)                                │
│     ├── World Normal (RT1)                                  │
│     ├── Albedo + Alpha (RT2)                                │
│     ├── PBR Params (Metallic, Roughness, AO) (RT3)          │
│     └── Emissive (RT4)                                      │
│                                                             │
│  2. Lighting Pass                                           │
│     ├── SSAO computation                                    │
│     ├── SSGI computation                                    │
│     ├── Screen-space lighting accumulation                  │
│     └── SSR computation                                     │
│                                                             │
│  3. Forward+ (transparent objects)                           │
│                                                             │
│  4. Post-Processing (same as Forward)                       │
│                                                             │
│  5. EndFrame() -> Present                                   │
└─────────────────────────────────────────────────────────────┘
```

## Shader Constant Buffers

### b0: Transform Buffer (per-object)
```hlsl
cbuffer TransformBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    matrix WorldViewProjection;
};
```

### b1: Light Buffer (per-frame)
```hlsl
cbuffer LightBuffer : register(b1)
{
    float3 LightDirection;
    float Padding0;
    float3 LightColor;
    float Padding1;
    float3 AmbientColor;
    float Padding2;
    float3 CameraPosition;
    float Padding3;
};
```

### b2+: Material / Custom Buffers
```hlsl
cbuffer MaterialBuffer : register(b2)
{
    float4 AlbedoColor;
    float Metallic;
    float Roughness;
    float AO;
    float EmissiveStrength;
    // ... PBR parameters
};
```

## Texture Register Conventions

| Register | Usage |
|----------|-------|
| t0 | Albedo/Diffuse |
| t1 | Normal map |
| t2 | Metallic map |
| t3 | Roughness map |
| t4 | AO map |
| t5 | Emissive map |
| t6 | Height map |

## Sub-System Details

### Shadow System (`Graphics/Shadow/`)
- **KShadowMap**: Standard shadow map rendering
- **KShadowRenderer**: Shadow pass orchestration
- **KCascadedShadowMap**: Cascaded shadow maps for large scenes
- **KCascadedShadowRenderer**: CSM pass with cascade splitting

### Post-Processing Pipeline (`Graphics/PostProcess/`)
- **KPostProcessor**: Central post-processing orchestrator
- **KLensEffects**: Bloom effect (multi-pass Gaussian blur)
- **KAutoExposure**: Automatic exposure control
- **KDepthOfField**: Depth-based blur
- **KMotionBlur**: Velocity-based motion blur

### Screen-Space Effects
- **KSSAO** (`Graphics/SSAO/`): Screen-space ambient occlusion
- **KSSR** (`Graphics/SSR/`): Screen-space reflections
- **KTAA** (`Graphics/TAA/`): Temporal anti-aliasing
- **KSSGI** (`Graphics/SSGI/`): Screen-space global illumination

### Environment Rendering
- **KSkySystem** (`Graphics/Sky/`): Procedural sky rendering
- **KVolumetricFog** (`Graphics/Volumetric/`): Volumetric fog
- **KWater** (`Graphics/Water/`): Water rendering with reflections; **KWaterComponent** for Actor-Component integration
- **KTerrain** (`Graphics/Terrain/`): Heightmap-based terrain; **KTerrainComponent** for Actor-Component integration

### IBL System (`Graphics/IBL/`)
- Diffuse irradiance map generation
- Specular prefiltered environment map
- BRDF integration LUT

### Optimization Systems
- **KFrustum** (`Graphics/Culling/`): Frustum culling
- **KOcclusionQuery** (`Graphics/Culling/`): GPU occlusion culling
- **KInstancedRenderer** (`Graphics/Instanced/`): GPU instanced rendering
- **KLODSystem/KLODGenerator** (`Graphics/LOD/`): Automatic LOD generation
- **KParticleEmitter** (`Graphics/Particle/`): GPU particle system
- **KCommandBuffer** (`Graphics/CommandBuffer/`): Deferred command recording
- **KGPUTimer** (`Graphics/Performance/`): GPU timing queries

### Skeletal Animation Rendering
- **Skinned shader**: Vertex shader with bone matrix palette
- **Bone matrix buffer**: Up to 256 bones, 4 influences per vertex
- **RenderSkeletalMesh()**: Dedicated skeletal mesh rendering path

## API Reference

### KRenderer Key Methods

| Method | Description |
|--------|-------------|
| `Initialize(KGraphicsDevice*)` | Initialize renderer with graphics device |
| `BeginFrame(KCamera*, ClearColor)` | Begin rendering frame |
| `EndFrame(bVSync)` | End and present frame |
| `RenderMesh(Mesh, WorldMatrix, Texture)` | Render mesh with basic shader |
| `RenderMeshLit(Mesh, WorldMatrix, Texture)` | Render mesh with Phong lighting |
| `RenderMeshPBR(Mesh, WorldMatrix, Material)` | Render mesh with PBR |
| `RenderSkeletalMesh(SkelMesh, Skeleton, AnimInstance, WorldMatrix)` | Render skeletal mesh with GPU skinning |
| `SetDirectionalLight(Light)` | Set directional light |
| `AddPointLight(Light)` / `AddSpotLight(Light)` | Add dynamic lights |
| `SetRenderPath(ERenderPath)` | Switch between Forward/Deferred |
| `SetShadowEnabled(bool)` | Enable/disable shadow mapping |
| `SetSSAOEnabled(bool)` | Enable/disable SSAO |
| `SetPostProcessEnabled(bool)` | Enable/disable post-processing |
| `SetDebugMode(bool)` | Toggle debug visualization |
| `GetDrawCallCount()` / `GetVertexCount()` / `GetFrameTime()` | Performance stats |

### Mesh Factory Methods

```cpp
auto Triangle = Renderer.CreateTriangleMesh();
auto Quad = Renderer.CreateQuadMesh();
auto Cube = Renderer.CreateCubeMesh();
auto Sphere = Renderer.CreateSphereMesh(16, 16);
```

## Performance Guidelines

1. **Sort by shader, then texture** to minimize state changes
2. **Use GPU instancing** for repeated objects via `KInstancedRenderer`
3. **Map/Unmap** for frequently updated data, `UpdateSubresource` for static data
4. **Enable frustum culling** for large scenes
5. **Profile with `KGPUTimer`** to identify bottlenecks
6. **Use LOD** for distant objects

## Error Handling

```cpp
HRESULT hr = Renderer.Initialize(&GraphicsDevice);
if (FAILED(hr))
{
    LOG_ERROR("Failed to initialize renderer");
    LOG_HRESULT_ERROR(hr);
    return hr;
}
```

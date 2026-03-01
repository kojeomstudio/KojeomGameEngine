# DirectX 11 Renderer Documentation

## Overview

This document describes the DirectX 11 renderer architecture for the KojeomGameEngine. The renderer is designed to provide a modern, flexible rendering pipeline while maintaining simplicity and performance.

## Current Architecture

### Core Components

#### KGraphicsDevice
The graphics device manager handles all DirectX 11 core objects:

- **ID3D11Device**: Device for creating resources
- **ID3D11DeviceContext**: Device context for rendering commands
- **IDXGISwapChain**: Swap chain for presenting frames
- **ID3D11RenderTargetView**: Render target view for the back buffer
- **ID3D11DepthStencilView**: Depth stencil view for depth testing

```cpp
// Initialization example
KGraphicsDevice GraphicsDevice;
HRESULT hr = GraphicsDevice.Initialize(hWnd, 1280, 720, true);
```

#### KRenderer
The main renderer class manages the rendering pipeline:

- Frame management (BeginFrame/EndFrame)
- Object rendering with shaders and textures
- Default resource creation (shaders, meshes)
- Lighting integration

```cpp
KRenderer Renderer;
Renderer.Initialize(&GraphicsDevice);

// Frame rendering
Renderer.BeginFrame(&Camera);
Renderer.RenderMeshLit(CubeMesh, WorldMatrix, Texture);
Renderer.EndFrame(true); // with V-Sync
```

#### KCamera
Camera system with view/projection matrix management:

- View matrix calculation
- Projection matrix (perspective)
- Camera movement and rotation

#### KShaderProgram
Shader program management:

- Vertex and Pixel shader compilation
- Input layout creation
- Constant buffer management
- Built-in shader creation methods

#### KMesh
Mesh geometry handling:

- Vertex and Index buffers
- Primitive topology
- Constant buffer for transform matrices
- Factory methods for basic shapes (Triangle, Quad, Cube, Sphere)

#### KTexture
Texture resource management:

- Texture loading from files
- Sampler state creation
- Default white texture

## Rendering Pipeline

### Frame Flow

```
┌─────────────────────────────────────────────────────────────┐
│                         Frame Cycle                          │
├─────────────────────────────────────────────────────────────┤
│  1. BeginFrame()                                            │
│     ├── Update Camera Matrices                              │
│     ├── Clear Back Buffer & Depth Buffer                    │
│     └── Update Light Constant Buffer                        │
│                                                             │
│  2. Render Calls (per object)                               │
│     ├── Bind Shader Program                                 │
│     ├── Bind Texture (if any)                               │
│     ├── Update Mesh Constant Buffer (WVP matrices)          │
│     └── Draw Indexed                                        │
│                                                             │
│  3. EndFrame()                                              │
│     └── Present (with optional V-Sync)                      │
└─────────────────────────────────────────────────────────────┘
```

### Shader Constant Buffers

#### b0: Transform Buffer (per-mesh)
```hlsl
cbuffer TransformBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    matrix WorldViewProjection;
};
```

#### b1: Light Buffer (per-frame)
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

## Built-in Shaders

### Basic Color Shader
Simple shader for rendering with vertex colors:
- Input: Position, Color
- No lighting calculations
- Useful for debugging and UI

### Phong Lighting Shader
Shader with Phong lighting model:
- Input: Position, Normal, UV, Tangent, Bitangent
- Diffuse lighting with directional light
- Ambient lighting
- Specular highlights
- Optional texture mapping

## Mesh Creation

The renderer provides factory methods for creating basic meshes:

```cpp
// Create basic shapes
auto Triangle = Renderer.CreateTriangleMesh();
auto Quad = Renderer.CreateQuadMesh();
auto Cube = Renderer.CreateCubeMesh();
auto Sphere = Renderer.CreateSphereMesh(32, 32); // slices, stacks
```

## Future Development Plans

### Phase 1: Core Enhancements
- [ ] Deferred rendering pipeline
- [ ] Multiple render targets (G-Buffer)
- [ ] Point and Spot lights
- [ ] Shadow mapping

### Phase 2: Advanced Features
- [ ] Post-processing effects
- [ ] HDR rendering
- [ ] PBR materials
- [ ] Screen-space reflections

### Phase 3: Optimization
- [ ] Frustum culling
- [ ] Occlusion culling
- [ ] Instanced rendering
- [ ] GPU particle systems

## API Reference

### KRenderer Methods

| Method | Description |
|--------|-------------|
| `Initialize(KGraphicsDevice*)` | Initialize renderer with graphics device |
| `BeginFrame(KCamera*, ClearColor)` | Begin rendering frame |
| `EndFrame(bVSync)` | End and present frame |
| `RenderObject(FRenderObject&)` | Render a render object |
| `RenderMesh(Mesh, WorldMatrix, Texture)` | Render mesh with basic shader |
| `RenderMeshLit(Mesh, WorldMatrix, Texture)` | Render mesh with lighting |
| `SetDirectionalLight(Light)` | Set directional light properties |
| `Cleanup()` | Release all resources |

### FRenderObject Structure

```cpp
struct FRenderObject
{
    std::shared_ptr<KMesh> Mesh;
    std::shared_ptr<KShaderProgram> Shader;
    std::shared_ptr<KTexture> Texture;
    XMMATRIX WorldMatrix;
};
```

## Best Practices

1. **Resource Management**: Use `std::shared_ptr` for mesh and texture resources
2. **Frame Timing**: Always pair `BeginFrame()` with `EndFrame()`
3. **Shader Binding**: Minimize shader switches by sorting objects by shader
4. **Constant Buffers**: Update buffers only when data changes
5. **Debug Layer**: Enable debug layer during development for error messages

## Error Handling

The engine uses HRESULT for error reporting and includes a logging system:

```cpp
HRESULT hr = Renderer.Initialize(&GraphicsDevice);
if (FAILED(hr))
{
    LOG_ERROR("Failed to initialize renderer");
    return hr;
}
```

## Dependencies

- Windows SDK (DirectX 11)
- DirectXMath library
- stb_image for texture loading (planned)

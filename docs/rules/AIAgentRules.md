# AI Agent Rules for KojeomGameEngine

This document defines the rules and guidelines that AI agents must follow when working on the KojeomGameEngine project.

## Project Overview

KojeomGameEngine is a C++ game engine built with DirectX 11. The project follows a modular architecture with clear separation between core engine, graphics, and utility components.

## Code Style Guidelines

### Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Classes | PascalCase with 'K' prefix | `KRenderer`, `KGraphicsDevice` |
| Structs | PascalCase with 'F' prefix | `FRenderObject`, `FDirectionalLight` |
| Functions/Methods | PascalCase | `Initialize()`, `BeginFrame()` |
| Variables | camelCase with descriptive names | `graphicsDevice`, `currentCamera` |
| Member Variables | camelCase with 'b' prefix for booleans | `bInFrame`, `bDebugLayerEnabled` |
| Constants | PascalCase in namespace | `Colors::CornflowerBlue` |
| Enum values | PascalCase | `D3D_FEATURE_LEVEL_11_0` |

### File Organization

```
Engine/
├── Core/           # Engine core (initialization, main loop)
├── Graphics/       # Rendering components
│   ├── Renderer.h/cpp
│   ├── GraphicsDevice.h/cpp
│   ├── Camera.h/cpp
│   ├── Shader.h/cpp
│   ├── Mesh.h/cpp
│   ├── Texture.h/cpp
│   └── Light.h
└── Utils/          # Utility classes
    ├── Common.h
    └── Logger.h
```

### Header File Structure

```cpp
#pragma once

// 1. Project includes
#include "../Utils/Common.h"
#include "../Utils/Logger.h"

// 2. External includes
#include <d3d11.h>
#include <dxgi.h>

// 3. Standard library includes
#include <memory>
#include <vector>

/**
 * @brief Brief description of the class
 * 
 * Detailed description if necessary.
 */
class KClassName
{
public:
    // Constructor/Destructor
    KClassName() = default;
    ~KClassName() = default;
    
    // Prevent copying (if applicable)
    KClassName(const KClassName&) = delete;
    KClassName& operator=(const KClassName&) = delete;
    
    // Public methods
    HRESULT Initialize();
    void Cleanup();
    
private:
    // Private methods
    HRESULT CreateResources();
    
    // Member variables
    ComPtr<ID3D11Device> Device;
    bool bInitialized = false;
};
```

## DirectX 11 Specific Rules

### Resource Management

1. **Use ComPtr**: Always use `Microsoft::WRL::ComPtr` for COM objects
   ```cpp
   ComPtr<ID3D11Device> Device;
   ComPtr<ID3D11DeviceContext> Context;
   ```

2. **HRESULT Checking**: Always check HRESULT return values
   ```cpp
   HRESULT hr = Device->CreateBuffer(&desc, nullptr, &Buffer);
   if (FAILED(hr))
   {
       LOG_ERROR("Failed to create buffer");
       return hr;
   }
   ```

3. **Resource Creation Order**: Create resources in dependency order
   - Device → Context → SwapChain → RenderTargetView → DepthStencilView

### Constant Buffer Alignment

Constant buffers must be 16-byte aligned:
```cpp
struct FConstantBuffer
{
    XMMATRIX World;           // 64 bytes
    XMMATRIX View;            // 64 bytes
    XMMATRIX Projection;      // 64 bytes
};  // Total: 192 bytes (divisible by 16)

// For vectors, use padding:
struct FLightBuffer
{
    XMFLOAT3 LightDirection;  // 12 bytes
    float Padding0;           // 4 bytes (padding)
    XMFLOAT3 LightColor;      // 12 bytes
    float Padding1;           // 4 bytes (padding)
};  // Total: 32 bytes (divisible by 16)
```

### Shader Development

1. **Shader Model**: Use Shader Model 5.0 (`vs_5_0`, `ps_5_0`)
2. **Constant Buffer Registers**: 
   - `b0`: Transform buffer (per-object)
   - `b1`: Light buffer (per-frame)
   - `b2+`: Material/Custom buffers
3. **Texture Registers**:
   - `t0`: Diffuse texture
   - `t1+`: Normal, specular, etc.

## Documentation Rules

### Code Comments

1. **Doxygen-style documentation** for all public APIs:
   ```cpp
   /**
    * @brief Initialize the renderer
    * @param InGraphicsDevice Pointer to the graphics device
    * @return S_OK on success, error code on failure
    */
   HRESULT Initialize(KGraphicsDevice* InGraphicsDevice);
   ```

2. **Inline comments** for complex logic:
   ```cpp
   // Calculate half-pixel offset for DirectX 9 compatibility
   float2 halfPixel = float2(0.5f / width, 0.5f / height);
   ```

### File Headers

All source files should include a brief description at the top:
```cpp
/**
 * @file Renderer.cpp
 * @brief Implementation of the KRenderer class
 * 
 * Manages the rendering pipeline and coordinates
 * all graphics components.
 */
```

## Error Handling

1. **Use LOG macros** for logging:
   ```cpp
   LOG_INFO("Renderer initialized successfully");
   LOG_WARNING("Texture not found, using default");
   LOG_ERROR("Failed to create vertex buffer");
   ```

2. **Return HRESULT** for functions that can fail:
   ```cpp
   HRESULT CreateBuffer();
   ```

3. **Validate pointers** at function entry:
   ```cpp
   HRESULT Initialize(KGraphicsDevice* InDevice)
   {
       if (!InDevice)
       {
           LOG_ERROR("Invalid graphics device");
           return E_INVALIDARG;
       }
       // ...
   }
   ```

## Memory Management

1. **Use smart pointers** for automatic memory management:
   ```cpp
   std::shared_ptr<KMesh> Mesh;
   std::unique_ptr<KTextureManager> TextureManager;
   ```

2. **Prefer stack allocation** for small, short-lived objects

3. **Delete copy constructor/assignment** for resource-owning classes:
   ```cpp
   KGraphicsDevice(const KGraphicsDevice&) = delete;
   KGraphicsDevice& operator=(const KGraphicsDevice&) = delete;
   ```

## Performance Guidelines

1. **Minimize state changes**: Sort objects by shader, then by texture
2. **Batch draw calls**: Use instancing where possible
3. **Map/Unmap vs UpdateSubresource**: 
   - Use Map/Unmap for frequently updated data
   - Use UpdateSubresource for rarely updated data
4. **Avoid redundant updates**: Check if data actually changed before updating buffers

## Testing Requirements

1. **Test with Debug Layer**: Enable DirectX debug layer during development
2. **Check for memory leaks**: Use debug layer to detect resource leaks
3. **Test on different hardware**: Verify on different GPU vendors if possible

## Git Commit Guidelines

### Commit Message Format

```
[Category] Brief description

Detailed description if necessary.

- List of changes
- Another change
```

### Categories

- `[Core]`: Core engine changes
- `[Graphics]`: Graphics/renderer changes
- `[Docs]`: Documentation updates
- `[Build]`: Build system changes
- `[Refactor]`: Code refactoring
- `[Fix]`: Bug fixes
- `[Feature]`: New features

### Examples

```
[Graphics] Add deferred rendering support

Implemented G-Buffer creation and deferred shading pipeline.

- Added G-Buffer render targets (position, normal, albedo)
- Created deferred lighting pass shader
- Updated renderer to support both forward and deferred paths
```

## AI Agent Specific Instructions

1. **Always read existing code** before making changes to understand the patterns used
2. **Follow existing naming conventions** in the codebase
3. **Maintain consistency** with the existing architecture
4. **Document new features** in the appropriate docs folder
5. **Update work plans** in docs/plans/ with progress
6. **Test changes** before marking work as complete
7. **Keep changes focused** - one feature/fix per commit

## File Modification Rules

1. **Preserve existing structure** when modifying files
2. **Add new methods** at appropriate locations (grouped by functionality)
3. **Update headers** when adding new includes
4. **Maintain const correctness** for getter methods
5. **Keep the same code style** as surrounding code

## Prohibited Actions

1. **Do not** modify third-party library code
2. **Do not** commit debug/test code to main branch
3. **Do not** break existing API without updating all usages
4. **Do not** use raw pointers for owning resources
5. **Do not** ignore compiler warnings

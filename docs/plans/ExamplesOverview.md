# Examples Overview

This document provides an overview of the basic example programs available in the `Examples` folder.

## Available Examples

| Example | Description | Features Demonstrated |
|---------|-------------|----------------------|
| TriangleExample | Minimal triangle rendering | Vertex/Index buffers, basic shaders, constant buffers |
| BasicExample | Basic engine initialization | Engine setup, render loop, minimal rendering |
| AdvancedExample | Integrated rendering system | Mesh creation, lighting, camera, rotation |

## Building Examples

All examples are built as part of the main solution (`KojeomEngine.sln`). They depend on the Engine project.

### Build Configuration

- **Debug|x64**: Debug build with full debug symbols
- **Release|x64**: Optimized release build

### Build Order

1. Build the Engine project first (required dependency)
2. Build any example project

## Running Examples

After building, the example executables will be located in:
```
bin\x64\Debug\
bin\x64\Release\
```

Each example is a standalone executable that can be run independently.

## Example Details

### TriangleExample

- **File**: `Examples/TriangleExample.cpp`
- **Purpose**: Demonstrates the most basic DirectX 11 rendering
- **Features**:
  - Manual vertex buffer creation
  - Manual index buffer creation
  - Simple vertex/fragment shader
  - Basic constant buffer (World/View/Projection)
  - Triangle rotation animation
  - Color interpolation

### BasicExample

- **File**: `Examples/BasicExample.cpp`
- **Purpose**: Shows minimal engine usage pattern
- **Features**:
  - Engine initialization
  - Basic render loop
  - FPS display
  - Parent class override pattern

### AdvancedExample

- **File**: `Examples/AdvancedExample.cpp`
- **Purpose**: Demonstrates integrated rendering with multiple objects
- **Features**:
  - Mesh creation (triangle, cube, sphere)
  - Directional light setup
  - Camera positioning
  - Multiple object rendering
  - Object rotation animation
  - Phong lighting

## Difference Between Examples and Samples

- **Examples**: Basic/minimal demonstrations of engine usage patterns
- **Samples**: Feature-specific demonstrations of advanced engine features

For feature-specific demonstrations (PBR, Terrain, Particles, etc.), see the `samples/` folder and [SamplesOverview.md](SamplesOverview.md).

## Notes

- All examples require the Engine project to be built first
- Examples are designed to be educational and show minimal engine usage
- For production-ready code patterns, refer to the Samples folder

# KojeomEngine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows-blue.svg)](https://www.microsoft.com/windows)
[![Language: C++](https://img.shields.io/badge/Language-C++17-purple.svg)](https://isocpp.org/)
[![API: DirectX 11](https://img.shields.io/badge/API-DirectX%2011-green.svg)](https://docs.microsoft.com/en-us/windows/win32/direct3d11)

A lightweight, modular C++ game engine built on DirectX 11. Designed with simplicity, extensibility, and clean code principles in mind.

DirectX 11 기반의 경량화된 C++ 게임 엔진입니다. 모듈성과 확장성을 중시하여 설계되었으며, 간결한 코드 스타일과 쉬운 사용법을 목표로 합니다.

## 🎯 Features / 주요 특징

### Core Features
- **Lightweight Design**: Minimal dependencies - only DirectX 11 and standard C++ libraries
- **Modular Architecture**: Use only the components you need
- **Safe Resource Management**: Automatic memory management using ComPtr
- **Clean API**: Simple, intuitive interface for complex rendering operations

### Graphics System
- **DirectX 11 Rendering**: Full DirectX 11 support with feature level 11.0+
- **3D Camera System**: Perspective and orthographic projection support
- **Shader Management**: Runtime shader compilation with HLSL support
- **Mesh System**: Built-in primitives (triangle, cube, sphere, quad)
- **Texture System**: Runtime texture generation and caching
- **Lighting**: Directional light with Phong shading model

## 🏗️ Project Structure / 프로젝트 구조

```
KojeomEngine/
├── Engine/                     # Engine Core Library
│   ├── Core/
│   │   ├── Engine.h/cpp       # Main engine class
│   │   └── Engine.vcxproj     # Engine library project
│   ├── Graphics/
│   │   ├── GraphicsDevice.h/cpp  # DirectX 11 device management
│   │   ├── Camera.h/cpp          # 3D camera system
│   │   ├── Renderer.h/cpp        # Integrated rendering system
│   │   ├── Shader.h/cpp          # Shader management
│   │   ├── Mesh.h/cpp            # Mesh rendering system
│   │   ├── Texture.h/cpp         # Texture management
│   │   └── Light.h               # Lighting structures
│   └── Utils/
│       ├── Common.h           # Common headers and macros
│       └── Logger.h           # Logging system
├── Examples/                   # Example Applications
│   ├── BasicExample.cpp       # Basic window and rendering
│   ├── TriangleExample.cpp    # 3D triangle rendering
│   └── AdvancedExample.cpp    # Full-featured demo
├── KojeomEngine.sln           # Visual Studio Solution
├── LICENSE                    # MIT License
└── README.md                  # This file
```

## 🛠️ Requirements / 빌드 요구사항

- **Compiler**: Visual Studio 2019 or later (C++17 support)
- **Platform**: Windows 10/11
- **SDK**: Windows SDK (includes DirectX 11)
- **Optional**: Git for cloning the repository

## 🚀 Getting Started / 시작하기

### Clone the Repository

```bash
git clone https://github.com/YOUR_USERNAME/KojeomEngine.git
cd KojeomEngine
```

### Build with Visual Studio

1. Open `KojeomEngine.sln` in Visual Studio 2019 or later
2. Select your build configuration (Debug or Release)
3. Build the solution (Ctrl+Shift+B)
4. Run any example project (BasicExample, TriangleExample, or AdvancedExample)

### Build from Command Line

```cmd
# Debug build
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64

# Release build
msbuild KojeomEngine.sln /p:Configuration=Release /p:Platform=x64
```

## 📖 Usage / 사용법

### Basic Application

```cpp
#include "Engine/Core/Engine.h"

class MyGame : public KEngine
{
public:
    void Update(float deltaTime) override
    {
        KEngine::Update(deltaTime);
        // Your game logic here
    }

protected:
    void RenderFrame_Internal() override
    {
        auto renderer = GetRenderer();
        auto camera = GetCamera();
        
        // Render your objects here
        auto cube = renderer->CreateCubeMesh();
        XMMATRIX world = XMMatrixIdentity();
        renderer->RenderMeshLit(cube, world);
    }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    return KEngine::RunApplication<MyGame>(
        hInstance,
        L"My Game",
        1024, 768
    );
}
```

### Camera Control

```cpp
// Get camera from engine
KCamera* camera = GetCamera();

// Set position
camera->SetPosition(XMFLOAT3(0.0f, 5.0f, -10.0f));

// Look at target
camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

// Set perspective projection
camera->SetPerspective(XM_PIDIV4, 16.0f/9.0f, 0.1f, 1000.0f);
```

### Creating and Rendering Meshes

```cpp
// Create meshes
auto triangle = renderer->CreateTriangleMesh();
auto cube = renderer->CreateCubeMesh();
auto sphere = renderer->CreateSphereMesh(32, 16);

// Render with basic shader (no lighting)
XMMATRIX world = XMMatrixRotationY(angle);
renderer->RenderMeshBasic(triangle, world);

// Render with Phong lighting
renderer->RenderMeshLit(cube, world);

// Render with texture
auto texture = renderer->GetTextureManager()->GetCheckerboardTexture();
renderer->RenderMeshLit(sphere, world, texture);
```

### Lighting Setup

```cpp
FDirectionalLight light;
light.Direction = XMFLOAT3(0.5f, -1.0f, 0.5f);
light.Color = XMFLOAT4(1.0f, 0.95f, 0.9f, 1.0f);
light.AmbientColor = XMFLOAT4(0.1f, 0.1f, 0.15f, 1.0f);

renderer->SetDirectionalLight(light);
```

## 🎮 Examples / 예제

### BasicExample
A minimal example that creates a window and displays an FPS counter.
- Window creation and initialization
- Basic rendering loop
- FPS display in window title

### TriangleExample
Demonstrates 3D triangle rendering with custom shaders.
- Custom vertex and pixel shaders
- Basic mesh creation and rendering
- Matrix transformations

### AdvancedExample
A comprehensive demo showcasing all engine features.
- Multiple mesh types (triangle, cube, sphere)
- Phong lighting with directional light
- Texture mapping with procedural checkerboard
- Orbital camera movement
- Animated objects

## 📁 Core Components / 핵심 컴포넌트

### KEngine
The main engine class responsible for initialization, main loop, and window management.

### KGraphicsDevice
Manages DirectX 11 device, device context, and swap chain.

### KCamera
3D camera system with view and projection matrix management.

### KRenderer
Integrated rendering system that manages all graphics components.

### KShaderProgram
HLSL shader compilation and management.

### KMesh
3D mesh container with vertex and index buffers.

### KTexture / KTextureManager
Texture resource management with caching support.

### KLogger
Debug logging system with console and Visual Studio output support.

## 📝 Coding Style / 코딩 스타일

- **Naming Convention**: 
  - Classes: `K` prefix (e.g., `KEngine`, `KCamera`)
  - Functions: PascalCase (e.g., `Initialize()`, `RenderFrame()`)
  - Variables: camelCase with `m_` prefix for members (e.g., `m_rotationAngle`)
  - Constants: `UPPER_CASE` or `PascalCase` in namespaces
- **Comments**: Doxygen-style documentation for public APIs
- **Error Handling**: HRESULT-based error handling with logging

## 🗺️ Roadmap / 향후 계획

### Completed ✅
- [x] Core engine framework
- [x] DirectX 11 rendering pipeline
- [x] Camera system
- [x] Shader management
- [x] Mesh rendering system
- [x] Texture management
- [x] Basic lighting (Phong)

### Planned 🚧
- [ ] 3D model loading (.obj, .fbx)
- [ ] Image file loading (.png, .jpg, .dds)
- [ ] Input system (keyboard, mouse)
- [ ] Audio system
- [ ] Scene graph and transform hierarchy
- [ ] Animation system
- [ ] Advanced lighting (PBR)
- [ ] Post-processing effects
- [ ] ImGui integration
- [ ] Physics engine integration

## 🤝 Contributing / 기여하기

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License / 라이선스

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 📧 Contact / 연락처

For questions or suggestions, please open an issue on GitHub.

---

Made with ❤️ by Kojeom

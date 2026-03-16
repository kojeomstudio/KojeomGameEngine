# Samples Overview

This document provides an overview of the sample programs available in the `samples` folder.

> **Note**: For basic engine usage examples (Triangle, Basic, Advanced), see [ExamplesOverview.md](ExamplesOverview.md).

## Available Samples

| Sample | Description | Features Demonstrated |
|-------|-------------|---------------------|
| BasicRendering | Basic 3D rendering with meshes | Mesh creation, basic lighting, camera movement |
| Lighting | Advanced lighting and shadow system | Point lights, spot lights, shadows, light volume visualization |
| PBR | Physically Based Rendering materials | PBR shaders, material system, metalness/roughness workflow |
| PostProcessing | All post-processing effects | SSAO, SSR, TAA, bloom, tonemapping, FXAA, volumetric fog, motion blur, DoF, lens effects |
| Terrain | Terrain rendering with height maps | Height map generation, LOD system, terrain mesh, height queries |
| Water | Water rendering with wave animation | Gerstner waves, reflection/refraction, fresnel effect, foam |
| Sky | Atmospheric scattering / sky rendering | Rayleigh/Mie scattering, sun disk, time of day system, ACES tonemapping |
| Particles | Particle system with multiple emitters | Fire, smoke, sparks, snow particles, emitter shapes, billboard rendering |
| SkeletalMesh | Skeletal mesh rendering with animation | GPU skinning, bone hierarchy, animation playback, skeleton debug visualization |
| AnimationStateMachine | State-based animation system | Animation states, transitions, parameter-based blending, smooth animation transitions |
| Gameplay | Input and Audio system demonstration | Keyboard/mouse controls, player movement, particle effects, collectible system |
| Physics | Physics simulation demonstration | Rigid body dynamics, collision detection (sphere/box), impulse resolution, gravity |
| UI | UI system demonstration | Panels, text, buttons, click callbacks, hover states |
| UI/Layout | UI layout system demonstration | Vertical/horizontal layouts, grid layout, checkbox, slider controls |
| LOD | Level of Detail system demonstration | LOD mesh generation, distance-based LOD selection, smooth LOD blending |

## Building Samples

All samples are built as part of the main solution (`KojeomEngine.sln`). They depend on the Engine project.

### Build Configuration

- **Debug|x64**: Debug build with full debug symbols
- **Release|x64**: Optimized release build

### Build Order

1. Build the Engine project first (required dependency)
2. Build any sample project

## Running Samples

After building, the sample executables will be located in:
```
samples/<SampleName>/bin/x64/Debug/
samples/<SampleName>/bin/x64/Release/
```

For example:
- `samples/BasicRendering/bin/x64/Release/BasicRenderingSample.exe`
- `samples/Gameplay/bin/x64/Release/GameplaySample.exe`

Each sample is a standalone executable that can be run independently.

## Sample Details

### BasicRendering Sample
- **File**: `samples/BasicRendering/BasicRenderingSample.cpp`
- **Features**: 
  - Cube and sphere mesh rendering
  - Rotating objects with orbital camera
  - Checkerboard texture on floor
  - Basic Phong lighting

### Lighting Sample
- **File**: `samples/Lighting/LightingSample.cpp`
- **Features**:
  - Multiple point lights (3) with animation
  - Spot light
  - Shadow mapping
  - Light volume visualization (debug mode)
  - Dynamic light color changes

### PBR Sample
- **File**: `samples/PBR/PBRSample.cpp`
- **Features**:
  - PBR material presets (Gold, Silver, Copper, Plastic, Rubber)
  - Material grid showing metallic/roughness variations
  - Deferred rendering path
  - SSAO integration

### PostProcessing Sample
- **File**: `samples/PostProcessing/PostProcessingSample.cpp`
- **Features**:
  - SSAO (Screen Space Ambient Occlusion)
  - SSR (Screen Space Reflections)
  - TAA (Temporal Anti-Aliasing)
  - Bloom effect
  - Motion blur
  - Depth of field
  - Lens effects (chromatic aberration, vignette, film grain)
  - Volumetric fog

### Terrain Sample
- **File**: `samples/Terrain/TerrainSample.cpp`
- **Features**:
  - Perlin noise height map generation
  - Terrain LOD system (4 levels)
  - Height queries for object placement
  - Atmospheric fog integration
  - Shadow mapping

### Water Sample
- **File**: `samples/Water/WaterSample.cpp`
- **Features**:
  - Gerstner wave animation (4 wave components)
  - Water surface rendering
  - Sky system integration
  - Floating objects animation
  - Reflection/refraction support

### Sky Sample
- **File**: `samples/Sky/SkySample.cpp`
- **Features**:
  - Atmospheric scattering (Rayleigh + Mie)
  - Dynamic time of day system (24-hour cycle)
  - Sun disk rendering
  - Automatic lighting updates
  - ACES tonemapping

### Particles Sample
- **File**: `samples/Particles/ParticlesSample.cpp`
- **Features**:
  - Multiple particle emitters (4 types)
  - Fire particles (cone emission)
  - Smoke particles (point emission)
  - Spark particles (sphere emission)
  - Snow particles (box emission)
  - Bloom effect for bright particles

### SkeletalMesh Sample
- **File**: `samples/SkeletalMesh/SkeletalMeshSample.cpp`
- **Features**:
  - Procedural skeleton creation (12 bones)
  - GPU skinning with bone matrix palette
  - Procedural animation (waving arm)
  - Skeleton debug visualization
  - Bone hierarchy with bind poses
  - Animation playback system

### AnimationStateMachine Sample
- **File**: `samples/AnimationStateMachine/AnimationStateMachineSample.cpp`
- **Features**:
  - State-based animation system (Idle, Walk, Run)
  - Parameter-driven transitions (Speed parameter)
  - Smooth animation blending between states
  - Condition-based transition rules (Speed > 0.1, Speed > 2.0, etc.)
  - Procedural skeleton with 10 bones
  - Keyboard input for movement (WASD + Shift for run)
  - Orbiting camera with character tracking
  - Real-time state debugging display

### Gameplay Sample
- **File**: `samples/Gameplay/GameplaySample.cpp`
- **Features**:
  - Input system integration (WASD movement, mouse look)
  - Action mapping system (MoveForward, MoveBackward, MoveLeft, MoveRight, Jump, Sprint)
  - Player character with third-person camera
  - Collectible system with collision detection
  - Particle effects for player movement
  - Audio system integration (ready for sound effects)
  - Jump mechanics with gravity

### Physics Sample
- **File**: `samples/Physics/PhysicsSample.cpp`
- **Features**:
  - Rigid body dynamics simulation
  - Sphere and box collision detection
  - Impulse-based collision resolution
  - Gravity and damping
  - Positional correction (Baumgarte stabilization)
  - Fixed timestep simulation with sub-stepping

### UI Sample
- **File**: `samples/UI/UISample.cpp`
- **Features**:
  - Panel elements with background colors
  - Text labels with various fonts
  - Interactive buttons with click callbacks
  - Hover and pressed states
  - Click counter demonstration

### UI Layout Sample
- **File**: `samples/UI/Layout/LayoutSample.cpp`
- **Features**:
  - Vertical layout container
  - Horizontal layout container
  - Grid layout container
  - Checkbox controls with value change callbacks
  - Slider controls with min/max range
  - Automatic child sizing

### LOD Sample
- **File**: `samples/LOD/LODSample.cpp`
- **Features**:
  - Quadric Error Metric mesh simplification (LOD generation)
  - Distance-based LOD selection (4 LOD levels)
  - Smooth LOD blending transitions
  - 25 spheres at various distances to demonstrate LOD switching
  - Configurable LOD distances (10, 25, 50, 100 units)
  - W/S keys to move camera and observe LOD transitions

## Adding New Samples

To add a new sample:

1. Create a new folder under `samples/`
2. Create the sample `.cpp` file
3. Create the corresponding `.vcxproj` file (copy from existing sample)
4. Add the project to `KojeomEngine.sln`
5. Build and test

## Notes

- All samples require the Engine project to be built first
- Samples use the engine's rendering pipeline
- Each sample demonstrates specific engine features
- Samples are designed to be educational and demonstrate engine capabilities

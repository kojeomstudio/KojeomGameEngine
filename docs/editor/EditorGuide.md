# KojeomGameEngine Editor Guide

## Table of Contents

1. [Editor Overview](#1-editor-overview)
2. [Editor Structure](#2-editor-structure)
3. [EngineInterop - Native Bridge](#3-engineinterop---native-bridge)
4. [MVVM Architecture](#4-mvvm-architecture)
5. [Views](#5-views)
6. [UndoRedoService](#6-undoredoservice)
7. [Editor Features](#7-editor-features)
8. [Build Instructions](#8-build-instructions)

---

## 1. Editor Overview

KojeomGameEngine ships with a WPF-based visual editor built on **.NET 8.0** (x64 only). The editor provides a graphical interface for scene composition, PBR material editing, asset management, and real-time 3D viewport rendering powered by the native C++ engine via an interop bridge.

Key technical details:

- **Framework:** WPF (.NET 8.0, `net8.0-windows`), C# with nullable reference types enabled
- **Architecture:** MVVM (Model-View-ViewModel) pattern with data binding
- **Rendering:** Native DirectX 11 rendering embedded into the WPF window via Win32 HWND interop
- **Platform:** x64 only (`<Platforms>x64</Platforms>`, `<PlatformTarget>x64</PlatformTarget>`)
- **Project file:** `Editor/KojeomEditor/KojeomEditor.csproj`

The editor window is laid out as a classic game editor: scene hierarchy on the left, 3D viewport in the center, properties panel and material editor on the right, and a content browser at the bottom.

---

## 2. Editor Structure

```
Editor/
├── EngineInterop/           # C++ DLL exposing flat C API for P/Invoke
│   ├── EngineAPI.h          # Native function declarations (extern "C")
│   ├── EngineAPI.cpp        # Native function implementations
│   └── EngineInterop.vcxproj
│
└── KojeomEditor/            # C# WPF editor application
    ├── App.xaml / App.xaml.cs
    ├── MainWindow.xaml / MainWindow.xaml.cs   # Root window with menus, toolbar, status bar
    ├── KojeomEditor.csproj                   # .NET 8.0 project with native DLL copy target
    ├── Services/                             # Business logic and engine bridge
    │   ├── EngineInterop.cs                  # P/Invoke wrapper (96 DllImport declarations)
    │   └── UndoRedoService.cs                # Command-pattern undo/redo system
    ├── ViewModels/                           # MVVM ViewModels
    │   ├── MainViewModel.cs                  # Root VM, ViewModelBase, RelayCommand, ETransformMode
    │   ├── SceneViewModel.cs                 # Scene state, ActorViewModel, actor CRUD
    │   ├── PropertiesViewModel.cs             # Selection properties, material sync
    │   └── ComponentViewModel.cs             # Component VMs, MaterialViewModel
    ├── Views/                                # XAML UserControls
    │   ├── ViewportControl.xaml(.cs)         # Native HWND embedding, fly camera, object picking
    │   ├── SceneHierarchyControl.xaml(.cs)   # Actor tree, context menu, drag reorder
    │   ├── PropertiesPanelControl.xaml(.cs)  # Transform + component inspectors (pure XAML binding)
    │   ├── MaterialEditorControl.xaml(.cs)   # PBR material editing, 7 presets
    │   └── ContentBrowserControl.xaml(.cs)   # Folder tree, asset grid, thumbnails, drag-drop
    └── Styles/
        └── CommonStyles.xaml                 # Shared XAML styles
```

### Dependencies

The editor has minimal external dependencies, declared in `KojeomEditor.csproj`:

```xml
<ItemGroup>
    <PackageReference Include="Microsoft.Extensions.Logging" Version="8.0.0" />
    <PackageReference Include="Microsoft.Extensions.Logging.Debug" Version="8.0.0" />
</ItemGroup>
```

---

## 3. EngineInterop - Native Bridge

### Architecture

The editor communicates with the native C++ engine through a **flat C API** bridge. The native side is compiled as a DLL (`EngineInterop.dll`) that exposes `extern "C"` functions, which the C# editor consumes via `DllImport` with `CallingConvention.Cdecl`.

```
C# Editor (EngineInterop.cs)  ──P/Invoke──>  EngineInterop.dll (EngineAPI.cpp)  ──>  Engine C++ Core
     96 DllImport declarations               extern "C" API (103 functions)        KEngine, KRenderer, etc.
```

### C++ Side (`Editor/EngineInterop/EngineAPI.h`)

All native functions are declared as `extern "C"` with `ENGINEAPI` export macro. Every engine object is passed as an opaque `void*` pointer. The API covers the following groups:

| Group | Functions | Description |
|-------|-----------|-------------|
| **Lifecycle** | `Engine_Create`, `Engine_Destroy`, `Engine_Initialize`, `Engine_InitializeEmbedded`, `Engine_Tick`, `Engine_Render`, `Engine_Resize` | Engine instance creation, initialization, main loop |
| **Scene** | `Scene_Create`, `Scene_Load`, `Scene_Save`, `Scene_SetActive`, `Scene_GetActive`, `Scene_Raycast`, `Scene_GetActorCount`, `Scene_GetActorAt` | Scene management and queries |
| **Actor** | `Actor_Create`, `Actor_Destroy`, `Actor_SetPosition`, `Actor_GetPosition`, `Actor_SetRotation`, `Actor_GetRotation`, `Actor_SetScale`, `Actor_GetScale`, `Actor_GetName`, `Actor_SetName`, `Actor_SetVisibility`, `Actor_IsVisible`, `Actor_AddChild`, `Actor_GetChildCount`, `Actor_GetChild`, `Actor_GetParent` | Actor transform, hierarchy, visibility |
| **Camera** | `Camera_GetMain`, `Camera_SetPosition`, `Camera_GetPosition`, `Camera_SetRotation`, `Camera_GetViewMatrix`, `Camera_GetProjectionMatrix`, `Camera_SetFOV`, `Camera_GetFOV`, `Camera_SetNearFar`, `Camera_GetNearZ`, `Camera_GetFarZ` | Camera control and matrix queries |
| **Renderer** | `Renderer_SetRenderPath`, `Renderer_SetDebugMode`, `Renderer_GetStats`, `Renderer_SetSSAOEnabled`, `Renderer_SetPostProcessEnabled`, `Renderer_SetShadowEnabled`, `Renderer_SetSkyEnabled`, `Renderer_SetTAAEnabled`, `Renderer_SetDebugUIEnabled`, `Renderer_SetSSREnabled`, `Renderer_SetVolumetricFogEnabled` | Rendering features and toggles |
| **Lighting** | `Renderer_SetDirectionalLight`, `Renderer_GetDirectionalLight`, `Renderer_SetDirectionalLightDirection`, `Renderer_SetDirectionalLightColor`, `Renderer_SetDirectionalLightAmbient`, `Renderer_SetDirectionalLightIntensity`, `Renderer_GetDirectionalLightIntensity`, `Renderer_AddPointLight`, `Renderer_GetPointLightCount`, `Renderer_GetPointLight`, `Renderer_ClearPointLights`, `Renderer_AddSpotLight`, `Renderer_ClearSpotLights`, `Renderer_SetShadowSceneBounds` | Directional, point, and spot lights |
| **Material** | `Material_SetAlbedo`, `Material_GetAlbedo`, `Material_SetMetallic`, `Material_GetMetallic`, `Material_SetRoughness`, `Material_GetRoughness`, `Material_SetAO`, `Material_GetAO` | PBR material properties |
| **Components** | `Actor_AddComponent`, `Actor_GetComponentCount`, `Actor_GetComponentName`, `Actor_GetComponentType`, `Actor_GetStaticMeshComponent`, `Actor_GetSkeletalMeshComponent`, `Actor_GetLightComponent`, `StaticMeshComponent_SetMesh`, `StaticMeshComponent_GetMaterial`, `StaticMeshComponent_CreateDefaultMesh` | Component management |
| **Animation** | `SkeletalMeshComponent_PlayAnimation`, `SkeletalMeshComponent_StopAnimation`, `SkeletalMeshComponent_PauseAnimation`, `SkeletalMeshComponent_ResumeAnimation`, `SkeletalMeshComponent_GetAnimationCount`, `SkeletalMeshComponent_GetAnimationName`, `SkeletalMeshComponent_SetSkeletalMeshFromModel` | Skeletal animation playback |
| **Model/Texture** | `Model_Load`, `Model_Unload`, `Model_LoadAndGetStaticMesh`, `Model_LoadAndGetSkeletalMesh`, `Model_HasSkeleton`, `Model_GetAnimationCount`, `Model_GetAnimationName`, `Texture_Load`, `Texture_Unload` | Asset loading |

### C# Side (`Editor/KojeomEditor/Services/EngineInterop.cs`)

The `EngineInterop` class wraps all 96 P/Invoke declarations with a managed API. It implements `IDisposable` for proper cleanup of the native engine instance. Every method guards against uninitialized state by checking `_isInitialized` and `_enginePtr`.

**P/Invoke declaration pattern:**

```csharp
private const string DllName = "EngineInterop.dll";

[DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
private static extern IntPtr Engine_Create();

[DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
private static extern int Engine_Initialize(IntPtr engine, IntPtr hwnd, int width, int height);

[DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
private static extern void Actor_SetPosition(IntPtr actor, float x, float y, float z);

[DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
private static extern void Actor_GetPosition(IntPtr actor, ref float x, ref float y, ref float z);

[DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
private static extern void Material_SetAlbedo(IntPtr material, float r, float g, float b, float a);
```

**Managed wrapper pattern:**

```csharp
public class EngineInterop : IDisposable
{
    private IntPtr _enginePtr;
    private bool _isInitialized;

    public bool Initialize(IntPtr hwnd, int width, int height)
    {
        _enginePtr = Engine_Create();
        if (_enginePtr == IntPtr.Zero) return false;

        int hr = Engine_Initialize(_enginePtr, hwnd, width, height);
        if (hr < 0)
        {
            Engine_Destroy(_enginePtr);
            _enginePtr = IntPtr.Zero;
            return false;
        }

        _isInitialized = true;
        return true;
    }

    public void SetActorPosition(IntPtr actor, float x, float y, float z)
    {
        if (actor != IntPtr.Zero)
        {
            Actor_SetPosition(actor, x, y, z);
        }
    }

    public (float x, float y, float z) GetActorPosition(IntPtr actor)
    {
        float x = 0, y = 0, z = 0;
        Actor_GetPosition(actor, ref x, ref y, ref z);
        return (x, y, z);
    }

    public void Dispose()
    {
        if (_enginePtr != IntPtr.Zero)
        {
            Engine_Destroy(_enginePtr);
            _enginePtr = IntPtr.Zero;
        }
        _isInitialized = false;
        GC.SuppressFinalize(this);
    }
}
```

**String marshaling patterns:**

The API uses two string marshaling conventions depending on the C++ side:

- `LPStr` (ANSI) for actor/component names:
  ```csharp
  [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
  private static extern IntPtr Actor_Create(IntPtr scene, [MarshalAs(UnmanagedType.LPStr)] string name);
  ```

- `LPWStr` (Unicode/wide) for file paths:
  ```csharp
  [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
  private static extern IntPtr Scene_Load(IntPtr sceneMgr, [MarshalAs(UnmanagedType.LPWStr)] string path);
  ```

- Returned C strings are unmarshaled via `Marshal.PtrToStringAnsi`:
  ```csharp
  public string? GetActorName(IntPtr actor)
  {
      return Marshal.PtrToStringAnsi(Actor_GetName(actor));
  }
  ```

**Boolean marshaling:**

C++ `bool` parameters use explicit `UnmanagedType.I1` to match 1-byte C++ bools:

```csharp
[DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
private static extern void Renderer_SetSSAOEnabled(IntPtr renderer, [MarshalAs(UnmanagedType.I1)] bool enabled);

[DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
[return: MarshalAs(UnmanagedType.I1)]
private static extern bool Actor_IsVisible(IntPtr actor);
```

**Initialization modes:**

Two initialization paths exist for embedding the native renderer in WPF:

- `Initialize(hwnd, width, height)` - Full window initialization
- `InitializeEmbedded(hwnd, width, height)` - Child window embedding (used by ViewportControl)

---

## 4. MVVM Architecture

### Core Infrastructure

The MVVM infrastructure is defined in `ViewModels/MainViewModel.cs`:

#### ViewModelBase

Base class implementing `INotifyPropertyChanged`:

```csharp
public class ViewModelBase : System.ComponentModel.INotifyPropertyChanged
{
    public event System.ComponentModel.PropertyChangedEventHandler? PropertyChanged;

    protected void OnPropertyChanged(string propertyName)
    {
        PropertyChanged?.Invoke(this, new System.ComponentModel.PropertyChangedEventArgs(propertyName));
    }
}
```

#### RelayCommand

Generic command implementation supporting `CanExecute`:

```csharp
public class RelayCommand : ICommand
{
    private readonly Action<object?> _execute;
    private readonly Predicate<object?>? _canExecute;

    public RelayCommand(Action<object?> execute, Predicate<object?>? canExecute = null)
    {
        _execute = execute ?? throw new ArgumentNullException(nameof(execute));
        _canExecute = canExecute;
    }

    public event EventHandler? CanExecuteChanged
    {
        add => CommandManager.RequerySuggested += value;
        remove => CommandManager.RequerySuggested -= value;
    }

    public bool CanExecute(object? parameter) => _canExecute == null || _canExecute(parameter);
    public void Execute(object? parameter) => _execute(parameter);
}
```

#### Data Binding Flow

```
MainWindow.xaml (DataContext = MainViewModel)
  ├── ViewportControl    ← binds Engine, SceneViewModel, PropertiesViewModel via DependencyProperties
  ├── SceneHierarchyControl ← binds SceneViewModel.Actors via DataContext chain
  ├── PropertiesPanelControl ← binds PropertiesViewModel.SelectedActor via DataContext chain
  ├── MaterialEditorControl  ← binds PropertiesViewModel.Material via DependencyProperties
  └── ContentBrowserControl  ← self-contained DataContext, drag-drop events to ViewportControl

MainViewModel
  ├── SceneViewModel          → ObservableCollection<ActorViewModel> Actors
  │     └── ActorViewModel    → Position/Rotation/Scale properties (notify engine on change)
  └── PropertiesViewModel     → SelectedActor, MaterialViewModel
        └── MaterialViewModel → Albedo, Metallic, Roughness, AO (sync to engine)
```

**ViewModel hierarchy:**

| ViewModel | Location | Responsibility |
|-----------|----------|----------------|
| `MainViewModel` | `ViewModels/MainViewModel.cs` | Root VM, holds SceneViewModel + PropertiesViewModel, engine reference, transform mode |
| `SceneViewModel` | `ViewModels/SceneViewModel.cs` | Scene CRUD, actor collection, syncs property changes to native engine |
| `PropertiesViewModel` | `ViewModels/PropertiesViewModel.cs` | Selection forwarding, material sync between engine and MaterialViewModel |
| `ActorViewModel` | `ViewModels/SceneViewModel.cs` | Per-actor state: transform, visibility, name, components, native pointer |
| `ComponentViewModel` | `ViewModels/ComponentViewModel.cs` | Base + specialized component VMs (StaticMesh, Light, Camera) |
| `MaterialViewModel` | `ViewModels/ComponentViewModel.cs` | PBR material properties with PropertyChanged → engine sync |

**Property change propagation to engine:**

When a user edits a property in the UI (e.g., Position X), the data binding flow is:

1. TextBox edits `ActorViewModel.PositionX` via XAML binding
2. `ActorViewModel.OnPropertyChanged("PositionX")` fires
3. `SceneViewModel.OnActorPropertyChanged` handler detects the property name
4. The handler calls `EngineInterop.SetActorPosition()` to push the change to native code
5. If undo/redo is active, a `SetPropertyAction` is recorded before applying the change

```csharp
private void OnActorPropertyChanged(object? sender, PropertyChangedEventArgs e)
{
    if (_engine == null || !_engine.IsInitialized) return;
    if (sender is not ActorViewModel actor || actor.NativePtr == IntPtr.Zero) return;

    switch (e.PropertyName)
    {
        case nameof(ActorViewModel.PositionX):
        case nameof(ActorViewModel.PositionY):
        case nameof(ActorViewModel.PositionZ):
            _engine.SetActorPosition(actor.NativePtr,
                actor.PositionX, actor.PositionY, actor.PositionZ);
            break;
        // ... Rotation, Scale, Name, IsVisible
    }
}
```

---

## 5. Views

### 5.1 ViewportControl

**Location:** `Views/ViewportControl.xaml` / `Views/ViewportControl.xaml.cs`

The viewport embeds the native DirectX 11 renderer into the WPF window using Win32 interop. It creates a child `HWND` via `CreateWindowEx` and passes it to `Engine_InitializeEmbedded`.

**Native HWND embedding:**

```csharp
private void InitializeHwndHost()
{
    _hwndSource = PresentationSource.FromVisual(this) as HwndSource;
    _parentHwnd = _hwndSource?.Handle ?? IntPtr.Zero;

    var screenPoint = PointToScreen(new Point(0, 0));
    var clientPoint = new POINT((int)screenPoint.X, (int)screenPoint.Y);
    ScreenToClient(_parentHwnd, ref clientPoint);

    _childHwnd = CreateWindowEx(
        0, "Static", "KojeomViewport",
        WS_CHILD | WS_VISIBLE,
        clientPoint.X, clientPoint.Y, width, height,
        _parentHwnd, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero);

    _engine.InitializeEmbedded(_childHwnd, width, height);
    StartRendering();
}
```

**Rendering loop:**

Uses `CompositionTarget.Rendering` for a WPF-integrated render loop:

```csharp
private void OnRendering(object? sender, EventArgs e)
{
    var currentTime = _stopwatch?.Elapsed.TotalSeconds ?? 0;
    var deltaTime = (float)(currentTime - _lastTime);

    if (deltaTime > 0 && deltaTime < 1.0f)
    {
        UpdateCameraMovement(deltaTime);
        _engine.Tick(deltaTime);
        _engine.Render();
    }
}
```

**WASD fly camera:**

Controls are active while the right mouse button is held:

| Key | Action |
|-----|--------|
| W / S | Forward / Backward |
| A / D | Left / Right |
| Q / E | Down / Up |
| Left Shift | Speed multiplier (2x) |
| Right Mouse Drag | Look around (yaw/pitch) |

Camera constants: `CameraSpeed = 5.0f`, `MouseSensitivity = 0.2f`, pitch clamped to [-89, 89] degrees.

**Object picking:**

Left-click in the viewport performs ray-based object picking:

1. Convert mouse position to NDC coordinates
2. Unproject through inverse projection and view matrices
3. Call `EngineInterop.Raycast()` against the active scene
4. Match the returned actor pointer to an `ActorViewModel` by name
5. Update `SceneViewModel.SelectedActor` and `PropertiesViewModel.SelectedActor`

**Drag-drop from Content Browser:**

The viewport accepts drops with `AssetPath` data. When an asset is dropped, it spawns a new actor at a computed world position in front of the camera.

**Resize handling:**

When the viewport resizes, both the engine and the child HWND are updated:

```csharp
protected override void OnRenderSizeChanged(SizeChangedInfo sizeInfo)
{
    _engine.Resize((int)sizeInfo.NewSize.Width, (int)sizeInfo.NewSize.Height);
    if (_childHwnd != IntPtr.Zero && _parentHwnd != IntPtr.Zero)
    {
        MoveWindow(_childHwnd, clientPoint.X, clientPoint.Y,
            (int)sizeInfo.NewSize.Width, (int)sizeInfo.NewSize.Height, true);
    }
}
```

### 5.2 SceneHierarchyControl

**Location:** `Views/SceneHierarchyControl.xaml` / `Views/SceneHierarchyControl.xaml.cs`

Displays the list of actors in the active scene as a `TreeView`, bound to `SceneViewModel.Actors`.

**Features:**
- Actor tree with icon + name display via `HierarchicalDataTemplate`
- Context menu with **Add Actor**, **Add Empty**, and **Delete** commands
- Drag-and-drop reordering of actors using `DragDrop.DoDragDrop` with a threshold of 5 pixels
- Selection synchronization: selecting an item in the tree updates both `SceneViewModel.SelectedActor` and `PropertiesViewModel.SetSelectedActor`

**Drag reorder implementation:**

```csharp
private void TreeView_Drop(object sender, DragEventArgs e)
{
    var draggedActor = e.Data.GetData(typeof(ActorViewModel)) as ActorViewModel;
    var targetActor = targetItem.DataContext as ActorViewModel;

    if (DataContext is MainViewModel mainVm)
    {
        mainVm.SceneViewModel.ReorderActors(draggedActor, targetActor);
    }
}
```

### 5.3 PropertiesPanelControl

**Location:** `Views/PropertiesPanelControl.xaml` / `Views/PropertiesPanelControl.xaml.cs`

Displays properties of the currently selected actor using **pure XAML data binding** - no code-behind is needed for property display.

**Layout:**
- **Transform section:** Position (X/Y/Z), Rotation (X/Y/Z), Scale (X/Y/Z) with color-coded labels (red X, green Y, blue Z)
- **General section:** Name (editable TextBox), Visibility (CheckBox)
- **Components section:** Expandable list of `ComponentViewModel` items, each with a typed `DataTemplate`

**Typed DataTemplate pattern** for component inspectors:

```xml
<ContentControl Content="{Binding}">
    <ContentControl.Resources>
        <DataTemplate DataType="{x:Type ViewModels:StaticMeshComponentViewModel}">
            <StackPanel>
                <TextBox Text="{Binding MeshPath}" IsReadOnly="True"/>
                <CheckBox Content="Cast Shadows" IsChecked="{Binding CastShadows}"/>
            </StackPanel>
        </DataTemplate>
        <DataTemplate DataType="{x:Type ViewModels:LightComponentViewModel}">
            <StackPanel>
                <ComboBox SelectedIndex="{Binding LightType}">
                    <ComboBoxItem Content="Directional"/>
                    <ComboBoxItem Content="Point"/>
                    <ComboBoxItem Content="Spot"/>
                </ComboBox>
                <Slider Value="{Binding Intensity}" Minimum="0" Maximum="10"/>
            </StackPanel>
        </DataTemplate>
        <DataTemplate DataType="{x:Type ViewModels:CameraComponentViewModel}">
            <!-- FOV, near/far planes, perspective toggle -->
        </DataTemplate>
    </ContentControl.Resources>
</ContentControl>
```

WPF automatically selects the correct `DataTemplate` based on the runtime type of the bound `ComponentViewModel`, providing type-specific editors without any code-behind conditionals.

### 5.4 MaterialEditorControl

**Location:** `Views/MaterialEditorControl.xaml` / `Views/MaterialEditorControl.xaml.cs`

Provides PBR material property editing with real-time sync to the engine via `PropertiesViewModel.Material`.

**PBR Properties exposed:**

| Property | Range | Control |
|----------|-------|---------|
| Albedo Color (R/G/B/A) | 0.0 - 1.0 | TextBox with `StringFormat=F3` |
| Metallic | 0.0 - 1.0 | Slider + value display |
| Roughness | 0.0 - 1.0 | Slider + value display |
| Ambient Occlusion | 0.0 - 1.0 | Slider + value display |
| Emissive Intensity | 0.0 - 10.0 | Slider + value display |

**Texture map slots:** Albedo, Normal, Metallic, Roughness, AO - each with a read-only path TextBox and a "..." browse button that opens a file dialog filtered to image formats.

**7 Material Presets:**

| Preset | Albedo (R, G, B) | Metallic | Roughness |
|--------|-------------------|----------|-----------|
| Default | (0.80, 0.80, 0.80) | 0.0 | 0.5 |
| Metal | (0.95, 0.95, 0.95) | 1.0 | 0.2 |
| Plastic | (0.50, 0.50, 0.50) | 0.0 | 0.4 |
| Rubber | (0.20, 0.20, 0.20) | 0.0 | 0.9 |
| Gold | (1.00, 0.766, 0.336) | 1.0 | 0.3 |
| Silver | (0.972, 0.960, 0.915) | 1.0 | 0.3 |
| Copper | (0.955, 0.637, 0.538) | 1.0 | 0.35 |

**Material sync to engine:**

`PropertiesViewModel` subscribes to `MaterialViewModel.PropertyChanged` and pushes changes to the native engine:

```csharp
private void OnMaterialPropertyChanged(object? sender, PropertyChangedEventArgs e)
{
    if (_syncingFromEngine) return;  // prevent feedback loops

    switch (e.PropertyName)
    {
        case nameof(MaterialViewModel.Metallic):
            _engine.SetMaterialMetallic(_currentMaterialPtr, _material.Metallic);
            break;
        case nameof(MaterialViewModel.Roughness):
            _engine.SetMaterialRoughness(_currentMaterialPtr, _material.Roughness);
            break;
        // ...
    }
}
```

A `_syncingFromEngine` flag prevents feedback loops when material properties are being populated from the engine (e.g., when switching actor selection).

### 5.5 ContentBrowserControl

**Location:** `Views/ContentBrowserControl.xaml` / `Views/ContentBrowserControl.xaml.cs`

Asset management panel at the bottom of the editor window.

**Layout:**
- **Breadcrumb bar:** Shows current folder path with navigation
- **Folder tree** (left, 200px): Hierarchical tree of asset folders (Assets, Models, Textures, Materials, Scenes)
- **Asset grid** (right): WrapPanel-based grid of assets with icons/thumbnails

**Default folder structure:**

| Folder | Path |
|--------|------|
| Assets | `Assets/` |
| Models | `Assets/Models/` |
| Textures | `Assets/Textures/` |
| Materials | `Assets/Materials/` |
| Scenes | `Assets/Scenes/` |

**Features:**
- **File import:** Opens a dialog supporting `*.fbx;*.obj;*.gltf;*.png;*.jpg;*.tga` files, copies to the current folder
- **Thumbnails:** Image files (`.png`, `.jpg`, `.jpeg`, `.bmp`, `.tga`, `.gif`) are decoded to 64x64 `BitmapImage` thumbnails
- **File type icons:** Extension-based icon assignment using emoji (e.g., `📦` for models, `🖼` for textures, `🎨` for materials)
- **Drag-drop to viewport:** Assets can be dragged to the viewport to spawn actors. Uses `DataObject` with `AssetPath`, `AssetName`, `AssetExtension` data
- **Double-click navigation:** Double-clicking a folder navigates into it

**Asset item data model:**

```csharp
public class AssetItem
{
    public string Name { get; set; }
    public string Path { get; set; }
    public string Extension { get; set; }
    public string IconText { get; set; }
    public BitmapImage? ThumbnailImage { get; set; }
    public bool HasThumbnail => ThumbnailImage != null;
}
```

---

## 6. UndoRedoService

**Location:** `Services/UndoRedoService.cs`

Implements the **Command pattern** for undo/redo functionality. Uses two stacks with a configurable max history size (default 100).

### IUndoableAction Interface

```csharp
public interface IUndoableAction
{
    string Description { get; }
    void Undo();
    void Redo();
}
```

### UndoRedoService

```csharp
public class UndoRedoService
{
    private readonly Stack<IUndoableAction> _undoStack = new();
    private readonly Stack<IUndoableAction> _redoStack = new();
    private readonly int _maxHistorySize;  // default: 100

    public bool CanUndo => _undoStack.Count > 0;
    public bool CanRedo => _redoStack.Count > 0;
    public event EventHandler? StateChanged;

    public void ExecuteAction(IUndoableAction action)
    {
        action.Redo();
        _undoStack.Push(action);
        _redoStack.Clear();
        TrimHistory();
        OnStateChanged();
    }

    public void Undo() { /* pop from undo, push to redo */ }
    public void Redo() { /* pop from redo, push to undo */ }
}
```

### Built-in Action Types

#### SetPropertyAction\<T\>

Records a property change for undo/redo. Used for transform edits (position, rotation, scale):

```csharp
public class SetPropertyAction<T> : IUndoableAction
{
    private readonly T _oldValue;
    private readonly T _newValue;
    private readonly Action<T> _setter;

    public string Description => $"Set {_propertyName}";

    public SetPropertyAction(object target, string propertyName,
        T oldValue, T newValue, Action<T> setter)
    {
        _oldValue = oldValue;
        _newValue = newValue;
        _setter = setter;
    }

    public void Undo() => _setter(_oldValue);
    public void Redo() => _setter(_newValue);
}
```

Usage in `SceneViewModel` for position tracking:

```csharp
_undoRedo.ExecuteAction(new SetPropertyAction<(float, float, float)>(
    actor, "Position", (oldX, oldY, oldZ), (newX, newY, newZ),
    v => { actor.PositionX = v.Item1; actor.PositionY = v.Item2; actor.PositionZ = v.Item3; }));
```

#### AddItemAction\<T\>

Records adding an item to a collection. On undo, removes the item; on redo, inserts at the original index:

```csharp
public class AddItemAction<T> : IUndoableAction
{
    private readonly IList<T> _collection;
    private readonly T _item;
    private readonly int _index;

    public void Undo() { _collection.Remove(_item); }
    public void Redo()
    {
        if (_index >= 0 && _index <= _collection.Count)
            _collection.Insert(_index, _item);
        else
            _collection.Add(_item);
    }
}
```

#### RemoveItemAction\<T\>

Records removing an item from a collection. On undo, re-inserts at the original position; on redo, removes again:

```csharp
public class RemoveItemAction<T> : IUndoableAction
{
    private readonly IList<T> _collection;
    private readonly T _item;
    private int _originalIndex;

    public void Undo()
    {
        if (_originalIndex >= 0 && _originalIndex <= _collection.Count)
            _collection.Insert(_originalIndex, _item);
    }

    public void Redo() { _collection.Remove(_item); }
}
```

#### CompositeAction

Groups multiple actions into a single undo/redo step. Undo runs actions in reverse order; redo runs forward:

```csharp
public class CompositeAction : IUndoableAction
{
    private readonly List<IUndoableAction> _actions;

    public CompositeAction(string description, params IUndoableAction[] actions)
    {
        _description = description;
        _actions = new List<IUndoableAction>(actions);
    }

    public void Undo()
    {
        for (int i = _actions.Count - 1; i >= 0; i--)
            _actions[i].Undo();
    }

    public void Redo()
    {
        foreach (var action in _actions)
            action.Redo();
    }
}
```

### ICommand Adapters

`UndoCommand` and `RedoCommand` are `ICommand` implementations that delegate to the service and auto-wire `CanExecuteChanged` to `StateChanged`:

```csharp
public class UndoCommand : ICommand
{
    public event EventHandler? CanExecuteChanged
    {
        add { _service.StateChanged += value; }
        remove { _service.StateChanged -= value; }
    }
    public bool CanExecute(object? parameter) => _service.CanUndo;
    public void Execute(object? parameter) => _service.Undo();
}
```

### Integration with SceneViewModel

`SceneViewModel` uses `BeginUndoRedoOperation()` / `EndUndoRedoOperation()` flags to prevent undo actions from being recorded during undo/redo playback (which would cause infinite recursion):

```csharp
private void Undo()
{
    _viewModel.SceneViewModel.BeginUndoRedoOperation();
    _undoRedo.Undo();
    _viewModel.SceneViewModel.EndUndoRedoOperation();
}
```

---

## 7. Editor Features

### Menu Bar

| Menu | Items | Shortcut |
|------|-------|----------|
| **File** | New Scene, Open Scene, Save Scene, Exit | `Ctrl+N`, `Ctrl+O`, `Ctrl+S` |
| **Edit** | Undo, Redo, Delete | `Ctrl+Z`, `Ctrl+Y`, `Delete` |
| **View** | Scene Hierarchy (toggle), Properties (toggle), Content Browser (toggle) | - |
| **Help** | About | - |

### Toolbar

**Transform modes:**

| Button | Shortcut | Description |
|--------|----------|-------------|
| Select | Q | Selection mode |
| Move | W | Translate objects |
| Rotate | E | Rotate objects |
| Scale | R | Scale objects |

Transform mode is tracked in `MainViewModel.TransformMode` (`ETransformMode` enum).

**Simulation controls:**

| Button | Description |
|--------|-------------|
| Play | Start/resume simulation |
| Pause | Pause simulation |
| Stop | Stop simulation and refresh scene from engine |

### Status Bar

Displays real-time engine statistics, updated every 500ms via a `DispatcherTimer`:

- **Ready status:** Current operation/mode text
- **Draw Calls:** Number of draw calls per frame
- **FPS:** Frames per second calculated from engine frame time
- **Vertices:** Total vertex count per frame

```csharp
_statsTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(500) };
_statsTimer.Tick += (s, e) =>
{
    var (drawCalls, vertexCount, frameTime) = _engine.GetRenderStats();
    float fps = frameTime > 0.0001f ? (1000.0f / frameTime) : 0;
    StatusDrawCalls.Text = $"Draw Calls: {drawCalls}";
    StatusFPS.Text = $"FPS: {fps:F0} | Verts: {vertexCount}";
};
```

### Keyboard Shortcuts (in code-behind)

```csharp
InputBindings.Add(new KeyBinding(new RelayCommand(_ => Undo()),
    new KeyGesture(Key.Z, ModifierKeys.Control)));
InputBindings.Add(new KeyBinding(new RelayCommand(_ => Redo()),
    new KeyGesture(Key.Y, ModifierKeys.Control)));
InputBindings.Add(new KeyBinding(new RelayCommand(_ => DeleteSelected()),
    new KeyGesture(Key.Delete)));
```

### Window Layout

```
┌──────────────────────────────────────────────────────────┐
│ Menu Bar                                                   │
├──────────────────────────────────────────────────────────┤
│ Toolbar [Select][Move][Rotate][Scale] │ [▶][⏸][⏹]       │
├──────────┬──────────────────────┬────────────────────────┤
│ Scene    │                      │ Properties             │
│ Hierarchy│   3D Viewport        │   ──────────────────── │
│          │                      │   Material Editor      │
│          │                      │                        │
├──────────┴──────────────────────┴────────────────────────┤
│ Content Browser  (Folder Tree │ Asset Grid)              │
├──────────────────────────────────────────────────────────┤
│ Status Bar: Ready │ Draw Calls: 0 │ FPS: 0               │
└──────────────────────────────────────────────────────────┘
```

Column widths: Scene Hierarchy = 250px, Properties = 300px, Content Browser height = 200px.

---

## 8. Build Instructions

### Prerequisites

- Visual Studio 2022 with `.NET 8.0 SDK` and `C++ Desktop Development` workload
- The native `EngineInterop.dll` must be built first (C++ project)

### Building the C# Editor

```bash
dotnet build Editor/KojeomEditor/KojeomEditor.csproj
dotnet build Editor/KojeomEditor/KojeomEditor.csproj -c Release
```

### Building the Native DLL

```bash
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64 /t:EngineInterop
msbuild KojeomEngine.sln /p:Configuration=Release /p:Platform=x64 /t:EngineInterop
```

### Native DLL Copy Mechanism

The C# project includes an MSBuild target that automatically copies the native DLL after each build:

```xml
<Target Name="CopyNativeDlls" AfterTargets="Build">
    <PropertyGroup>
        <NativeDllDir>$(ProjectDir)..\EngineInterop\bin\x64\$(Configuration)\</NativeDllDir>
    </PropertyGroup>
    <Copy SourceFiles="$(NativeDllDir)EngineInterop.dll"
          DestinationFolder="$(OutputPath)"
          Condition="Exists('$(NativeDllDir)EngineInterop.dll')"
          SkipUnchangedFiles="true" />
    <Copy SourceFiles="$(NativeDllDir)EngineInterop.pdb"
          DestinationFolder="$(OutputPath)"
          Condition="Exists('$(NativeDllDir)EngineInterop.pdb')"
          SkipUnchangedFiles="true" />
</Target>
```

Source: `Editor/EngineInterop/bin/x64/<Configuration>/EngineInterop.dll`
Destination: `Editor/KojeomEditor/bin/x64/<Configuration>/net8.0-windows/`

The `SkipUnchangedFiles="true"` flag ensures the copy only runs when the DLL has changed, avoiding unnecessary file operations.

### Build Order

1. Build the Engine static library (`Engine` project)
2. Build the EngineInterop DLL (`EngineInterop` project) - depends on Engine
3. Build the KojeomEditor (`KojeomEditor` project) - automatically copies EngineInterop.dll

Or build the entire solution at once:

```bash
msbuild KojeomEngine.sln /p:Configuration=Debug /p:Platform=x64
```

### Running the Editor

After building, the editor executable is located at:

```
Editor/KojeomEditor/bin/x64/Debug/net8.0-windows/KojeomEditor.exe
```

The `EngineInterop.dll` must be in the same directory (copied automatically by the build target).

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using KojeomEditor.Services;
using KojeomEditor.ViewModels;

namespace KojeomEditor.Views;

public partial class ViewportControl : UserControl
{
    private EngineInterop? _engine;
    private HwndSource? _hwndSource;
    private Stopwatch? _stopwatch;
    private bool _isRendering;
    private double _lastTime;

    private bool _isRightMouseDown;
    private Point _lastMousePosition;
    private float _cameraYaw;
    private float _cameraPitch;
    private const float CameraSpeed = 5.0f;
    private const float MouseSensitivity = 0.2f;

    private readonly HashSet<Key> _pressedKeys = new();

    [DllImport("user32.dll", SetLastError = true)]
    private static extern IntPtr CreateWindowEx(
        uint dwExStyle, string lpClassName, string lpWindowName,
        uint dwStyle, int x, int y, int nWidth, int nHeight,
        IntPtr hWndParent, IntPtr hMenu, IntPtr hInstance, IntPtr lpParam);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern bool DestroyWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    private static extern IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);

    [DllImport("user32.dll")]
    private static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [DllImport("user32.dll")]
    private static extern bool MoveWindow(IntPtr hWnd, int X, int Y, int nWidth, int nHeight, bool bRepaint);

    [DllImport("user32.dll")]
    private static extern bool ScreenToClient(IntPtr hWnd, ref POINT lpPoint);

    [StructLayout(LayoutKind.Sequential)]
    private struct POINT
    {
        public int X;
        public int Y;
        public POINT(int x, int y) { X = x; Y = y; }
    }

    private const int WS_CHILD = 0x40000000;
    private const int WS_VISIBLE = 0x10000000;
    private const int SW_SHOW = 5;
    private IntPtr _childHwnd = IntPtr.Zero;
    private IntPtr _parentHwnd = IntPtr.Zero;

    public static readonly DependencyProperty EngineProperty =
        DependencyProperty.Register(nameof(Engine), typeof(EngineInterop), typeof(ViewportControl),
            new PropertyMetadata(null, OnEngineChanged));

    public static readonly DependencyProperty SceneViewModelProperty =
        DependencyProperty.Register(nameof(SceneViewModel), typeof(SceneViewModel), typeof(ViewportControl),
            new PropertyMetadata(null));

    public static readonly DependencyProperty PropertiesViewModelProperty =
        DependencyProperty.Register(nameof(PropertiesViewModel), typeof(PropertiesViewModel), typeof(ViewportControl),
            new PropertyMetadata(null));

    public EngineInterop? Engine
    {
        get => (EngineInterop?)GetValue(EngineProperty);
        set => SetValue(EngineProperty, value);
    }

    public SceneViewModel? SceneViewModel
    {
        get => (SceneViewModel?)GetValue(SceneViewModelProperty);
        set => SetValue(SceneViewModelProperty, value);
    }

    public PropertiesViewModel? PropertiesViewModel
    {
        get => (PropertiesViewModel?)GetValue(PropertiesViewModelProperty);
        set => SetValue(PropertiesViewModelProperty, value);
    }

    public event EventHandler<ActorViewModel?>? ActorSelected;

    public ViewportControl()
    {
        InitializeComponent();
        Loaded += OnLoaded;
        Unloaded += OnUnloaded;
        Focusable = true;
        KeyDown += OnKeyDown;
        KeyUp += OnKeyUp;
        MouseDown += OnMouseDown;
        MouseUp += OnMouseUp;
        MouseMove += OnMouseMove;
        MouseLeave += OnMouseLeave;
        AllowDrop = true;
        Drop += OnDrop;
        DragOver += OnDragOver;
    }

    private static void OnEngineChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is ViewportControl control)
        {
            control._engine = e.NewValue as EngineInterop;
        }
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        _stopwatch = Stopwatch.StartNew();
        InitializeHwndHost();
        Keyboard.Focus(this);
    }

    private void OnUnloaded(object sender, RoutedEventArgs e)
    {
        StopRendering();
        if (_childHwnd != IntPtr.Zero)
        {
            DestroyWindow(_childHwnd);
            _childHwnd = IntPtr.Zero;
        }
        _hwndSource?.Dispose();
        _hwndSource = null;
    }

    private void InitializeHwndHost()
    {
        var contentPresenter = new ContentPresenter
        {
            Name = "ViewportHost"
        };
        Content = contentPresenter;

        contentPresenter.Loaded += (s, e) =>
        {
            if (contentPresenter.IsLoaded && _engine != null && !_engine.IsInitialized)
            {
                var parentWindow = Window.GetWindow(this);
                if (parentWindow == null) return;

                _hwndSource = PresentationSource.FromVisual(this) as HwndSource;
                _parentHwnd = _hwndSource?.Handle ?? IntPtr.Zero;
                if (_parentHwnd == IntPtr.Zero) return;

                int width = (int)ActualWidth;
                int height = (int)ActualHeight;
                if (width <= 0 || height <= 0) return;

                var screenPoint = PointToScreen(new Point(0, 0));
                var clientPoint = new POINT((int)screenPoint.X, (int)screenPoint.Y);
                ScreenToClient(_parentHwnd, ref clientPoint);

                _childHwnd = CreateWindowEx(
                    0, "Static", "KojeomViewport",
                    WS_CHILD | WS_VISIBLE,
                    clientPoint.X, clientPoint.Y, width, height,
                    _parentHwnd, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero);

                if (_childHwnd != IntPtr.Zero)
                {
                    _engine.InitializeEmbedded(_childHwnd, width, height);
                    _engine.DebugRendererSetEnabled(true);
                    StartRendering();
                }
            }
        };
    }

    private System.Windows.Rect GetViewportRect()
    {
        var point = PointToScreen(new Point(0, 0));
        return new System.Windows.Rect(point.X, point.Y, ActualWidth, ActualHeight);
    }

    private void StartRendering()
    {
        if (_isRendering)
        {
            return;
        }

        _isRendering = true;
        _lastTime = 0;
        CompositionTarget.Rendering += OnRendering;
    }

    private void StopRendering()
    {
        if (!_isRendering)
        {
            return;
        }

        _isRendering = false;
        CompositionTarget.Rendering -= OnRendering;
    }

    private void OnRendering(object? sender, EventArgs e)
    {
        if (!_isRendering || _engine == null || !_engine.IsInitialized)
        {
            return;
        }

        var currentTime = _stopwatch?.Elapsed.TotalSeconds ?? 0;
        var deltaTime = (float)(currentTime - _lastTime);
        _lastTime = currentTime;

        if (deltaTime > 0 && deltaTime < 1.0f)
        {
            UpdateCameraMovement(deltaTime);
            if (SceneViewModel == null || !SceneViewModel.IsEnginePaused)
            {
                _engine.Tick(deltaTime);
            }
            _engine.DebugRendererDrawGrid(0, 0, 0, 40.0f, 2.0f, 10);
            _engine.DebugRendererDrawAxis(0, 0.01f, 0, 2.0f);
            _engine.Render();
            _engine.DebugRendererRenderFrame(deltaTime);
        }
    }

    private void UpdateCameraMovement(float deltaTime)
    {
        if (_engine == null || !_isRightMouseDown) return;

        var (x, y, z) = _engine.GetCameraPosition();
        
        float forwardX = (float)Math.Sin(_cameraYaw * Math.PI / 180.0);
        float forwardZ = (float)Math.Cos(_cameraYaw * Math.PI / 180.0);
        float rightX = (float)Math.Cos(_cameraYaw * Math.PI / 180.0);
        float rightZ = -(float)Math.Sin(_cameraYaw * Math.PI / 180.0);

        float speed = CameraSpeed * deltaTime;

        if (_pressedKeys.Contains(Key.LeftShift))
        {
            speed *= 2.0f;
        }

        if (_pressedKeys.Contains(Key.W))
        {
            x += forwardX * speed;
            z += forwardZ * speed;
        }
        if (_pressedKeys.Contains(Key.S))
        {
            x -= forwardX * speed;
            z -= forwardZ * speed;
        }
        if (_pressedKeys.Contains(Key.A))
        {
            x -= rightX * speed;
            z -= rightZ * speed;
        }
        if (_pressedKeys.Contains(Key.D))
        {
            x += rightX * speed;
            z += rightZ * speed;
        }
        if (_pressedKeys.Contains(Key.Q))
        {
            y -= speed;
        }
        if (_pressedKeys.Contains(Key.E))
        {
            y += speed;
        }

        _engine.SetCameraPosition(x, y, z);
        _engine.SetCameraRotation(_cameraPitch, _cameraYaw, 0);
    }

    private void OnKeyDown(object sender, KeyEventArgs e)
    {
        _pressedKeys.Add(e.Key);
        e.Handled = true;
    }

    private void OnKeyUp(object sender, KeyEventArgs e)
    {
        _pressedKeys.Remove(e.Key);
        e.Handled = true;
    }

    private void OnMouseDown(object sender, MouseButtonEventArgs e)
    {
        if (e.RightButton == MouseButtonState.Pressed)
        {
            _isRightMouseDown = true;
            _lastMousePosition = e.GetPosition(this);
            CaptureMouse();
            Keyboard.Focus(this);
        }
        else if (e.LeftButton == MouseButtonState.Pressed && !_isRightMouseDown)
        {
            PerformObjectPicking(e.GetPosition(this));
        }
    }

    private void OnMouseUp(object sender, MouseButtonEventArgs e)
    {
        if (e.RightButton == MouseButtonState.Released)
        {
            _isRightMouseDown = false;
            ReleaseMouseCapture();
        }
    }

    private void OnMouseMove(object sender, MouseEventArgs e)
    {
        if (!_isRightMouseDown) return;

        var currentPosition = e.GetPosition(this);
        var delta = currentPosition - _lastMousePosition;
        _lastMousePosition = currentPosition;

        _cameraYaw += (float)(delta.X * MouseSensitivity);
        _cameraPitch += (float)(delta.Y * MouseSensitivity);
        _cameraPitch = Math.Clamp(_cameraPitch, -89.0f, 89.0f);
    }

    private void OnMouseLeave(object sender, MouseEventArgs e)
    {
        _isRightMouseDown = false;
        _pressedKeys.Clear();
        ReleaseMouseCapture();
    }

    private void PerformObjectPicking(Point mousePosition)
    {
        if (_engine == null || !_engine.IsInitialized) return;

        float ndcX = (float)(2.0 * mousePosition.X / ActualWidth - 1.0);
        float ndcY = (float)(1.0 - 2.0 * mousePosition.Y / ActualHeight);

        var (viewMatrix, projMatrix) = _engine.GetCameraMatrices();
        if (viewMatrix == null || projMatrix == null) return;

        var invProj = InvertMatrix4x4(projMatrix);
        var invView = InvertMatrix4x4(viewMatrix);

        float rayX = ndcX;
        float rayY = ndcY;
        float rayZ = 1.0f;

        float x = rayX * invProj[0] + rayY * invProj[4] + rayZ * invProj[8] + invProj[12];
        float y = rayX * invProj[1] + rayY * invProj[5] + rayZ * invProj[9] + invProj[13];
        float z = rayX * invProj[2] + rayY * invProj[6] + rayZ * invProj[10] + invProj[14];
        float w = rayX * invProj[3] + rayY * invProj[7] + rayZ * invProj[11] + invProj[15];
        
        if (Math.Abs(w) > 0.0001f)
        {
            x /= w; y /= w; z /= w;
        }

        float dirX = x * invView[0] + y * invView[4] + z * invView[8];
        float dirY = x * invView[1] + y * invView[5] + z * invView[9];
        float dirZ = x * invView[2] + y * invView[6] + z * invView[10];

        float length = (float)Math.Sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
        if (length > 0.0001f)
        {
            dirX /= length;
            dirY /= length;
            dirZ /= length;
        }

        var (camX, camY, camZ) = _engine.GetCameraPosition();

        var hitActor = _engine.Raycast(camX, camY, camZ, dirX, dirY, dirZ, out _, out _, out _);

        if (hitActor != IntPtr.Zero)
        {
            string? actorName = _engine.GetActorName(hitActor);
            var actorViewModel = FindActorViewModel(actorName);
            if (actorViewModel != null)
            {
                SelectActor(actorViewModel);
            }
        }
        else
        {
            SelectActor(null);
        }
    }

    private ActorViewModel? FindActorViewModel(string? name)
    {
        if (string.IsNullOrEmpty(name) || SceneViewModel == null) return null;

        foreach (var actor in SceneViewModel.Actors)
        {
            var found = FindActorViewModelRecursive(actor, name);
            if (found != null)
                return found;
        }
        return null;
    }

    private static ActorViewModel? FindActorViewModelRecursive(ActorViewModel actor, string name)
    {
        if (actor.Name == name)
            return actor;

        foreach (var child in actor.Children)
        {
            var found = FindActorViewModelRecursive(child, name);
            if (found != null)
                return found;
        }
        return null;
    }

    private void SelectActor(ActorViewModel? actor)
    {
        if (SceneViewModel != null)
        {
            SceneViewModel.SelectedActor = actor;
        }
        if (PropertiesViewModel != null)
        {
            PropertiesViewModel.SetSelectedActor(actor);
        }
        if (_engine != null && actor != null && actor.NativePtr != IntPtr.Zero)
        {
            _engine.SetActorPosition(actor.NativePtr, actor.PositionX, actor.PositionY, actor.PositionZ);
        }
        ActorSelected?.Invoke(this, actor);
    }

    private static float[] InvertMatrix4x4(float[] m)
    {
        float[] inv = new float[16];
        
        inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
        inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
        inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
        inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
        inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
        inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
        inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
        inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
        inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
        inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
        inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
        inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
        inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
        inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
        inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
        inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

        float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
        if (Math.Abs(det) < 0.0001f) return new float[16];

        det = 1.0f / det;
        for (int i = 0; i < 16; i++)
            inv[i] *= det;

        return inv;
    }

    private void OnDragOver(object sender, DragEventArgs e)
    {
        if (e.Data.GetDataPresent("AssetPath"))
        {
            e.Effects = DragDropEffects.Copy;
            e.Handled = true;
        }
        else
        {
            e.Effects = DragDropEffects.None;
        }
    }

    private void OnDrop(object sender, DragEventArgs e)
    {
        if (!e.Data.GetDataPresent("AssetPath")) return;
        
        var assetPath = e.Data.GetData("AssetPath") as string;
        var assetName = e.Data.GetData("AssetName") as string;
        var assetExt = e.Data.GetData("AssetExtension") as string;
        
        if (string.IsNullOrEmpty(assetPath) || string.IsNullOrEmpty(assetExt)) return;
        
        var dropPosition = e.GetPosition(this);
        SpawnActorFromAsset(assetPath, assetName ?? "Actor", assetExt, dropPosition);
    }

    private void SpawnActorFromAsset(string assetPath, string assetName, string assetExt, Point dropPosition)
    {
        if (_engine == null || !_engine.IsInitialized || SceneViewModel == null) return;
        
        float ndcX = (float)(2.0 * dropPosition.X / ActualWidth - 1.0);
        float ndcY = (float)(1.0 - 2.0 * dropPosition.Y / ActualHeight);
        
        var (camX, camY, camZ) = _engine.GetCameraPosition();
        float spawnDistance = 5.0f;
        
        float spawnX = camX + (float)(ndcX * spawnDistance);
        float spawnY = camY;
        float spawnZ = camZ + spawnDistance;
        
        var scene = _engine.GetActiveScene();
        if (scene == IntPtr.Zero) return;
        
        var actorPtr = _engine.CreateActor(scene, assetName);
        if (actorPtr != IntPtr.Zero)
        {
            _engine.SetActorPosition(actorPtr, spawnX, spawnY, spawnZ);
            
            string lowerExt = assetExt.ToLower();
            if (lowerExt is ".fbx" or ".obj" or ".gltf" or ".glb" or ".dae" or ".3ds")
            {
                var componentPtr = _engine.AddComponent(actorPtr, NativeComponentType.StaticMesh);
                if (componentPtr != IntPtr.Zero)
                {
                    _engine.SetMesh(componentPtr, assetPath);
                }
            }
        }
        
        var newActor = new ActorViewModel
        {
            Name = assetName,
            NativePtr = actorPtr,
            PositionX = spawnX,
            PositionY = spawnY,
            PositionZ = spawnZ
        };
        
        SceneViewModel.Actors.Add(newActor);
        SceneViewModel.SelectedActor = newActor;
        
        if (PropertiesViewModel != null)
        {
            PropertiesViewModel.SetSelectedActor(newActor);
        }
    }

    private static string GetActorTypeFromExtension(string ext)
    {
        return ext.ToLower() switch
        {
            ".fbx" or ".obj" or ".gltf" or ".glb" => "StaticMesh",
            ".png" or ".jpg" or ".jpeg" or ".tga" => "Texture",
            ".mat" or ".material" => "Material",
            ".scene" => "Scene",
            _ => "Actor"
        };
    }

    protected override void OnRenderSizeChanged(SizeChangedInfo sizeInfo)
    {
        base.OnRenderSizeChanged(sizeInfo);

        if (_engine?.IsInitialized == true && sizeInfo.NewSize.Width > 0 && sizeInfo.NewSize.Height > 0)
        {
            _engine.Resize((int)sizeInfo.NewSize.Width, (int)sizeInfo.NewSize.Height);
            if (_childHwnd != IntPtr.Zero && _parentHwnd != IntPtr.Zero)
            {
                var screenPoint = PointToScreen(new Point(0, 0));
                var clientPoint = new POINT((int)screenPoint.X, (int)screenPoint.Y);
                ScreenToClient(_parentHwnd, ref clientPoint);
                MoveWindow(_childHwnd, clientPoint.X, clientPoint.Y, (int)sizeInfo.NewSize.Width, (int)sizeInfo.NewSize.Height, true);
            }
        }
    }
}

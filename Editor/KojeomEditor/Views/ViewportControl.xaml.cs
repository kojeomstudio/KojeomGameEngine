using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using KojeomEditor.Services;

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

    public static readonly DependencyProperty EngineProperty =
        DependencyProperty.Register(nameof(Engine), typeof(EngineInterop), typeof(ViewportControl),
            new PropertyMetadata(null, OnEngineChanged));

    public EngineInterop? Engine
    {
        get => (EngineInterop?)GetValue(EngineProperty);
        set => SetValue(EngineProperty, value);
    }

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
            if (contentPresenter.IsLoaded)
            {
                var helper = new WindowInteropHelper(Window.GetWindow(this));
                if (helper.Handle != IntPtr.Zero && _engine != null && !_engine.IsInitialized)
                {
                    var rect = GetViewportRect();
                    _engine.Initialize(helper.Handle, (int)rect.Width, (int)rect.Height);
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
            _engine.Tick(deltaTime);
            _engine.Render();
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
        if (_pressedKeys.Contains(Key.LeftShift))
        {
            speed *= 2.0f;
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

    protected override void OnRenderSizeChanged(SizeChangedInfo sizeInfo)
    {
        base.OnRenderSizeChanged(sizeInfo);

        if (_engine?.IsInitialized == true && sizeInfo.NewSize.Width > 0 && sizeInfo.NewSize.Height > 0)
        {
            _engine.Resize((int)sizeInfo.NewSize.Width, (int)sizeInfo.NewSize.Height);
        }
    }
}

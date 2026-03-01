using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
using KojeomEditor.Services;

namespace KojeomEditor.Views;

public partial class ViewportControl : UserControl
{
    private EngineInterop? _engine;
    private HwndSource? _hwndSource;
    private Stopwatch? _stopwatch;
    private bool _isRendering;
    private double _lastTime;

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
            _engine.Tick(deltaTime);
            _engine.Render();
        }
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

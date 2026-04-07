using System;
using System.ComponentModel;
using System.Windows;
using System.Windows.Input;
using KojeomEditor.Services;
using KojeomEditor.ViewModels;

namespace KojeomEditor;

public partial class MainWindow : Window
{
    private readonly EngineInterop _engine;
    private readonly MainViewModel _viewModel;
    private System.Windows.Threading.DispatcherTimer _statsTimer;

    public MainWindow()
    {
        InitializeComponent();
        _viewModel = new MainViewModel();
        DataContext = _viewModel;
        _engine = new EngineInterop();
        _viewModel.Engine = _engine;
        _viewModel.PropertiesViewModel.Engine = _engine;
        _engine.LogMessage += OnEngineLogMessage;

        Viewport.Engine = _engine;
        Viewport.SceneViewModel = _viewModel.SceneViewModel;
        Viewport.PropertiesViewModel = _viewModel.PropertiesViewModel;

        _viewModel.SceneViewModel.PropertyChanged += SceneViewModel_PropertyChanged;
        Viewport.ActorSelected += Viewport_ActorSelected;

        MaterialEditor.MaterialViewModel = _viewModel.PropertiesViewModel.Material;

        RendererSettings.Engine = _engine;

        Loaded += OnWindowLoaded;
        Closed += OnWindowClosed;

        _statsTimer = new System.Windows.Threading.DispatcherTimer
        {
            Interval = TimeSpan.FromMilliseconds(500)
        };
        _statsTimer.Tick += UpdateStatusBarStats;
        _statsTimer.Start();

        InputBindings.Add(new KeyBinding(new RelayCommand(_ => Undo()), new KeyGesture(Key.Z, ModifierKeys.Control)));
        InputBindings.Add(new KeyBinding(new RelayCommand(_ => Redo()), new KeyGesture(Key.Y, ModifierKeys.Control)));
        InputBindings.Add(new KeyBinding(new RelayCommand(_ => DeleteSelected()), new KeyGesture(Key.Delete)));
    }

    private void OnWindowLoaded(object sender, RoutedEventArgs e)
    {
        _viewModel.SceneViewModel.Initialize();
        _viewModel.SceneViewModel.UndoRedo = _undoRedo;
        Viewport.Focus();
    }

    private void OnWindowClosed(object? sender, EventArgs e)
    {
        _statsTimer?.Stop();
        _engine.Dispose();
    }

    private void SceneViewModel_PropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(SceneViewModel.SelectedActor))
        {
            _viewModel.PropertiesViewModel.SetSelectedActor(_viewModel.SceneViewModel.SelectedActor);
        }
    }

    private void Viewport_ActorSelected(object? sender, ActorViewModel? actor)
    {
        _viewModel.SceneViewModel.SelectedActor = actor;
        _viewModel.PropertiesViewModel.SetSelectedActor(actor);
    }

    private void UpdateStatusBarStats(object? sender, EventArgs e)
    {
        if (_engine.IsInitialized)
        {
            var (drawCalls, vertexCount, frameTime) = _engine.GetRenderStats();
            StatusDrawCalls.Text = $"Draw Calls: {drawCalls}";
            float fps = frameTime > 0.0001f ? (1000.0f / frameTime) : 0;
            StatusFPS.Text = $"FPS: {fps:F0} | Verts: {vertexCount}";
        }
    }

    private void OnEngineLogMessage(object? sender, string message)
    {
        Dispatcher.Invoke(() =>
        {
            StatusReady.Text = message;
        });
    }

    private readonly UndoRedoService _undoRedo = new();

    private void Undo()
    {
        _viewModel.SceneViewModel.BeginUndoRedoOperation();
        _undoRedo.Undo();
        _viewModel.SceneViewModel.EndUndoRedoOperation();
    }

    private void Redo()
    {
        _viewModel.SceneViewModel.BeginUndoRedoOperation();
        _undoRedo.Redo();
        _viewModel.SceneViewModel.EndUndoRedoOperation();
    }

    private void DeleteSelected()
    {
        if (_viewModel.SceneViewModel.SelectedActor != null)
        {
            _viewModel.SceneViewModel.RemoveActor(_viewModel.SceneViewModel.SelectedActor);
        }
    }

    private void MenuItem_Exit_Click(object sender, RoutedEventArgs e)
    {
        Application.Current.Shutdown();
    }

    private void MenuItem_NewScene_Click(object sender, RoutedEventArgs e)
    {
        _viewModel.NewScene();
    }

    private void MenuItem_OpenScene_Click(object sender, RoutedEventArgs e)
    {
        _viewModel.OpenScene();
    }

    private void MenuItem_SaveScene_Click(object sender, RoutedEventArgs e)
    {
        _viewModel.SaveScene();
    }

    private void MenuItem_Undo_Click(object sender, RoutedEventArgs e) => Undo();
    private void MenuItem_Redo_Click(object sender, RoutedEventArgs e) => Redo();
    private void MenuItem_Delete_Click(object sender, RoutedEventArgs e) => DeleteSelected();

    private void Toolbar_Select_Click(object sender, RoutedEventArgs e)
    {
        _viewModel.TransformMode = ETransformMode.Select;
        StatusReady.Text = "Mode: Select";
    }

    private void Toolbar_Move_Click(object sender, RoutedEventArgs e)
    {
        _viewModel.TransformMode = ETransformMode.Move;
        StatusReady.Text = "Mode: Move";
    }

    private void Toolbar_Rotate_Click(object sender, RoutedEventArgs e)
    {
        _viewModel.TransformMode = ETransformMode.Rotate;
        StatusReady.Text = "Mode: Rotate";
    }

    private void Toolbar_Scale_Click(object sender, RoutedEventArgs e)
    {
        _viewModel.TransformMode = ETransformMode.Scale;
        StatusReady.Text = "Mode: Scale";
    }

    private bool _isPlaying;
    private bool _isPaused;

    private void Toolbar_Play_Click(object sender, RoutedEventArgs e)
    {
        if (!_isPlaying)
        {
            _isPlaying = true;
            _isPaused = false;
            _viewModel.SceneViewModel.IsEnginePaused = false;
            StatusReady.Text = "Playing...";
            StatusBar.Background = new System.Windows.Media.SolidColorBrush(
                (System.Windows.Media.Color)System.Windows.Media.ColorConverter.ConvertFromString("#4CAF50"));
        }
        else if (_isPaused)
        {
            _isPaused = false;
            _viewModel.SceneViewModel.IsEnginePaused = false;
            StatusReady.Text = "Playing...";
            StatusBar.Background = new System.Windows.Media.SolidColorBrush(
                (System.Windows.Media.Color)System.Windows.Media.ColorConverter.ConvertFromString("#4CAF50"));
        }
    }

    private void Toolbar_Pause_Click(object sender, RoutedEventArgs e)
    {
        if (_isPlaying && !_isPaused)
        {
            _isPaused = true;
            _viewModel.SceneViewModel.IsEnginePaused = true;
            StatusReady.Text = "Paused";
            StatusBar.Background = new System.Windows.Media.SolidColorBrush(
                (System.Windows.Media.Color)System.Windows.Media.ColorConverter.ConvertFromString("#FF9800"));
        }
    }

    private void Toolbar_Stop_Click(object sender, RoutedEventArgs e)
    {
        if (_isPlaying)
        {
            _isPlaying = false;
            _isPaused = false;
            _viewModel.SceneViewModel.IsEnginePaused = false;
            StatusReady.Text = "Ready";
            StatusBar.Background = new System.Windows.Media.SolidColorBrush(
                (System.Windows.Media.Color)System.Windows.Media.ColorConverter.ConvertFromString("#007ACC"));
            if (_viewModel.SceneViewModel.Engine != null && _viewModel.SceneViewModel.Engine.IsInitialized)
            {
                _viewModel.SceneViewModel.RefreshFromEngine();
            }
        }
    }

    private void MenuItem_ToggleSceneHierarchy_Click(object sender, RoutedEventArgs e)
    {
        SceneHierarchyPanel.Visibility = SceneHierarchyPanel.Visibility == Visibility.Collapsed ? Visibility.Visible : Visibility.Collapsed;
    }

    private void MenuItem_ToggleProperties_Click(object sender, RoutedEventArgs e)
    {
        PropertiesPanel.Visibility = PropertiesPanel.Visibility == Visibility.Collapsed ? Visibility.Visible : Visibility.Collapsed;
    }

    private void MenuItem_ToggleContentBrowser_Click(object sender, RoutedEventArgs e)
    {
        ContentBrowserPanel.Visibility = ContentBrowserPanel.Visibility == Visibility.Collapsed ? Visibility.Visible : Visibility.Collapsed;
    }

    private void MenuItem_About_Click(object sender, RoutedEventArgs e)
    {
        var message = "Kojeom Engine Editor\n\nVersion 1.0.0\n\nC++17 / DirectX 11 Game Engine\nC# / .NET 8.0 WPF Editor\n\nForward & Deferred Rendering\nPBR Materials • Skeletal Animation\nPhysics • Audio • UI System";
        MessageBox.Show(message, "About Kojeom Engine Editor", MessageBoxButton.OK, MessageBoxImage.Information);
    }
}

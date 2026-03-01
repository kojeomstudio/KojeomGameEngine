using System.Windows;
using KojeomEditor.Services;

namespace KojeomEditor;

public partial class MainWindow : Window
{
    private readonly EngineInterop _engine;

    public MainWindow()
    {
        InitializeComponent();
        DataContext = new ViewModels.MainViewModel();
        _engine = new EngineInterop();
        _engine.LogMessage += OnEngineLogMessage;
        
        Loaded += OnWindowLoaded;
        Closed += OnWindowClosed;
    }

    private void OnWindowLoaded(object sender, RoutedEventArgs e)
    {
        if (FindName("ViewportControl") is Views.ViewportControl viewport)
        {
            viewport.Engine = _engine;
        }
    }

    private void OnWindowClosed(object? sender, System.EventArgs e)
    {
        _engine.Dispose();
    }

    private void OnEngineLogMessage(object? sender, string message)
    {
        Dispatcher.Invoke(() =>
        {
            if (FindName("StatusBarText") is System.Windows.Controls.TextBlock statusText)
            {
                statusText.Text = message;
            }
        });
    }

    private void MenuItem_Exit_Click(object sender, RoutedEventArgs e)
    {
        Application.Current.Shutdown();
    }

    private void MenuItem_NewScene_Click(object sender, RoutedEventArgs e)
    {
        if (DataContext is ViewModels.MainViewModel vm)
        {
            vm.NewScene();
        }
    }

    private void MenuItem_OpenScene_Click(object sender, RoutedEventArgs e)
    {
        if (DataContext is ViewModels.MainViewModel vm)
        {
            vm.OpenScene();
        }
    }

    private void MenuItem_SaveScene_Click(object sender, RoutedEventArgs e)
    {
        if (DataContext is ViewModels.MainViewModel vm)
        {
            vm.SaveScene();
        }
    }
}

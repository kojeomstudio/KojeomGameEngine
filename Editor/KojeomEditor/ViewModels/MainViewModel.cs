using System.Collections.ObjectModel;
using System.Windows.Input;
using KojeomEditor.Services;

namespace KojeomEditor.ViewModels;

public enum ETransformMode
{
    Select,
    Move,
    Rotate,
    Scale
}

public class MainViewModel : ViewModelBase
{
    private SceneViewModel _sceneViewModel;
    private PropertiesViewModel _propertiesViewModel;
    private EngineInterop? _engine;
    private ETransformMode _transformMode = ETransformMode.Select;

    public SceneViewModel SceneViewModel => _sceneViewModel;
    public PropertiesViewModel PropertiesViewModel => _propertiesViewModel;

    public ETransformMode TransformMode
    {
        get => _transformMode;
        set
        {
            _transformMode = value;
            OnPropertyChanged(nameof(TransformMode));
        }
    }

    public EngineInterop? Engine
    {
        get => _engine;
        set
        {
            _engine = value;
            _sceneViewModel.Engine = value;
            _propertiesViewModel.Engine = value;
        }
    }

    public MainViewModel()
    {
        _sceneViewModel = new SceneViewModel();
        _propertiesViewModel = new PropertiesViewModel();
    }

    public void NewScene()
    {
        _sceneViewModel.CreateNewScene();
    }

    public void OpenScene()
    {
        var dialog = new Microsoft.Win32.OpenFileDialog
        {
            Filter = "Scene Files (*.scene)|*.scene|All Files (*.*)|*.*",
            Title = "Open Scene"
        };

        if (dialog.ShowDialog() == true)
        {
            var fullPath = System.IO.Path.GetFullPath(dialog.FileName);
            var projectRoot = GetProjectRoot();
            if (!IsPathWithinDirectory(fullPath, projectRoot))
            {
                System.Windows.MessageBox.Show("Scene file must be within the project directory.", "Security Warning",
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Warning);
                return;
            }
            _sceneViewModel.LoadScene(dialog.FileName);
        }
    }

    public void SaveScene()
    {
        _sceneViewModel.SaveScene();
    }

    public void SaveSceneAs()
    {
        _sceneViewModel.SaveSceneAs();
    }

    public void LoadSceneValidated(string scenePath)
    {
        if (string.IsNullOrEmpty(scenePath)) return;

        var fullPath = System.IO.Path.GetFullPath(scenePath);
        var projectRoot = GetProjectRoot();
        if (!IsPathWithinDirectory(fullPath, projectRoot))
        {
            System.Windows.MessageBox.Show("Scene file must be within the project directory.", "Security Warning",
                System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Warning);
            return;
        }
        _sceneViewModel.LoadScene(scenePath);
    }

    public static string GetProjectRoot()
    {
        var dir = AppDomain.CurrentDomain.BaseDirectory;
        for (int i = 0; i < 6 && dir != null; i++)
        {
            var test = System.IO.Path.Combine(dir, "KojeomEngine.sln");
            if (System.IO.File.Exists(test))
                return dir;
            dir = System.IO.Path.GetDirectoryName(dir);
        }
        return AppDomain.CurrentDomain.BaseDirectory;
    }

    public static bool IsPathWithinDirectory(string path, string directory)
    {
        var normalizedDir = directory.TrimEnd(System.IO.Path.DirectorySeparatorChar,
            System.IO.Path.AltDirectorySeparatorChar) + System.IO.Path.DirectorySeparatorChar;
        return path.StartsWith(normalizedDir, StringComparison.OrdinalIgnoreCase)
            || string.Equals(path, directory.TrimEnd(System.IO.Path.DirectorySeparatorChar,
                System.IO.Path.AltDirectorySeparatorChar), StringComparison.OrdinalIgnoreCase);
    }
}

public class ViewModelBase : System.ComponentModel.INotifyPropertyChanged
{
    public event System.ComponentModel.PropertyChangedEventHandler? PropertyChanged;

    protected void OnPropertyChanged(string propertyName)
    {
        PropertyChanged?.Invoke(this, new System.ComponentModel.PropertyChangedEventArgs(propertyName));
    }
}

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

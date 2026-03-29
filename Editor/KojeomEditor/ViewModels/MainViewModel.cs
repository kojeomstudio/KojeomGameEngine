using System.Collections.ObjectModel;
using System.Windows.Input;
using KojeomEditor.Services;

namespace KojeomEditor.ViewModels;

public class MainViewModel : ViewModelBase
{
    private SceneViewModel _sceneViewModel;
    private PropertiesViewModel _propertiesViewModel;

    public SceneViewModel SceneViewModel => _sceneViewModel;
    public PropertiesViewModel PropertiesViewModel => _propertiesViewModel;

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
            _sceneViewModel.LoadScene(dialog.FileName);
        }
    }

    public void SaveScene()
    {
        var dialog = new Microsoft.Win32.SaveFileDialog
        {
            Filter = "Scene Files (*.scene)|*.scene|All Files (*.*)|*.*",
            Title = "Save Scene"
        };

        if (dialog.ShowDialog() == true)
        {
            _sceneViewModel.SaveScene(dialog.FileName);
        }
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

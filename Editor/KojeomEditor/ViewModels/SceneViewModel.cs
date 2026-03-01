using System.Collections.ObjectModel;

namespace KojeomEditor.ViewModels;

public class SceneViewModel : ViewModelBase
{
    private string _sceneName = "Untitled Scene";
    private ActorViewModel? _selectedActor;

    public string SceneName
    {
        get => _sceneName;
        set { _sceneName = value; OnPropertyChanged(nameof(SceneName)); }
    }

    public ObservableCollection<ActorViewModel> Actors { get; } = new();

    public ActorViewModel? SelectedActor
    {
        get => _selectedActor;
        set
        {
            _selectedActor = value;
            OnPropertyChanged(nameof(SelectedActor));
        }
    }

    public SceneViewModel()
    {
        CreateNewScene();
    }

    public void CreateNewScene()
    {
        Actors.Clear();
        SceneName = "Untitled Scene";
        
        Actors.Add(new ActorViewModel { Name = "Directional Light", ActorType = "Light" });
        Actors.Add(new ActorViewModel { Name = "Main Camera", ActorType = "Camera" });
    }

    public void LoadScene(string path)
    {
        // TODO: Implement actual scene loading
        CreateNewScene();
        SceneName = System.IO.Path.GetFileNameWithoutExtension(path);
    }

    public void SaveScene(string path)
    {
        // TODO: Implement actual scene saving
    }

    public void AddActor(string name, string type = "Actor")
    {
        Actors.Add(new ActorViewModel { Name = name, ActorType = type });
    }

    public void RemoveActor(ActorViewModel actor)
    {
        Actors.Remove(actor);
        if (SelectedActor == actor)
        {
            SelectedActor = null;
        }
    }
}

public class ActorViewModel : ViewModelBase
{
    private string _name = "Actor";
    private string _actorType = "Actor";
    private float _positionX, _positionY, _positionZ;
    private float _rotationX, _rotationY, _rotationZ;
    private float _scaleX = 1, _scaleY = 1, _scaleZ = 1;
    private bool _isVisible = true;

    public string Name
    {
        get => _name;
        set { _name = value; OnPropertyChanged(nameof(Name)); }
    }

    public string ActorType
    {
        get => _actorType;
        set { _actorType = value; OnPropertyChanged(nameof(ActorType)); }
    }

    public float PositionX
    {
        get => _positionX;
        set { _positionX = value; OnPropertyChanged(nameof(PositionX)); }
    }

    public float PositionY
    {
        get => _positionY;
        set { _positionY = value; OnPropertyChanged(nameof(PositionY)); }
    }

    public float PositionZ
    {
        get => _positionZ;
        set { _positionZ = value; OnPropertyChanged(nameof(PositionZ)); }
    }

    public float RotationX
    {
        get => _rotationX;
        set { _rotationX = value; OnPropertyChanged(nameof(RotationX)); }
    }

    public float RotationY
    {
        get => _rotationY;
        set { _rotationY = value; OnPropertyChanged(nameof(RotationY)); }
    }

    public float RotationZ
    {
        get => _rotationZ;
        set { _rotationZ = value; OnPropertyChanged(nameof(RotationZ)); }
    }

    public float ScaleX
    {
        get => _scaleX;
        set { _scaleX = value; OnPropertyChanged(nameof(ScaleX)); }
    }

    public float ScaleY
    {
        get => _scaleY;
        set { _scaleY = value; OnPropertyChanged(nameof(ScaleY)); }
    }

    public float ScaleZ
    {
        get => _scaleZ;
        set { _scaleZ = value; OnPropertyChanged(nameof(ScaleZ)); }
    }

    public bool IsVisible
    {
        get => _isVisible;
        set { _isVisible = value; OnPropertyChanged(nameof(IsVisible)); }
    }
}

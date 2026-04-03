using System.Collections.ObjectModel;
using KojeomEditor.Services;

namespace KojeomEditor.ViewModels;

public class SceneViewModel : ViewModelBase
{
    private string _sceneName = "Untitled Scene";
    private ActorViewModel? _selectedActor;
    private EngineInterop? _engine;
    private IntPtr _activeScenePtr = IntPtr.Zero;

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
            if (_selectedActor != value)
            {
                if (_selectedActor != null)
                {
                    _selectedActor.PropertyChanged -= OnActorPropertyChanged;
                }
                _selectedActor = value;
                if (_selectedActor != null)
                {
                    _selectedActor.PropertyChanged += OnActorPropertyChanged;
                }
                OnPropertyChanged(nameof(SelectedActor));
            }
        }
    }

    public EngineInterop? Engine
    {
        get => _engine;
        set { _engine = value; }
    }

    public IntPtr ActiveScenePtr => _activeScenePtr;

    public SceneViewModel()
    {
    }

    public void Initialize()
    {
        CreateNewScene();
    }

    public void CreateNewScene()
    {
        Actors.Clear();
        SceneName = "Untitled Scene";
        SelectedActor = null;

        if (_engine != null && _engine.IsInitialized)
        {
            var scenePtr = _engine.CreateScene(SceneName);
            if (scenePtr != IntPtr.Zero)
            {
                _engine.SetActiveScene(scenePtr);
                _activeScenePtr = scenePtr;
            }
        }
    }

    public void LoadScene(string path)
    {
        Actors.Clear();
        SceneName = System.IO.Path.GetFileNameWithoutExtension(path);
        SelectedActor = null;

        if (_engine != null && _engine.IsInitialized)
        {
            var scenePtr = _engine.LoadScene(path);
            if (scenePtr != IntPtr.Zero)
            {
                _engine.SetActiveScene(scenePtr);
                _activeScenePtr = scenePtr;
                RefreshFromEngine();
            }
        }
    }

    public void SaveScene(string path)
    {
        if (_engine != null && _engine.IsInitialized)
        {
            var scenePtr = _engine.GetActiveScene();
            if (scenePtr != IntPtr.Zero)
            {
                _engine.SaveScene(path, scenePtr);
            }
        }
    }

    public void AddActor(string name, string type = "Actor")
    {
        IntPtr nativePtr = IntPtr.Zero;
        if (_engine != null && _engine.IsInitialized && _activeScenePtr != IntPtr.Zero)
        {
            nativePtr = _engine.CreateActor(_activeScenePtr, name);
        }

        var actor = new ActorViewModel { Name = name, ActorType = type, NativePtr = nativePtr };
        if (type == "StaticMesh" && nativePtr != IntPtr.Zero)
        {
            actor.AddComponent(new StaticMeshComponentViewModel());
            var compPtr = _engine!.AddComponent(nativePtr, 2);
            if (compPtr != IntPtr.Zero)
            {
                _engine!.CreateDefaultMesh(compPtr);
            }
        }
        else if (type == "SkeletalMesh" && nativePtr != IntPtr.Zero)
        {
            var compPtr = _engine!.AddComponent(nativePtr, 3);
        }

        Actors.Add(actor);
        SelectedActor = actor;
    }

    public void RemoveActor(ActorViewModel actor)
    {
        if (_engine != null && _engine.IsInitialized && actor.NativePtr != IntPtr.Zero && _activeScenePtr != IntPtr.Zero)
        {
            _engine.DestroyActor(_activeScenePtr, actor.NativePtr);
        }
        Actors.Remove(actor);
        if (SelectedActor == actor)
        {
            SelectedActor = null;
        }
    }

    public void ReorderActors(ActorViewModel draggedActor, ActorViewModel targetActor)
    {
        int draggedIndex = Actors.IndexOf(draggedActor);
        int targetIndex = Actors.IndexOf(targetActor);

        if (draggedIndex < 0 || targetIndex < 0)
            return;

        Actors.RemoveAt(draggedIndex);
        Actors.Insert(targetIndex, draggedActor);
    }

    public void RefreshFromEngine()
    {
        if (_engine == null || !_engine.IsInitialized) return;

        if (_selectedActor != null)
        {
            _selectedActor.PropertyChanged -= OnActorPropertyChanged;
        }

        Actors.Clear();

        int actorCount = _engine.GetActorCount();
        for (int i = 0; i < actorCount; i++)
        {
            var actorPtr = _engine.GetActorAt(i);
            if (actorPtr == IntPtr.Zero) continue;

            string? name = _engine.GetActorName(actorPtr);
            var vm = new ActorViewModel
            {
                Name = name ?? $"Actor_{i}",
                NativePtr = actorPtr
            };

            var (px, py, pz) = _engine.GetActorPosition(actorPtr);
            vm.PositionX = px;
            vm.PositionY = py;
            vm.PositionZ = pz;

            _engine.GetActorRotation(actorPtr, out float rx, out float ry, out float rz, out float rw);
            vm.RotationX = rx;
            vm.RotationY = ry;
            vm.RotationZ = rz;

            _engine.GetActorScale(actorPtr, out float sx, out float sy, out float sz);
            vm.ScaleX = sx;
            vm.ScaleY = sy;
            vm.ScaleZ = sz;

            vm.IsVisible = _engine.IsActorVisible(actorPtr);

            int compCount = _engine.GetComponentCount(actorPtr);
            for (int j = 0; j < compCount; j++)
            {
                string? compName = _engine.GetComponentName(actorPtr, j);
                int compType = _engine.GetComponentType(actorPtr, j);
                vm.AddComponent(new ComponentViewModel
                {
                    Name = compName ?? "Component",
                    ComponentType = (EComponentType)compType
                });
            }

            Actors.Add(vm);
        }

        if (_selectedActor != null)
        {
            _selectedActor.PropertyChanged += OnActorPropertyChanged;
        }
    }

    private void OnActorPropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
    {
        if (_engine == null || !_engine.IsInitialized) return;
        if (sender is not ActorViewModel actor || actor.NativePtr == IntPtr.Zero) return;

        switch (e.PropertyName)
        {
            case nameof(ActorViewModel.PositionX):
            case nameof(ActorViewModel.PositionY):
            case nameof(ActorViewModel.PositionZ):
                _engine.SetActorPosition(actor.NativePtr, actor.PositionX, actor.PositionY, actor.PositionZ);
                break;
            case nameof(ActorViewModel.RotationX):
            case nameof(ActorViewModel.RotationY):
            case nameof(ActorViewModel.RotationZ):
                _engine.SetActorRotation(actor.NativePtr, actor.RotationX, actor.RotationY, actor.RotationZ, 0);
                break;
            case nameof(ActorViewModel.ScaleX):
            case nameof(ActorViewModel.ScaleY):
            case nameof(ActorViewModel.ScaleZ):
                _engine.SetActorScale(actor.NativePtr, actor.ScaleX, actor.ScaleY, actor.ScaleZ);
                break;
            case nameof(ActorViewModel.Name):
                _engine.SetActorName(actor.NativePtr, actor.Name);
                break;
            case nameof(ActorViewModel.IsVisible):
                _engine.SetActorVisibility(actor.NativePtr, actor.IsVisible);
                break;
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
    private IntPtr _nativePtr = IntPtr.Zero;

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

    public IntPtr NativePtr
    {
        get => _nativePtr;
        set { _nativePtr = value; OnPropertyChanged(nameof(NativePtr)); }
    }

    public ObservableCollection<ComponentViewModel> Components { get; } = new();

    public ActorViewModel()
    {
        Components.Add(new ComponentViewModel { Name = "Transform", ComponentType = EComponentType.Transform });
    }

    public void AddComponent(ComponentViewModel component)
    {
        Components.Add(component);
    }

    public void RemoveComponent(ComponentViewModel component)
    {
        Components.Remove(component);
    }
}

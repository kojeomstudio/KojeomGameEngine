using System.Collections.ObjectModel;
using KojeomEditor.Services;

namespace KojeomEditor.ViewModels;

public class SceneViewModel : ViewModelBase
{
    private string _sceneName = "Untitled Scene";
    private ActorViewModel? _selectedActor;
    private EngineInterop? _engine;

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
                _selectedActor = value;
                OnPropertyChanged(nameof(SelectedActor));
                SyncActorSelectionToEngine();
            }
        }
    }

    public EngineInterop? Engine
    {
        get => _engine;
        set { _engine = value; }
    }

    public SceneViewModel()
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
            }
        }

        var dirLight = new ActorViewModel { Name = "Directional Light", ActorType = "Light" };
        dirLight.AddComponent(new LightComponentViewModel { LightType = 0, Intensity = 1.0f });
        Actors.Add(dirLight);

        var mainCam = new ActorViewModel { Name = "Main Camera", ActorType = "Camera" };
        mainCam.AddComponent(new CameraComponentViewModel());
        Actors.Add(mainCam);
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
        var actor = new ActorViewModel { Name = name, ActorType = type };
        if (type == "StaticMesh")
        {
            actor.AddComponent(new StaticMeshComponentViewModel());
        }
        Actors.Add(actor);
        SelectedActor = actor;
    }

    public void RemoveActor(ActorViewModel actor)
    {
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
    }

    private void SyncActorSelectionToEngine()
    {
        if (_engine == null || !_engine.IsInitialized) return;
        if (SelectedActor == null || SelectedActor.NativePtr == IntPtr.Zero) return;
        _engine.SetActorPosition(SelectedActor.NativePtr, SelectedActor.PositionX, SelectedActor.PositionY, SelectedActor.PositionZ);
        _engine.SetActorScale(SelectedActor.NativePtr, SelectedActor.ScaleX, SelectedActor.ScaleY, SelectedActor.ScaleZ);
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

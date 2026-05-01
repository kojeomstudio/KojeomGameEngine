using System.Collections.ObjectModel;
using System.Linq;
using KojeomEditor.Services;

namespace KojeomEditor.ViewModels;

public class SceneViewModel : ViewModelBase
{
    private string _sceneName = "Untitled Scene";
    private string? _currentScenePath;
    private ActorViewModel? _selectedActor;
    private EngineInterop? _engine;
    private IntPtr _activeScenePtr = IntPtr.Zero;
    private UndoRedoService? _undoRedo;
    private bool _isUndoRedoInProgress;
    private bool _isEnginePaused;

    public event System.ComponentModel.PropertyChangedEventHandler? ActorChanged;

    public bool IsEnginePaused
    {
        get => _isEnginePaused;
        set { _isEnginePaused = value; OnPropertyChanged(nameof(IsEnginePaused)); }
    }

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

    public UndoRedoService? UndoRedo
    {
        get => _undoRedo;
        set { _undoRedo = value; }
    }

    public void BeginUndoRedoOperation() { _isUndoRedoInProgress = true; }
    public void EndUndoRedoOperation() { _isUndoRedoInProgress = false; }

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
        _currentScenePath = null;
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
        _currentScenePath = path;
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

    public void SaveScene(string? path = null)
    {
        if (path == null)
            path = _currentScenePath;

        if (path == null)
        {
            SaveSceneAs();
            return;
        }

        if (_engine != null && _engine.IsInitialized)
        {
            var scenePtr = _engine.GetActiveScene();
            if (scenePtr != IntPtr.Zero)
            {
                bool success = _engine.SaveScene(path, scenePtr);
                if (success)
                {
                    _currentScenePath = path;
                    SceneName = System.IO.Path.GetFileNameWithoutExtension(path);
                }
            }
        }
    }

    public void SaveSceneAs()
    {
        var dialog = new Microsoft.Win32.SaveFileDialog
        {
            Filter = "Scene Files (*.scene)|*.scene|All Files (*.*)|*.*",
            Title = "Save Scene As",
            FileName = SceneName
        };

        if (dialog.ShowDialog() == true)
        {
            var fullPath = System.IO.Path.GetFullPath(dialog.FileName);
            var projectRoot = MainViewModel.GetProjectRoot();
            if (!MainViewModel.IsPathWithinDirectory(fullPath, projectRoot))
            {
                System.Windows.MessageBox.Show("Scene file must be saved within the project directory.", "Security Warning",
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Warning);
                return;
            }
            SaveScene(dialog.FileName);
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
            var compPtr = _engine!.AddComponent(nativePtr, NativeComponentType.StaticMesh);
            if (compPtr != IntPtr.Zero)
            {
                _engine!.CreateDefaultMesh(compPtr);
            }
        }
        else if (type == "SkeletalMesh" && nativePtr != IntPtr.Zero)
        {
            actor.AddComponent(new ComponentViewModel { Name = "SkeletalMesh", ComponentType = EComponentType.SkeletalMesh });
            var compPtr = _engine!.AddComponent(nativePtr, NativeComponentType.SkeletalMesh);
        }
        else if (type == "Light" && nativePtr != IntPtr.Zero)
        {
            actor.AddComponent(new LightComponentViewModel());
            var compPtr = _engine!.AddComponent(nativePtr, NativeComponentType.Light);
        }

        if (_undoRedo != null)
        {
            _undoRedo.ExecuteAction(new AddItemAction<ActorViewModel>(Actors, actor));
        }
        else
        {
            Actors.Add(actor);
        }
        SelectedActor = actor;
        ActorChanged?.Invoke(this, new System.ComponentModel.PropertyChangedEventArgs("Actors"));
    }

    public void RemoveActor(ActorViewModel actor)
    {
        if (!Actors.Contains(actor)) return;

        if (_undoRedo != null)
        {
            _undoRedo.ExecuteAction(new RemoveItemAction<ActorViewModel>(Actors, actor));
        }
        else
        {
            Actors.Remove(actor);
        }

        if (SelectedActor == actor)
        {
            SelectedActor = null;
        }
        ActorChanged?.Invoke(this, new System.ComponentModel.PropertyChangedEventArgs("Actors"));
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

        IntPtr selectedPtr = _selectedActor?.NativePtr ?? IntPtr.Zero;
        if (_selectedActor != null)
        {
            _selectedActor.PropertyChanged -= OnActorPropertyChanged;
        }

        _selectedActor = null;
        Actors.Clear();

        var allViewModels = new List<ActorViewModel>();

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
            vm.RotationW = rw;

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

            allViewModels.Add(vm);
        }

        var vmByPtr = new Dictionary<IntPtr, ActorViewModel>();
        foreach (var vm in allViewModels)
        {
            vmByPtr[vm.NativePtr] = vm;
        }

        foreach (var vm in allViewModels)
        {
            var parentPtr = _engine.GetParent(vm.NativePtr);
            if (parentPtr != IntPtr.Zero && vmByPtr.TryGetValue(parentPtr, out var parentVm))
            {
                vm.Parent = parentVm;
                parentVm.Children.Add(vm);
            }
            else
            {
                Actors.Add(vm);
            }
        }

        if (selectedPtr != IntPtr.Zero)
        {
            var found = allViewModels.FirstOrDefault(vm => vm.NativePtr == selectedPtr);
            if (found != null)
            {
                SelectedActor = found;
            }
        }
    }

    public void MakeParentChild(ActorViewModel parent, ActorViewModel child)
    {
        if (_engine == null || !_engine.IsInitialized) return;
        if (parent.NativePtr == IntPtr.Zero || child.NativePtr == IntPtr.Zero) return;

        bool success = _engine.AddChild(parent.NativePtr, child.NativePtr);
        if (!success) return;

        Actors.Remove(child);
        child.Parent = parent;
        parent.Children.Add(child);
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
            {
                if (!_isUndoRedoInProgress && _undoRedo != null)
                {
                    var (oldX, oldY, oldZ) = _engine.GetActorPosition(actor.NativePtr);
                    var newValues = (actor.PositionX, actor.PositionY, actor.PositionZ);
                    if ((oldX, oldY, oldZ) != newValues)
                    {
                        _engine.SetActorPosition(actor.NativePtr, newValues.Item1, newValues.Item2, newValues.Item3);
                        _undoRedo.ExecuteAction(new SetPropertyAction<(float, float, float)>(
                            actor, "Position", (oldX, oldY, oldZ), newValues,
                            v => { actor.PositionX = v.Item1; actor.PositionY = v.Item2; actor.PositionZ = v.Item3; }));
                        break;
                    }
                }
                _engine.SetActorPosition(actor.NativePtr, actor.PositionX, actor.PositionY, actor.PositionZ);
                break;
            }
            case nameof(ActorViewModel.RotationX):
            case nameof(ActorViewModel.RotationY):
            case nameof(ActorViewModel.RotationZ):
            {
                if (!_isUndoRedoInProgress && _undoRedo != null)
                {
                    _engine.GetActorRotation(actor.NativePtr, out float oldRx, out float oldRy, out float oldRz, out _);
                    var newValues = (actor.RotationX, actor.RotationY, actor.RotationZ);
                    if ((oldRx, oldRy, oldRz) != newValues)
                    {
                        _engine.SetActorRotation(actor.NativePtr, newValues.Item1, newValues.Item2, newValues.Item3, actor.RotationW);
                        _undoRedo.ExecuteAction(new SetPropertyAction<(float, float, float)>(
                            actor, "Rotation", (oldRx, oldRy, oldRz), newValues,
                            v => { actor.RotationX = v.Item1; actor.RotationY = v.Item2; actor.RotationZ = v.Item3; }));
                        break;
                    }
                }
                _engine.SetActorRotation(actor.NativePtr, actor.RotationX, actor.RotationY, actor.RotationZ, actor.RotationW);
                break;
            }
            case nameof(ActorViewModel.ScaleX):
            case nameof(ActorViewModel.ScaleY):
            case nameof(ActorViewModel.ScaleZ):
            {
                if (!_isUndoRedoInProgress && _undoRedo != null)
                {
                    _engine.GetActorScale(actor.NativePtr, out float oldSx, out float oldSy, out float oldSz);
                    var newValues = (actor.ScaleX, actor.ScaleY, actor.ScaleZ);
                    if ((oldSx, oldSy, oldSz) != newValues)
                    {
                        _engine.SetActorScale(actor.NativePtr, newValues.Item1, newValues.Item2, newValues.Item3);
                        _undoRedo.ExecuteAction(new SetPropertyAction<(float, float, float)>(
                            actor, "Scale", (oldSx, oldSy, oldSz), newValues,
                            v => { actor.ScaleX = v.Item1; actor.ScaleY = v.Item2; actor.ScaleZ = v.Item3; }));
                        break;
                    }
                }
                _engine.SetActorScale(actor.NativePtr, actor.ScaleX, actor.ScaleY, actor.ScaleZ);
                break;
            }
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
    private float _rotationX, _rotationY, _rotationZ, _rotationW = 1.0f;
    private float _scaleX = 1, _scaleY = 1, _scaleZ = 1;
    private bool _isVisible = true;
    private IntPtr _nativePtr = IntPtr.Zero;

    public ObservableCollection<ActorViewModel> Children { get; } = new();
    public ActorViewModel? Parent { get; set; }
    public bool HasChildren => Children.Count > 0;

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

    public float RotationW
    {
        get => _rotationW;
        set { _rotationW = value; OnPropertyChanged(nameof(RotationW)); }
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

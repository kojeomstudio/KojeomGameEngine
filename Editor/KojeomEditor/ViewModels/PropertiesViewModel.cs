using System.ComponentModel;
using KojeomEditor.Services;

namespace KojeomEditor.ViewModels;

public class PropertiesViewModel : ViewModelBase
{
    private ActorViewModel? _selectedActor;
    private EngineInterop? _engine;
    private MaterialViewModel _material = new();
    private LightComponentViewModel _light = new();
    private IntPtr _currentMaterialPtr = IntPtr.Zero;
    private bool _syncingFromEngine;

    public ActorViewModel? SelectedActor
    {
        get => _selectedActor;
        set
        {
            _selectedActor = value;
            OnPropertyChanged(nameof(SelectedActor));
            OnPropertyChanged(nameof(HasSelection));
            SyncMaterialFromEngine();
            SyncLightFromEngine();
        }
    }

    public bool HasSelection => SelectedActor != null;

    public EngineInterop? Engine
    {
        get => _engine;
        set { _engine = value; }
    }

    public MaterialViewModel Material => _material;

    public LightComponentViewModel Light => _light;

    public PropertiesViewModel()
    {
        _material.PropertyChanged += OnMaterialPropertyChanged;
        _light.PropertyChanged += OnLightPropertyChanged;
    }

    public void SetSelectedActor(ActorViewModel? actor)
    {
        SelectedActor = actor;
    }

    private IntPtr _currentComponentPtr = IntPtr.Zero;

    private void SyncMaterialFromEngine()
    {
        _material.PropertyChanged -= OnMaterialPropertyChanged;

        _currentMaterialPtr = IntPtr.Zero;
        _currentComponentPtr = IntPtr.Zero;

        if (_engine == null || !_engine.IsInitialized || _selectedActor == null || _selectedActor.NativePtr == IntPtr.Zero)
        {
            ResetMaterialDefaults();
            _material.PropertyChanged += OnMaterialPropertyChanged;
            return;
        }

        var componentPtr = _engine.GetActorStaticMeshComponent(_selectedActor.NativePtr);
        if (componentPtr == IntPtr.Zero)
        {
            ResetMaterialDefaults();
            _material.PropertyChanged += OnMaterialPropertyChanged;
            return;
        }

        _currentComponentPtr = componentPtr;

        var materialPtr = _engine.GetStaticMeshComponentMaterial(componentPtr);
        if (materialPtr == IntPtr.Zero)
        {
            ResetMaterialDefaults();
            _material.PropertyChanged += OnMaterialPropertyChanged;
            return;
        }

        _currentMaterialPtr = materialPtr;

        _syncingFromEngine = true;
        var (r, g, b, a) = _engine.GetMaterialAlbedo(materialPtr);
        _material.AlbedoR = r;
        _material.AlbedoG = g;
        _material.AlbedoB = b;
        _material.AlbedoA = a;
        _material.Metallic = _engine.GetMaterialMetallic(materialPtr);
        _material.Roughness = _engine.GetMaterialRoughness(materialPtr);
        _material.AO = _engine.GetMaterialAO(materialPtr);
        _syncingFromEngine = false;

        _material.PropertyChanged += OnMaterialPropertyChanged;
    }

    private void ResetMaterialDefaults()
    {
        _syncingFromEngine = true;
        _material.AlbedoR = 1.0f;
        _material.AlbedoG = 1.0f;
        _material.AlbedoB = 1.0f;
        _material.AlbedoA = 1.0f;
        _material.Metallic = 0.0f;
        _material.Roughness = 0.5f;
        _material.AO = 1.0f;
        _material.EmissiveR = 0.0f;
        _material.EmissiveG = 0.0f;
        _material.EmissiveB = 0.0f;
        _material.EmissiveIntensity = 0.0f;
        _material.AlbedoTexturePath = string.Empty;
        _material.NormalTexturePath = string.Empty;
        _material.MetallicTexturePath = string.Empty;
        _material.RoughnessTexturePath = string.Empty;
        _material.AOTexturePath = string.Empty;
        _material.FilePath = null;
        _syncingFromEngine = false;
    }

    private void OnMaterialPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (_syncingFromEngine) return;
        if (_engine == null || !_engine.IsInitialized || _currentMaterialPtr == IntPtr.Zero) return;

        switch (e.PropertyName)
        {
            case nameof(MaterialViewModel.AlbedoR):
            case nameof(MaterialViewModel.AlbedoG):
            case nameof(MaterialViewModel.AlbedoB):
            case nameof(MaterialViewModel.AlbedoA):
                _engine.SetMaterialAlbedo(_currentMaterialPtr, _material.AlbedoR, _material.AlbedoG, _material.AlbedoB, _material.AlbedoA);
                break;
            case nameof(MaterialViewModel.Metallic):
                _engine.SetMaterialMetallic(_currentMaterialPtr, _material.Metallic);
                break;
            case nameof(MaterialViewModel.Roughness):
                _engine.SetMaterialRoughness(_currentMaterialPtr, _material.Roughness);
                break;
            case nameof(MaterialViewModel.AO):
                _engine.SetMaterialAO(_currentMaterialPtr, _material.AO);
                break;
            case nameof(MaterialViewModel.EmissiveR):
            case nameof(MaterialViewModel.EmissiveG):
            case nameof(MaterialViewModel.EmissiveB):
            case nameof(MaterialViewModel.EmissiveIntensity):
                _engine.SetMaterialEmissive(_currentMaterialPtr, _material.EmissiveR, _material.EmissiveG, _material.EmissiveB, _material.EmissiveIntensity);
                break;
            case nameof(MaterialViewModel.AlbedoTexturePath):
                _engine.SetMaterialTexture(_currentMaterialPtr, 0, _material.AlbedoTexturePath);
                break;
            case nameof(MaterialViewModel.NormalTexturePath):
                _engine.SetMaterialTexture(_currentMaterialPtr, 1, _material.NormalTexturePath);
                break;
            case nameof(MaterialViewModel.MetallicTexturePath):
                _engine.SetMaterialTexture(_currentMaterialPtr, 2, _material.MetallicTexturePath);
                break;
            case nameof(MaterialViewModel.RoughnessTexturePath):
                _engine.SetMaterialTexture(_currentMaterialPtr, 3, _material.RoughnessTexturePath);
                break;
            case nameof(MaterialViewModel.AOTexturePath):
                _engine.SetMaterialTexture(_currentMaterialPtr, 4, _material.AOTexturePath);
                break;
        }
    }

    private void SyncLightFromEngine()
    {
        _light.PropertyChanged -= OnLightPropertyChanged;

        if (_engine == null || !_engine.IsInitialized)
        {
            ResetLightDefaults();
            _light.PropertyChanged += OnLightPropertyChanged;
            return;
        }

        _syncingFromEngine = true;
        var (dirX, dirY, dirZ, colR, colG, colB, colA, ambR, ambG, ambB, ambA) = _engine.GetDirectionalLight();
        _light.ColorR = colR;
        _light.ColorG = colG;
        _light.ColorB = colB;
        _light.Intensity = _engine.GetDirectionalLightIntensity();
        _syncingFromEngine = false;

        _light.PropertyChanged += OnLightPropertyChanged;
    }

    private void ResetLightDefaults()
    {
        _syncingFromEngine = true;
        _light.LightType = 0;
        _light.Intensity = 1.0f;
        _light.ColorR = 1.0f;
        _light.ColorG = 1.0f;
        _light.ColorB = 1.0f;
        _light.Range = 10.0f;
        _syncingFromEngine = false;
    }

    private void OnLightPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (_syncingFromEngine) return;
        if (_engine == null || !_engine.IsInitialized) return;

        switch (e.PropertyName)
        {
            case nameof(LightComponentViewModel.Intensity):
                _engine.SetDirectionalLightIntensity(_light.Intensity);
                break;
            case nameof(LightComponentViewModel.ColorR):
            case nameof(LightComponentViewModel.ColorG):
            case nameof(LightComponentViewModel.ColorB):
                _engine.SetDirectionalLightColor(_light.ColorR, _light.ColorG, _light.ColorB, 1.0f);
                break;
        }
    }
}

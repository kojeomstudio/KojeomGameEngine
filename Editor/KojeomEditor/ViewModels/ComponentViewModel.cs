using System.Collections.ObjectModel;

namespace KojeomEditor.ViewModels;

public enum EComponentType
{
    Unknown = 0,
    Transform = 1,
    StaticMesh = 2,
    SkeletalMesh = 3,
    Light = 4,
    Camera = 5,
    Audio = 6,
    Physics = 7
}

public class ComponentViewModel : ViewModelBase
{
    private string _name = "Component";
    private EComponentType _componentType = EComponentType.Unknown;

    public string Name
    {
        get => _name;
        set { _name = value; OnPropertyChanged(nameof(Name)); }
    }

    public EComponentType ComponentType
    {
        get => _componentType;
        set { _componentType = value; OnPropertyChanged(nameof(ComponentType)); OnPropertyChanged(nameof(TypeDisplayName)); }
    }

    public string TypeDisplayName => ComponentType.ToString();
}

public class StaticMeshComponentViewModel : ComponentViewModel
{
    private string _meshPath = "";
    private string _materialPath = "";
    private bool _castShadows = true;
    private bool _receiveShadows = true;

    public StaticMeshComponentViewModel()
    {
        ComponentType = EComponentType.StaticMesh;
        Name = "StaticMesh";
    }

    public string MeshPath
    {
        get => _meshPath;
        set { _meshPath = value; OnPropertyChanged(nameof(MeshPath)); }
    }

    public string MaterialPath
    {
        get => _materialPath;
        set { _materialPath = value; OnPropertyChanged(nameof(MaterialPath)); }
    }

    public bool CastShadows
    {
        get => _castShadows;
        set { _castShadows = value; OnPropertyChanged(nameof(CastShadows)); }
    }

    public bool ReceiveShadows
    {
        get => _receiveShadows;
        set { _receiveShadows = value; OnPropertyChanged(nameof(ReceiveShadows)); }
    }
}

public class LightComponentViewModel : ComponentViewModel
{
    private int _lightType = 0;
    private float _intensity = 1.0f;
    private float _colorR = 1.0f;
    private float _colorG = 1.0f;
    private float _colorB = 1.0f;
    private float _range = 10.0f;
    private float _outerConeAngle = 45.0f;
    private float _innerConeAngle = 35.0f;
    private bool _castShadows = true;

    public LightComponentViewModel()
    {
        ComponentType = EComponentType.Light;
        Name = "Light";
    }

    public int LightType
    {
        get => _lightType;
        set { _lightType = value; OnPropertyChanged(nameof(LightType)); OnPropertyChanged(nameof(LightTypeName)); }
    }

    public string LightTypeName => LightType switch
    {
        0 => "Directional",
        1 => "Point",
        2 => "Spot",
        _ => "Unknown"
    };

    public float Intensity
    {
        get => _intensity;
        set { _intensity = value; OnPropertyChanged(nameof(Intensity)); }
    }

    public float ColorR
    {
        get => _colorR;
        set { _colorR = value; OnPropertyChanged(nameof(ColorR)); }
    }

    public float ColorG
    {
        get => _colorG;
        set { _colorG = value; OnPropertyChanged(nameof(ColorG)); }
    }

    public float ColorB
    {
        get => _colorB;
        set { _colorB = value; OnPropertyChanged(nameof(ColorB)); }
    }

    public float Range
    {
        get => _range;
        set { _range = value; OnPropertyChanged(nameof(Range)); }
    }

    public float OuterConeAngle
    {
        get => _outerConeAngle;
        set { _outerConeAngle = value; OnPropertyChanged(nameof(OuterConeAngle)); }
    }

    public float InnerConeAngle
    {
        get => _innerConeAngle;
        set { _innerConeAngle = value; OnPropertyChanged(nameof(InnerConeAngle)); }
    }

    public bool CastShadows
    {
        get => _castShadows;
        set { _castShadows = value; OnPropertyChanged(nameof(CastShadows)); }
    }
}

public class CameraComponentViewModel : ComponentViewModel
{
    private float _fieldOfView = 60.0f;
    private float _nearPlane = 0.1f;
    private float _farPlane = 1000.0f;
    private float _aspectRatio = 16.0f / 9.0f;
    private bool _isPerspective = true;

    public CameraComponentViewModel()
    {
        ComponentType = EComponentType.Camera;
        Name = "Camera";
    }

    public float FieldOfView
    {
        get => _fieldOfView;
        set { _fieldOfView = value; OnPropertyChanged(nameof(FieldOfView)); }
    }

    public float NearPlane
    {
        get => _nearPlane;
        set { _nearPlane = value; OnPropertyChanged(nameof(NearPlane)); }
    }

    public float FarPlane
    {
        get => _farPlane;
        set { _farPlane = value; OnPropertyChanged(nameof(FarPlane)); }
    }

    public float AspectRatio
    {
        get => _aspectRatio;
        set { _aspectRatio = value; OnPropertyChanged(nameof(AspectRatio)); }
    }

    public bool IsPerspective
    {
        get => _isPerspective;
        set { _isPerspective = value; OnPropertyChanged(nameof(IsPerspective)); }
    }
}

public class MaterialViewModel : ViewModelBase
{
    private string _name = "Material";
    private float _albedoR = 1.0f;
    private float _albedoG = 1.0f;
    private float _albedoB = 1.0f;
    private float _albedoA = 1.0f;
    private float _metallic = 0.0f;
    private float _roughness = 0.5f;
    private float _ao = 1.0f;
    private float _emissiveIntensity = 0.0f;
    private string _albedoTexturePath = "";
    private string _normalTexturePath = "";
    private string _metallicTexturePath = "";
    private string _roughnessTexturePath = "";
    private string _aoTexturePath = "";

    public string Name
    {
        get => _name;
        set { _name = value; OnPropertyChanged(nameof(Name)); }
    }

    public float AlbedoR
    {
        get => _albedoR;
        set { _albedoR = value; OnPropertyChanged(nameof(AlbedoR)); }
    }

    public float AlbedoG
    {
        get => _albedoG;
        set { _albedoG = value; OnPropertyChanged(nameof(AlbedoG)); }
    }

    public float AlbedoB
    {
        get => _albedoB;
        set { _albedoB = value; OnPropertyChanged(nameof(AlbedoB)); }
    }

    public float AlbedoA
    {
        get => _albedoA;
        set { _albedoA = value; OnPropertyChanged(nameof(AlbedoA)); }
    }

    public float Metallic
    {
        get => _metallic;
        set { _metallic = value; OnPropertyChanged(nameof(Metallic)); }
    }

    public float Roughness
    {
        get => _roughness;
        set { _roughness = value; OnPropertyChanged(nameof(Roughness)); }
    }

    public float AO
    {
        get => _ao;
        set { _ao = value; OnPropertyChanged(nameof(AO)); }
    }

    public float EmissiveIntensity
    {
        get => _emissiveIntensity;
        set { _emissiveIntensity = value; OnPropertyChanged(nameof(EmissiveIntensity)); }
    }

    public string AlbedoTexturePath
    {
        get => _albedoTexturePath;
        set { _albedoTexturePath = value; OnPropertyChanged(nameof(AlbedoTexturePath)); }
    }

    public string NormalTexturePath
    {
        get => _normalTexturePath;
        set { _normalTexturePath = value; OnPropertyChanged(nameof(NormalTexturePath)); }
    }

    public string MetallicTexturePath
    {
        get => _metallicTexturePath;
        set { _metallicTexturePath = value; OnPropertyChanged(nameof(MetallicTexturePath)); }
    }

    public string RoughnessTexturePath
    {
        get => _roughnessTexturePath;
        set { _roughnessTexturePath = value; OnPropertyChanged(nameof(RoughnessTexturePath)); }
    }

    public string AOTexturePath
    {
        get => _aoTexturePath;
        set { _aoTexturePath = value; OnPropertyChanged(nameof(AOTexturePath)); }
    }
}

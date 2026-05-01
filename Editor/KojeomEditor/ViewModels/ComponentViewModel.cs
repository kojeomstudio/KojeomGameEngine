using System.Collections.ObjectModel;

namespace KojeomEditor.ViewModels;

public enum EComponentType
{
    Base = 0,
    StaticMesh = 1,
    SkeletalMesh = 2,
    Water = 3,
    Terrain = 4,
    Light = 5
}

public class ComponentViewModel : ViewModelBase
{
    private string _name = "Component";
    private EComponentType _componentType = EComponentType.Base;

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
        ComponentType = EComponentType.Base;
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
    private float _emissiveR = 0.0f;
    private float _emissiveG = 0.0f;
    private float _emissiveB = 0.0f;
    private float _emissiveIntensity = 0.0f;
    private string _albedoTexturePath = "";
    private string _normalTexturePath = "";
    private string _metallicTexturePath = "";
    private string _roughnessTexturePath = "";
    private string _aoTexturePath = "";
    private string? _filePath;

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

    public float EmissiveR
    {
        get => _emissiveR;
        set { _emissiveR = value; OnPropertyChanged(nameof(EmissiveR)); }
    }

    public float EmissiveG
    {
        get => _emissiveG;
        set { _emissiveG = value; OnPropertyChanged(nameof(EmissiveG)); }
    }

    public float EmissiveB
    {
        get => _emissiveB;
        set { _emissiveB = value; OnPropertyChanged(nameof(EmissiveB)); }
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

    public string? FilePath
    {
        get => _filePath;
        set { _filePath = value; OnPropertyChanged(nameof(FilePath)); }
    }

    public string ToJson()
    {
        var sb = new System.Text.StringBuilder();
        sb.AppendLine("{");
        sb.AppendLine($"  \"name\": \"{EscapeJson(Name)}\",");
        sb.AppendLine($"  \"albedo\": [{AlbedoR:F6}, {AlbedoG:F6}, {AlbedoB:F6}, {AlbedoA:F6}],");
        sb.AppendLine($"  \"metallic\": {Metallic:F6},");
        sb.AppendLine($"  \"roughness\": {Roughness:F6},");
        sb.AppendLine($"  \"ao\": {AO:F6},");
        sb.AppendLine($"  \"emissive\": [{EmissiveR:F6}, {EmissiveG:F6}, {EmissiveB:F6}],");
        sb.AppendLine($"  \"emissiveIntensity\": {EmissiveIntensity:F6},");
        sb.AppendLine("  \"textures\": {");
        sb.AppendLine($"    \"albedo\": \"{EscapeJson(AlbedoTexturePath)}\",");
        sb.AppendLine($"    \"normal\": \"{EscapeJson(NormalTexturePath)}\",");
        sb.AppendLine($"    \"metallic\": \"{EscapeJson(MetallicTexturePath)}\",");
        sb.AppendLine($"    \"roughness\": \"{EscapeJson(RoughnessTexturePath)}\",");
        sb.AppendLine($"    \"ao\": \"{EscapeJson(AOTexturePath)}\"");
        sb.AppendLine("  }");
        sb.AppendLine("}");
        return sb.ToString();
    }

    public static MaterialViewModel? FromJson(string json)
    {
        try
        {
            var vm = new MaterialViewModel();
            json = json.Trim();
            if (json.StartsWith("{")) json = json.Substring(1);
            if (json.EndsWith("}")) json = json.Substring(0, json.Length - 1);

            vm.Name = ExtractStringValue(json, "name") ?? "Material";

            var albedo = ExtractFloatArray(json, "albedo");
            if (albedo != null && albedo.Length >= 4)
            {
                vm.AlbedoR = albedo[0]; vm.AlbedoG = albedo[1]; vm.AlbedoB = albedo[2]; vm.AlbedoA = albedo[3];
            }

            vm.Metallic = ExtractFloatValue(json, "metallic") ?? 0.0f;
            vm.Roughness = ExtractFloatValue(json, "roughness") ?? 0.5f;
            vm.AO = ExtractFloatValue(json, "ao") ?? 1.0f;

            var emissive = ExtractFloatArray(json, "emissive");
            if (emissive != null && emissive.Length >= 3)
            {
                vm.EmissiveR = emissive[0]; vm.EmissiveG = emissive[1]; vm.EmissiveB = emissive[2];
            }

            vm.EmissiveIntensity = ExtractFloatValue(json, "emissiveIntensity") ?? 0.0f;

            var texturesStart = json.IndexOf("\"textures\"");
            if (texturesStart >= 0)
            {
                var texturesBlock = json.Substring(texturesStart);
                var braceStart = texturesBlock.IndexOf('{');
                var braceEnd = texturesBlock.IndexOf('}');
                if (braceStart >= 0 && braceEnd > braceStart)
                {
                    var texturesJson = texturesBlock.Substring(braceStart, braceEnd - braceStart + 1);
                    vm.AlbedoTexturePath = ExtractStringValue(texturesJson, "albedo") ?? "";
                    vm.NormalTexturePath = ExtractStringValue(texturesJson, "normal") ?? "";
                    vm.MetallicTexturePath = ExtractStringValue(texturesJson, "metallic") ?? "";
                    vm.RoughnessTexturePath = ExtractStringValue(texturesJson, "roughness") ?? "";
                    vm.AOTexturePath = ExtractStringValue(texturesJson, "ao") ?? "";
                }
            }

            return vm;
        }
        catch
        {
            return null;
        }
    }

    private static string EscapeJson(string s)
    {
        return s.Replace("\\", "\\\\").Replace("\"", "\\\"");
    }

    private static string? ExtractStringValue(string json, string key)
    {
        var search = $"\"{key}\"";
        var idx = json.IndexOf(search, System.StringComparison.OrdinalIgnoreCase);
        if (idx < 0) return null;
        var colonIdx = json.IndexOf(':', idx + search.Length);
        if (colonIdx < 0) return null;
        var start = json.IndexOf('"', colonIdx + 1);
        if (start < 0) return null;
        var end = start + 1;
        while (end < json.Length && json[end] != '"')
        {
            if (json[end] == '\\') end++;
            end++;
        }
        if (end >= json.Length) return json.Substring(start + 1);
        var value = json.Substring(start + 1, end - start - 1);
        return value.Replace("\\\"", "\"").Replace("\\\\", "\\");
    }

    private static float? ExtractFloatValue(string json, string key)
    {
        var search = $"\"{key}\"";
        var idx = json.IndexOf(search, System.StringComparison.OrdinalIgnoreCase);
        if (idx < 0) return null;
        var colonIdx = json.IndexOf(':', idx + search.Length);
        if (colonIdx < 0) return null;
        var start = colonIdx + 1;
        while (start < json.Length && (json[start] == ' ' || json[start] == '\t' || json[start] == '\r' || json[start] == '\n')) start++;
        var end = start;
        while (end < json.Length && json[end] != ',' && json[end] != '}' && json[end] != ']' && json[end] != '\r' && json[end] != '\n') end++;
        var valStr = json.Substring(start, end - start).Trim();
        if (float.TryParse(valStr, System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out float val))
            return val;
        return null;
    }

    private static float[]? ExtractFloatArray(string json, string key)
    {
        var search = $"\"{key}\"";
        var idx = json.IndexOf(search, System.StringComparison.OrdinalIgnoreCase);
        if (idx < 0) return null;
        var colonIdx = json.IndexOf(':', idx + search.Length);
        if (colonIdx < 0) return null;
        var bracketStart = json.IndexOf('[', colonIdx);
        if (bracketStart < 0) return null;
        var bracketEnd = json.IndexOf(']', bracketStart);
        if (bracketEnd < 0) return null;
        var content = json.Substring(bracketStart + 1, bracketEnd - bracketStart - 1).Trim();
        var parts = content.Split(',');
        var values = new float[parts.Length];
        for (int i = 0; i < parts.Length; i++)
        {
            if (!float.TryParse(parts[i].Trim(), System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out values[i]))
                return null;
        }
        return values;
    }
}

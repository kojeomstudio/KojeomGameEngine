using System.IO;
using System.Windows;
using System.Windows.Controls;
using KojeomEditor.ViewModels;
using Microsoft.Win32;

namespace KojeomEditor.Views;

public partial class MaterialEditorControl : UserControl
{
    public static readonly DependencyProperty MaterialViewModelProperty =
        DependencyProperty.Register(nameof(MaterialViewModel), typeof(MaterialViewModel), typeof(MaterialEditorControl),
            new PropertyMetadata(null));

    public MaterialViewModel? MaterialViewModel
    {
        get => (MaterialViewModel?)GetValue(MaterialViewModelProperty);
        set => SetValue(MaterialViewModelProperty, value);
    }

    public MaterialEditorControl()
    {
        InitializeComponent();
        MaterialViewModel = new MaterialViewModel();
    }

    private string? BrowseTextureFile()
    {
        var dialog = new OpenFileDialog
        {
            Title = "Select Texture File",
            Filter = "Image Files|*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.dds;*.tif;*.tiff|All Files|*.*",
            CheckFileExists = true,
            CheckPathExists = true
        };

        if (dialog.ShowDialog() == true)
        {
            if (dialog.FileName.Contains("..") || dialog.FileName.Contains("~"))
            {
                MessageBox.Show("Path traversal detected. Please select a file within the project directory.", "Invalid Path", MessageBoxButton.OK, MessageBoxImage.Warning);
                return null;
            }
            return dialog.FileName;
        }
        return null;
    }

    private void OnBrowseAlbedoTexture(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;
        var path = BrowseTextureFile();
        if (path != null)
            MaterialViewModel.AlbedoTexturePath = path;
    }

    private void OnBrowseNormalTexture(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;
        var path = BrowseTextureFile();
        if (path != null)
            MaterialViewModel.NormalTexturePath = path;
    }

    private void OnBrowseMetallicTexture(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;
        var path = BrowseTextureFile();
        if (path != null)
            MaterialViewModel.MetallicTexturePath = path;
    }

    private void OnBrowseRoughnessTexture(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;
        var path = BrowseTextureFile();
        if (path != null)
            MaterialViewModel.RoughnessTexturePath = path;
    }

    private void OnBrowseAOTexture(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;
        var path = BrowseTextureFile();
        if (path != null)
            MaterialViewModel.AOTexturePath = path;
    }

    private void ApplyPreset(string presetName)
    {
        if (MaterialViewModel == null) return;
        switch (presetName)
        {
            case "Metal":
                MaterialViewModel.AlbedoR = 0.95f;
                MaterialViewModel.AlbedoG = 0.95f;
                MaterialViewModel.AlbedoB = 0.95f;
                MaterialViewModel.Metallic = 1.0f;
                MaterialViewModel.Roughness = 0.2f;
                break;
            case "Plastic":
                MaterialViewModel.AlbedoR = 0.5f;
                MaterialViewModel.AlbedoG = 0.5f;
                MaterialViewModel.AlbedoB = 0.5f;
                MaterialViewModel.Metallic = 0.0f;
                MaterialViewModel.Roughness = 0.4f;
                break;
            case "Rubber":
                MaterialViewModel.AlbedoR = 0.2f;
                MaterialViewModel.AlbedoG = 0.2f;
                MaterialViewModel.AlbedoB = 0.2f;
                MaterialViewModel.Metallic = 0.0f;
                MaterialViewModel.Roughness = 0.9f;
                break;
            case "Gold":
                MaterialViewModel.AlbedoR = 1.0f;
                MaterialViewModel.AlbedoG = 0.765557f;
                MaterialViewModel.AlbedoB = 0.336057f;
                MaterialViewModel.Metallic = 1.0f;
                MaterialViewModel.Roughness = 0.3f;
                break;
            case "Silver":
                MaterialViewModel.AlbedoR = 0.971519f;
                MaterialViewModel.AlbedoG = 0.959915f;
                MaterialViewModel.AlbedoB = 0.915324f;
                MaterialViewModel.Metallic = 1.0f;
                MaterialViewModel.Roughness = 0.3f;
                break;
            case "Copper":
                MaterialViewModel.AlbedoR = 0.955008f;
                MaterialViewModel.AlbedoG = 0.637427f;
                MaterialViewModel.AlbedoB = 0.538163f;
                MaterialViewModel.Metallic = 1.0f;
                MaterialViewModel.Roughness = 0.35f;
                break;
            case "Emissive":
                MaterialViewModel.AlbedoR = 0.0f;
                MaterialViewModel.AlbedoG = 0.0f;
                MaterialViewModel.AlbedoB = 0.0f;
                MaterialViewModel.EmissiveR = 1.0f;
                MaterialViewModel.EmissiveG = 0.8f;
                MaterialViewModel.EmissiveB = 0.2f;
                MaterialViewModel.EmissiveIntensity = 5.0f;
                MaterialViewModel.Metallic = 0.0f;
                MaterialViewModel.Roughness = 0.5f;
                break;
            default:
                MaterialViewModel.AlbedoR = 0.8f;
                MaterialViewModel.AlbedoG = 0.8f;
                MaterialViewModel.AlbedoB = 0.8f;
                MaterialViewModel.Metallic = 0.0f;
                MaterialViewModel.Roughness = 0.5f;
                MaterialViewModel.EmissiveR = 0.0f;
                MaterialViewModel.EmissiveG = 0.0f;
                MaterialViewModel.EmissiveB = 0.0f;
                MaterialViewModel.EmissiveIntensity = 0.0f;
                break;
        }
    }

    private void OnMaterialPresetSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (MaterialPresetComboBox.SelectedItem is ComboBoxItem selectedItem)
        {
            ApplyPreset(selectedItem.Content.ToString() ?? "Default");
        }
    }

    private void OnNewMaterial(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;
        MaterialViewModel.Name = "New Material";
        MaterialViewModel.AlbedoR = 1.0f;
        MaterialViewModel.AlbedoG = 1.0f;
        MaterialViewModel.AlbedoB = 1.0f;
        MaterialViewModel.AlbedoA = 1.0f;
        MaterialViewModel.Metallic = 0.0f;
        MaterialViewModel.Roughness = 0.5f;
        MaterialViewModel.AO = 1.0f;
        MaterialViewModel.EmissiveR = 0.0f;
        MaterialViewModel.EmissiveG = 0.0f;
        MaterialViewModel.EmissiveB = 0.0f;
        MaterialViewModel.EmissiveIntensity = 0.0f;
        MaterialViewModel.AlbedoTexturePath = string.Empty;
        MaterialViewModel.NormalTexturePath = string.Empty;
        MaterialViewModel.MetallicTexturePath = string.Empty;
        MaterialViewModel.RoughnessTexturePath = string.Empty;
        MaterialViewModel.AOTexturePath = string.Empty;
        MaterialViewModel.FilePath = null;
    }

    private void OnSaveMaterial(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;

        string? filePath = MaterialViewModel.FilePath;
        if (filePath == null)
        {
            var dialog = new SaveFileDialog
            {
                Title = "Save Material",
                Filter = "Material Files (*.mat)|*.mat|All Files (*.*)|*.*",
                FileName = $"{MaterialViewModel.Name}.mat"
            };

            if (dialog.ShowDialog() != true) return;
            filePath = dialog.FileName;
        }

        try
        {
            var json = MaterialViewModel.ToJson();
            File.WriteAllText(filePath, json);
            MaterialViewModel.FilePath = filePath;
            MessageBox.Show($"Material saved to {Path.GetFileName(filePath)}", "Save Successful", MessageBoxButton.OK, MessageBoxImage.Information);
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Failed to save material: {ex.Message}", "Save Error", MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    private void OnSaveMaterialAs(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;

        var dialog = new SaveFileDialog
        {
            Title = "Save Material As",
            Filter = "Material Files (*.mat)|*.mat|All Files (*.*)|*.*",
            FileName = $"{MaterialViewModel.Name}.mat"
        };

        if (dialog.ShowDialog() != true) return;

        try
        {
            var json = MaterialViewModel.ToJson();
            File.WriteAllText(dialog.FileName, json);
            MaterialViewModel.FilePath = dialog.FileName;
            MessageBox.Show($"Material saved to {Path.GetFileName(dialog.FileName)}", "Save Successful", MessageBoxButton.OK, MessageBoxImage.Information);
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Failed to save material: {ex.Message}", "Save Error", MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    private void OnLoadMaterial(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;

        var dialog = new OpenFileDialog
        {
            Title = "Load Material",
            Filter = "Material Files (*.mat)|*.mat|All Files (*.*)|*.*",
            CheckFileExists = true
        };

        if (dialog.ShowDialog() != true) return;

        try
        {
            var json = File.ReadAllText(dialog.FileName);
            var loaded = MaterialViewModel.FromJson(json);
            if (loaded == null)
            {
                MessageBox.Show("Failed to parse material file.", "Load Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            loaded.FilePath = dialog.FileName;
            MaterialViewModel.Name = loaded.Name;
            MaterialViewModel.AlbedoR = loaded.AlbedoR;
            MaterialViewModel.AlbedoG = loaded.AlbedoG;
            MaterialViewModel.AlbedoB = loaded.AlbedoB;
            MaterialViewModel.AlbedoA = loaded.AlbedoA;
            MaterialViewModel.Metallic = loaded.Metallic;
            MaterialViewModel.Roughness = loaded.Roughness;
            MaterialViewModel.AO = loaded.AO;
            MaterialViewModel.EmissiveR = loaded.EmissiveR;
            MaterialViewModel.EmissiveG = loaded.EmissiveG;
            MaterialViewModel.EmissiveB = loaded.EmissiveB;
            MaterialViewModel.EmissiveIntensity = loaded.EmissiveIntensity;
            MaterialViewModel.AlbedoTexturePath = loaded.AlbedoTexturePath;
            MaterialViewModel.NormalTexturePath = loaded.NormalTexturePath;
            MaterialViewModel.MetallicTexturePath = loaded.MetallicTexturePath;
            MaterialViewModel.RoughnessTexturePath = loaded.RoughnessTexturePath;
            MaterialViewModel.AOTexturePath = loaded.AOTexturePath;
            MaterialViewModel.FilePath = loaded.FilePath;
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Failed to load material: {ex.Message}", "Load Error", MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    private void OnPresetDefault(object sender, RoutedEventArgs e)
    {
        ApplyPreset("Default");
    }

    private void OnPresetMetal(object sender, RoutedEventArgs e)
    {
        ApplyPreset("Metal");
    }

    private void OnPresetPlastic(object sender, RoutedEventArgs e)
    {
        ApplyPreset("Plastic");
    }

    private void OnPresetRubber(object sender, RoutedEventArgs e)
    {
        ApplyPreset("Rubber");
    }

    private void OnPresetGold(object sender, RoutedEventArgs e)
    {
        ApplyPreset("Gold");
    }

    private void OnPresetSilver(object sender, RoutedEventArgs e)
    {
        ApplyPreset("Silver");
    }

    private void OnPresetCopper(object sender, RoutedEventArgs e)
    {
        ApplyPreset("Copper");
    }

    private void OnPresetEmissive(object sender, RoutedEventArgs e)
    {
        ApplyPreset("Emissive");
    }
}

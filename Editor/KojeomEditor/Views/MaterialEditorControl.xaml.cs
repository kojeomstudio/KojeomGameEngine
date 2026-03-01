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

    private void OnPresetDefault(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;
        MaterialViewModel.AlbedoR = 0.8f;
        MaterialViewModel.AlbedoG = 0.8f;
        MaterialViewModel.AlbedoB = 0.8f;
        MaterialViewModel.Metallic = 0.0f;
        MaterialViewModel.Roughness = 0.5f;
    }

    private void OnPresetMetal(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;
        MaterialViewModel.AlbedoR = 0.95f;
        MaterialViewModel.AlbedoG = 0.95f;
        MaterialViewModel.AlbedoB = 0.95f;
        MaterialViewModel.Metallic = 1.0f;
        MaterialViewModel.Roughness = 0.2f;
    }

    private void OnPresetPlastic(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;
        MaterialViewModel.AlbedoR = 0.5f;
        MaterialViewModel.AlbedoG = 0.5f;
        MaterialViewModel.AlbedoB = 0.5f;
        MaterialViewModel.Metallic = 0.0f;
        MaterialViewModel.Roughness = 0.4f;
    }

    private void OnPresetRubber(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;
        MaterialViewModel.AlbedoR = 0.2f;
        MaterialViewModel.AlbedoG = 0.2f;
        MaterialViewModel.AlbedoB = 0.2f;
        MaterialViewModel.Metallic = 0.0f;
        MaterialViewModel.Roughness = 0.9f;
    }

    private void OnPresetGold(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;
        MaterialViewModel.AlbedoR = 1.0f;
        MaterialViewModel.AlbedoG = 0.765557f;
        MaterialViewModel.AlbedoB = 0.336057f;
        MaterialViewModel.Metallic = 1.0f;
        MaterialViewModel.Roughness = 0.3f;
    }

    private void OnPresetSilver(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;
        MaterialViewModel.AlbedoR = 0.971519f;
        MaterialViewModel.AlbedoG = 0.959915f;
        MaterialViewModel.AlbedoB = 0.915324f;
        MaterialViewModel.Metallic = 1.0f;
        MaterialViewModel.Roughness = 0.3f;
    }

    private void OnPresetCopper(object sender, RoutedEventArgs e)
    {
        if (MaterialViewModel == null) return;
        MaterialViewModel.AlbedoR = 0.955008f;
        MaterialViewModel.AlbedoG = 0.637427f;
        MaterialViewModel.AlbedoB = 0.538163f;
        MaterialViewModel.Metallic = 1.0f;
        MaterialViewModel.Roughness = 0.35f;
    }
}

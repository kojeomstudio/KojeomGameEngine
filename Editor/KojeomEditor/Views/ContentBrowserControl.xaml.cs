using System.Collections.ObjectModel;
using System.IO;
using System.Windows;
using System.Windows.Controls;

namespace KojeomEditor.Views;

public partial class ContentBrowserControl : UserControl
{
    public ObservableCollection<FolderItem> Folders { get; } = new();
    public ObservableCollection<AssetItem> Assets { get; } = new();
    
    public static readonly DependencyProperty CurrentFolderProperty =
        DependencyProperty.Register(nameof(CurrentFolder), typeof(string), typeof(ContentBrowserControl),
            new PropertyMetadata("Assets"));

    public string CurrentFolder
    {
        get => (string)GetValue(CurrentFolderProperty);
        set => SetValue(CurrentFolderProperty, value);
    }

    public static readonly DependencyProperty SelectedAssetProperty =
        DependencyProperty.Register(nameof(SelectedAsset), typeof(AssetItem), typeof(ContentBrowserControl),
            new PropertyMetadata(null));

    public AssetItem? SelectedAsset
    {
        get => (AssetItem?)GetValue(SelectedAssetProperty);
        set => SetValue(SelectedAssetProperty, value);
    }

    public ContentBrowserControl()
    {
        InitializeComponent();
        DataContext = this;
        InitializeFolders();
        RefreshAssets();
    }

    private void InitializeFolders()
    {
        Folders.Clear();
        Folders.Add(new FolderItem { Name = "Assets", Path = "Assets", IconText = "📁" });
        Folders.Add(new FolderItem { Name = "Models", Path = "Assets/Models", IconText = "📁" });
        Folders.Add(new FolderItem { Name = "Textures", Path = "Assets/Textures", IconText = "📁" });
        Folders.Add(new FolderItem { Name = "Materials", Path = "Assets/Materials", IconText = "📁" });
        Folders.Add(new FolderItem { Name = "Scenes", Path = "Assets/Scenes", IconText = "📁" });
    }

    private void RefreshAssets()
    {
        Assets.Clear();
        
        var currentPath = CurrentFolder;
        if (!Directory.Exists(currentPath))
        {
            AddDefaultAssets();
            return;
        }

        try
        {
            var files = Directory.GetFiles(currentPath);
            foreach (var file in files)
            {
                var ext = Path.GetExtension(file).ToLower();
                Assets.Add(new AssetItem
                {
                    Name = Path.GetFileNameWithoutExtension(file),
                    Path = file,
                    Extension = ext,
                    IconText = GetIconForExtension(ext)
                });
            }

            var dirs = Directory.GetDirectories(currentPath);
            foreach (var dir in dirs)
            {
                Assets.Add(new AssetItem
                {
                    Name = Path.GetFileName(dir),
                    Path = dir,
                    Extension = "[Folder]",
                    IconText = "📁"
                });
            }
        }
        catch
        {
            AddDefaultAssets();
        }
    }

    private void AddDefaultAssets()
    {
        Assets.Add(new AssetItem { Name = "DefaultCube", Extension = ".fbx", IconText = "📦" });
        Assets.Add(new AssetItem { Name = "DefaultSphere", Extension = ".fbx", IconText = "📦" });
        Assets.Add(new AssetItem { Name = "DefaultMaterial", Extension = ".mat", IconText = "🎨" });
        Assets.Add(new AssetItem { Name = "CheckerTexture", Extension = ".png", IconText = "🖼" });
    }

    private static string GetIconForExtension(string ext)
    {
        return ext switch
        {
            ".fbx" or ".obj" or ".gltf" or ".glb" => "📦",
            ".png" or ".jpg" or ".jpeg" or ".tga" or ".bmp" => "🖼",
            ".mat" or ".material" => "🎨",
            ".scene" => "🗺",
            ".anim" => "🎬",
            ".wav" or ".mp3" or ".ogg" => "🔊",
            ".cs" => "📄",
            _ => "📄"
        };
    }

    private void FolderTree_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
    {
        if (e.NewValue is FolderItem folder)
        {
            CurrentFolder = folder.Path;
            RefreshAssets();
        }
    }

    private void Import_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new Microsoft.Win32.OpenFileDialog
        {
            Filter = "All Supported Files|*.fbx;*.obj;*.gltf;*.png;*.jpg;*.tga|Model Files|*.fbx;*.obj;*.gltf|Texture Files|*.png;*.jpg;*.tga|All Files|*.*",
            Title = "Import Asset"
        };

        if (dialog.ShowDialog() == true)
        {
            var destPath = Path.Combine(CurrentFolder, Path.GetFileName(dialog.FileName));
            try
            {
                File.Copy(dialog.FileName, destPath, true);
                RefreshAssets();
            }
            catch { }
        }
    }

    private void Refresh_Click(object sender, RoutedEventArgs e)
    {
        RefreshAssets();
    }

    private void NavigateToAssets_Click(object sender, RoutedEventArgs e)
    {
        CurrentFolder = "Assets";
        RefreshAssets();
    }

    private void Asset_DoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (SelectedAsset == null) return;
        
        if (SelectedAsset.Extension == "[Folder]")
        {
            CurrentFolder = SelectedAsset.Path;
            RefreshAssets();
        }
        else if (SelectedAsset.Extension == ".scene")
        {
        }
    }
}

public class FolderItem
{
    public string Name { get; set; } = "";
    public string Path { get; set; } = "";
    public string IconText { get; set; } = "📁";
    public ObservableCollection<FolderItem> SubFolders { get; } = new();
}

public class AssetItem
{
    public string Name { get; set; } = "";
    public string Path { get; set; } = "";
    public string Extension { get; set; } = "";
    public string IconText { get; set; } = "📄";
}

using System.Collections.ObjectModel;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace KojeomEditor.Views;

public class BoolToVisibilityConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        return value is bool b && b ? Visibility.Visible : Visibility.Collapsed;
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        return value is Visibility v && v == Visibility.Visible;
    }
}

public class InverseBoolToVisibilityConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        return value is bool b && !b ? Visibility.Visible : Visibility.Collapsed;
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        return value is Visibility v && v != Visibility.Visible;
    }
}

public partial class ContentBrowserControl : UserControl
{
    public ObservableCollection<FolderItem> Folders { get; } = new();
    public ObservableCollection<AssetItem> Assets { get; } = new();
    
    private Point _dragStartPoint;
    private bool _isDragging;
    
    public static readonly RoutedEvent AssetDragEvent = 
        EventManager.RegisterRoutedEvent(nameof(AssetDrag), RoutingStrategy.Bubble, 
            typeof(RoutedEventHandler), typeof(ContentBrowserControl));
    
    public event RoutedEventHandler AssetDrag
    {
        add => AddHandler(AssetDragEvent, value);
        remove => RemoveHandler(AssetDragEvent, value);
    }
    
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
                var asset = new AssetItem
                {
                    Name = Path.GetFileNameWithoutExtension(file),
                    Path = file,
                    Extension = ext,
                    IconText = GetIconForExtension(ext)
                };
                
                if (IsImageFile(ext))
                {
                    asset.ThumbnailImage = CreateThumbnail(file);
                }
                
                Assets.Add(asset);
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

    private static bool IsImageFile(string ext)
    {
        return ext is ".png" or ".jpg" or ".jpeg" or ".bmp" or ".tga" or ".gif";
    }

    private BitmapImage? CreateThumbnail(string filePath)
    {
        try
        {
            var bitmap = new BitmapImage();
            bitmap.BeginInit();
            bitmap.UriSource = new Uri(filePath, UriKind.Absolute);
            bitmap.DecodePixelWidth = 64;
            bitmap.DecodePixelHeight = 64;
            bitmap.CacheOption = BitmapCacheOption.OnLoad;
            bitmap.EndInit();
            bitmap.Freeze();
            return bitmap;
        }
        catch
        {
            return null;
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

    private bool IsPathWithinProject(string path)
    {
        var fullPath = Path.GetFullPath(path);
        var exeDir = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location) ?? ".";
        var projectRoot = Path.GetFullPath(Path.Combine(exeDir, "Assets"));
        return fullPath.StartsWith(projectRoot, StringComparison.OrdinalIgnoreCase);
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
            if (!IsPathWithinProject(destPath)) return;
            try
            {
                if (File.Exists(destPath))
                {
                    var result = MessageBox.Show($"File '{Path.GetFileName(destPath)}' already exists. Overwrite?",
                        "Confirm Overwrite", MessageBoxButton.YesNo, MessageBoxImage.Question);
                    if (result != MessageBoxResult.Yes) return;
                }
                File.Copy(dialog.FileName, destPath, true);
                RefreshAssets();
            }
            catch (Exception ex) { MessageBox.Show($"Import failed: {ex.Message}", "Error"); }
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
            if (Window.GetWindow(this)?.DataContext is ViewModels.MainViewModel mainVm)
            {
                mainVm.SceneViewModel.LoadScene(SelectedAsset.Path);
            }
        }
    }
    
    private void AssetList_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        _dragStartPoint = e.GetPosition(null);
        _isDragging = false;
    }
    
    private void AssetList_PreviewMouseMove(object sender, MouseEventArgs e)
    {
        if (e.LeftButton == MouseButtonState.Pressed && !_isDragging)
        {
            var currentPosition = e.GetPosition(null);
            var diff = _dragStartPoint - currentPosition;
            
            if (Math.Abs(diff.X) > SystemParameters.MinimumHorizontalDragDistance ||
                Math.Abs(diff.Y) > SystemParameters.MinimumVerticalDragDistance)
            {
                StartDrag(e);
            }
        }
    }
    
    private void StartDrag(MouseEventArgs e)
    {
        if (SelectedAsset == null) return;
        if (SelectedAsset.Extension == "[Folder]") return;
        
        _isDragging = true;
        
        var data = new DataObject();
        data.SetData("AssetPath", SelectedAsset.Path);
        data.SetData("AssetName", SelectedAsset.Name);
        data.SetData("AssetExtension", SelectedAsset.Extension);
        
        try
        {
            DragDrop.DoDragDrop(AssetListBox, data, DragDropEffects.Copy);
        }
        finally
        {
            _isDragging = false;
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
    public BitmapImage? ThumbnailImage { get; set; }
    public bool HasThumbnail => ThumbnailImage != null;
}

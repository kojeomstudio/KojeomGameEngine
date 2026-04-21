using System.Windows;
using System.Windows.Controls;
using KojeomEditor.Services;

namespace KojeomEditor.Views;

public partial class RendererSettingsControl : UserControl
{
    public EngineInterop? Engine { get; set; }

    public event Action<bool>? ShowGridChanged;
    public event Action<bool>? ShowAxisChanged;

    private bool _showGrid = true;
    public bool ShowGrid
    {
        get => _showGrid;
        private set
        {
            if (_showGrid != value)
            {
                _showGrid = value;
                ShowGridChanged?.Invoke(value);
            }
        }
    }

    private bool _showAxis = true;
    public bool ShowAxis
    {
        get => _showAxis;
        private set
        {
            if (_showAxis != value)
            {
                _showAxis = value;
                ShowAxisChanged?.Invoke(value);
            }
        }
    }

    public RendererSettingsControl()
    {
        InitializeComponent();
        Loaded += OnLoaded;
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        CheckBoxSSAO.IsChecked = true;
        CheckBoxPostProcess.IsChecked = true;
        CheckBoxShadows.IsChecked = true;
        CheckBoxCascadedShadows.IsChecked = false;
        CheckBoxIBL.IsChecked = false;
        CheckBoxSky.IsChecked = true;
        CheckBoxTAA.IsChecked = true;
        CheckBoxDebugUI.IsChecked = false;
        CheckBoxSSR.IsChecked = true;
        CheckBoxVolumetricFog.IsChecked = true;
        CheckBoxWireframe.IsChecked = false;
    }

    private void OnSSAOChanged(object sender, RoutedEventArgs e)
    {
        if (Engine != null) Engine.SetSSAOEnabled(CheckBoxSSAO.IsChecked == true);
    }

    private void OnPostProcessChanged(object sender, RoutedEventArgs e)
    {
        if (Engine != null) Engine.SetPostProcessEnabled(CheckBoxPostProcess.IsChecked == true);
    }

    private void OnShadowsChanged(object sender, RoutedEventArgs e)
    {
        if (Engine != null) Engine.SetShadowEnabled(CheckBoxShadows.IsChecked == true);
    }

    private void OnCascadedShadowsChanged(object sender, RoutedEventArgs e)
    {
        if (Engine != null) Engine.SetCascadedShadowsEnabled(CheckBoxCascadedShadows.IsChecked == true);
    }

    private void OnIBLChanged(object sender, RoutedEventArgs e)
    {
        if (Engine != null) Engine.SetIBLEnabled(CheckBoxIBL.IsChecked == true);
    }

    private void OnSkyChanged(object sender, RoutedEventArgs e)
    {
        if (Engine != null) Engine.SetSkyEnabled(CheckBoxSky.IsChecked == true);
    }

    private void OnTAAChanged(object sender, RoutedEventArgs e)
    {
        if (Engine != null) Engine.SetTAAEnabled(CheckBoxTAA.IsChecked == true);
    }

    private void OnDebugUIChanged(object sender, RoutedEventArgs e)
    {
        if (Engine != null) Engine.SetDebugUIEnabled(CheckBoxDebugUI.IsChecked == true);
    }

    private void OnSSRChanged(object sender, RoutedEventArgs e)
    {
        if (Engine != null) Engine.SetSSREnabled(CheckBoxSSR.IsChecked == true);
    }

    private void OnVolumetricFogChanged(object sender, RoutedEventArgs e)
    {
        if (Engine != null) Engine.SetVolumetricFogEnabled(CheckBoxVolumetricFog.IsChecked == true);
    }

    private void OnWireframeChanged(object sender, RoutedEventArgs e)
    {
        if (Engine != null) Engine.SetDebugMode(CheckBoxWireframe.IsChecked == true);
    }

    private void OnShowGridChanged(object sender, RoutedEventArgs e)
    {
        ShowGrid = CheckBoxShowGrid.IsChecked == true;
        if (Engine != null && ShowGrid)
        {
            Engine.DebugRendererDrawGrid(0, 0, 0, 40.0f, 2.0f, 10);
        }
    }

    private void OnShowAxisChanged(object sender, RoutedEventArgs e)
    {
        ShowAxis = CheckBoxShowAxis.IsChecked == true;
        if (Engine != null && ShowAxis)
        {
            Engine.DebugRendererDrawAxis(0, 0.01f, 0, 2.0f);
        }
    }
}

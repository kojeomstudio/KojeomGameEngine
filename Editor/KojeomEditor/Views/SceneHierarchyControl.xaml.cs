using System.Windows;
using System.Windows.Controls;

namespace KojeomEditor.Views;

public partial class SceneHierarchyControl : UserControl
{
    public SceneHierarchyControl()
    {
        InitializeComponent();
    }

    private void TreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
    {
        if (DataContext is ViewModels.MainViewModel mainVm && e.NewValue is ViewModels.ActorViewModel actor)
        {
            mainVm.PropertiesViewModel.SetSelectedActor(actor);
            mainVm.SceneViewModel.SelectedActor = actor;
        }
    }

    private void AddActor_Click(object sender, RoutedEventArgs e)
    {
        if (DataContext is ViewModels.MainViewModel mainVm)
        {
            mainVm.SceneViewModel.AddActor($"Actor_{mainVm.SceneViewModel.Actors.Count + 1}");
        }
    }

    private void AddEmpty_Click(object sender, RoutedEventArgs e)
    {
        if (DataContext is ViewModels.MainViewModel mainVm)
        {
            mainVm.SceneViewModel.AddActor("Empty", "Empty");
        }
    }

    private void Delete_Click(object sender, RoutedEventArgs e)
    {
        if (DataContext is ViewModels.MainViewModel mainVm && mainVm.SceneViewModel.SelectedActor != null)
        {
            mainVm.SceneViewModel.RemoveActor(mainVm.SceneViewModel.SelectedActor);
            mainVm.PropertiesViewModel.SetSelectedActor(null);
        }
    }
}

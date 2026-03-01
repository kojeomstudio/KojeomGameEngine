using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using KojeomEditor.ViewModels;

namespace KojeomEditor.Views;

public partial class SceneHierarchyControl : UserControl
{
    private ActorViewModel? _draggedItem;
    private Point _dragStartPoint;
    private static readonly double DragThreshold = 5.0;

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

    private void TreeView_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        _dragStartPoint = e.GetPosition(ActorTreeView);
        var item = GetTreeViewItemAtPosition(e.GetPosition(ActorTreeView));
        _draggedItem = item?.DataContext as ActorViewModel;
    }

    private void TreeView_PreviewMouseMove(object sender, MouseEventArgs e)
    {
        if (e.LeftButton == MouseButtonState.Pressed && _draggedItem != null)
        {
            Point currentPosition = e.GetPosition(ActorTreeView);
            double distance = (_dragStartPoint - currentPosition).Length;

            if (distance > DragThreshold)
            {
                DragDrop.DoDragDrop(ActorTreeView, _draggedItem, DragDropEffects.Move);
                _draggedItem = null;
            }
        }
    }

    private void TreeView_DragOver(object sender, DragEventArgs e)
    {
        if (!e.Data.GetDataPresent(typeof(ActorViewModel)))
        {
            e.Effects = DragDropEffects.None;
            return;
        }

        var targetItem = GetTreeViewItemAtPosition(e.GetPosition(ActorTreeView));
        if (targetItem == null)
        {
            e.Effects = DragDropEffects.None;
            return;
        }

        var targetActor = targetItem.DataContext as ActorViewModel;
        if (targetActor == _draggedItem)
        {
            e.Effects = DragDropEffects.None;
            return;
        }

        e.Effects = DragDropEffects.Move;
        e.Handled = true;
    }

    private void TreeView_Drop(object sender, DragEventArgs e)
    {
        if (!e.Data.GetDataPresent(typeof(ActorViewModel)))
            return;

        var draggedActor = e.Data.GetData(typeof(ActorViewModel)) as ActorViewModel;
        if (draggedActor == null)
            return;

        var targetItem = GetTreeViewItemAtPosition(e.GetPosition(ActorTreeView));
        if (targetItem == null)
            return;

        var targetActor = targetItem.DataContext as ActorViewModel;
        if (targetActor == null || targetActor == draggedActor)
            return;

        if (DataContext is ViewModels.MainViewModel mainVm)
        {
            mainVm.SceneViewModel.ReorderActors(draggedActor, targetActor);
        }

        _draggedItem = null;
    }

    private TreeViewItem? GetTreeViewItemAtPosition(Point position)
    {
        var result = VisualTreeHelper.HitTest(ActorTreeView, position);
        if (result == null)
            return null;

        var dependencyObject = result.VisualHit;
        while (dependencyObject != null)
        {
            if (dependencyObject is TreeViewItem item)
                return item;
            dependencyObject = System.Windows.Media.VisualTreeHelper.GetParent(dependencyObject);
        }
        return null;
    }
}

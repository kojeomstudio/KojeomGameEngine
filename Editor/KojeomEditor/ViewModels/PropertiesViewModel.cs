namespace KojeomEditor.ViewModels;

public class PropertiesViewModel : ViewModelBase
{
    private ActorViewModel? _selectedActor;

    public ActorViewModel? SelectedActor
    {
        get => _selectedActor;
        set
        {
            _selectedActor = value;
            OnPropertyChanged(nameof(SelectedActor));
            OnPropertyChanged(nameof(HasSelection));
        }
    }

    public bool HasSelection => SelectedActor != null;

    public void SetSelectedActor(ActorViewModel? actor)
    {
        SelectedActor = actor;
    }
}

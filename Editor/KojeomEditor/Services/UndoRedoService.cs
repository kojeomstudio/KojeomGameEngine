using System;
using System.Collections.Generic;
using System.Windows.Input;

namespace KojeomEditor.Services;

public interface IUndoableAction
{
    string Description { get; }
    void Undo();
    void Redo();
}

public class UndoRedoService
{
    private readonly Stack<IUndoableAction> _undoStack = new();
    private readonly Stack<IUndoableAction> _redoStack = new();
    private readonly int _maxHistorySize;

    public bool CanUndo => _undoStack.Count > 0;
    public bool CanRedo => _redoStack.Count > 0;
    public int UndoCount => _undoStack.Count;
    public int RedoCount => _redoStack.Count;

    public event EventHandler? StateChanged;

    public UndoRedoService(int maxHistorySize = 100)
    {
        _maxHistorySize = maxHistorySize;
    }

    public void ExecuteAction(IUndoableAction action)
    {
        _undoStack.Push(action);
        _redoStack.Clear();

        TrimHistory();
        OnStateChanged();
    }

    public void ExecuteActionWithRedo(IUndoableAction action)
    {
        action.Redo();
        _undoStack.Push(action);
        _redoStack.Clear();

        TrimHistory();
        OnStateChanged();
    }

    public void Undo()
    {
        if (!CanUndo) return;

        var action = _undoStack.Pop();
        action.Undo();
        _redoStack.Push(action);
        OnStateChanged();
    }

    public void Redo()
    {
        if (!CanRedo) return;

        var action = _redoStack.Pop();
        action.Redo();
        _undoStack.Push(action);
        OnStateChanged();
    }

    public void Clear()
    {
        _undoStack.Clear();
        _redoStack.Clear();
        OnStateChanged();
    }

    public string? GetUndoDescription()
    {
        return CanUndo ? _undoStack.Peek().Description : null;
    }

    public string? GetRedoDescription()
    {
        return CanRedo ? _redoStack.Peek().Description : null;
    }

    private void TrimHistory()
    {
        while (_undoStack.Count > _maxHistorySize)
        {
            var temp = new Stack<IUndoableAction>();
            for (int i = 0; i < _maxHistorySize - 1 && _undoStack.Count > 0; i++)
            {
                temp.Push(_undoStack.Pop());
            }
            _undoStack.Clear();
            while (temp.Count > 0)
            {
                _undoStack.Push(temp.Pop());
            }
            break;
        }
    }

    protected virtual void OnStateChanged()
    {
        StateChanged?.Invoke(this, EventArgs.Empty);
    }
}

public class SetPropertyAction<T> : IUndoableAction
{
    private readonly object _target;
    private readonly string _propertyName;
    private readonly T _oldValue;
    private readonly T _newValue;
    private readonly Action<T> _setter;

    public string Description => $"Set {_propertyName}";

    public SetPropertyAction(object target, string propertyName, T oldValue, T newValue, Action<T> setter)
    {
        _target = target;
        _propertyName = propertyName;
        _oldValue = oldValue;
        _newValue = newValue;
        _setter = setter;
    }

    public void Undo() => _setter(_oldValue);
    public void Redo() => _setter(_newValue);
}

public class AddItemAction<T> : IUndoableAction
{
    private readonly IList<T> _collection;
    private readonly T _item;
    private readonly int _index;

    public string Description => $"Add {_item?.GetType().Name ?? "item"}";

    public AddItemAction(IList<T> collection, T item, int index = -1)
    {
        _collection = collection;
        _item = item;
        _index = index >= 0 ? index : collection.Count;
    }

    public void Undo()
    {
        if (_collection.Contains(_item))
        {
            _collection.Remove(_item);
        }
    }

    public void Redo()
    {
        if (!_collection.Contains(_item))
        {
            if (_index >= 0 && _index <= _collection.Count)
            {
                _collection.Insert(_index, _item);
            }
            else
            {
                _collection.Add(_item);
            }
        }
    }
}

public class RemoveItemAction<T> : IUndoableAction
{
    private readonly IList<T> _collection;
    private readonly T _item;
    private int _originalIndex;

    public string Description => $"Remove {_item?.GetType().Name ?? "item"}";

    public RemoveItemAction(IList<T> collection, T item)
    {
        _collection = collection;
        _item = item;
        _originalIndex = collection.IndexOf(item);
    }

    public void Undo()
    {
        if (!_collection.Contains(_item))
        {
            if (_originalIndex >= 0 && _originalIndex <= _collection.Count)
            {
                _collection.Insert(_originalIndex, _item);
            }
            else
            {
                _collection.Add(_item);
            }
        }
    }

    public void Redo()
    {
        _originalIndex = _collection.IndexOf(_item);
        if (_collection.Contains(_item))
        {
            _collection.Remove(_item);
        }
    }
}

public class CompositeAction : IUndoableAction
{
    private readonly List<IUndoableAction> _actions;
    private readonly string _description;

    public string Description => _description;

    public CompositeAction(string description, params IUndoableAction[] actions)
    {
        _description = description;
        _actions = new List<IUndoableAction>(actions);
    }

    public void AddAction(IUndoableAction action)
    {
        _actions.Add(action);
    }

    public void Undo()
    {
        for (int i = _actions.Count - 1; i >= 0; i--)
        {
            _actions[i].Undo();
        }
    }

    public void Redo()
    {
        foreach (var action in _actions)
        {
            action.Redo();
        }
    }
}

public class UndoCommand : ICommand
{
    private readonly UndoRedoService _service;

    public event EventHandler? CanExecuteChanged
    {
        add { _service.StateChanged += value; }
        remove { _service.StateChanged -= value; }
    }

    public UndoCommand(UndoRedoService service)
    {
        _service = service;
    }

    public bool CanExecute(object? parameter) => _service.CanUndo;
    public void Execute(object? parameter) => _service.Undo();
}

public class RedoCommand : ICommand
{
    private readonly UndoRedoService _service;

    public event EventHandler? CanExecuteChanged
    {
        add { _service.StateChanged += value; }
        remove { _service.StateChanged -= value; }
    }

    public RedoCommand(UndoRedoService service)
    {
        _service = service;
    }

    public bool CanExecute(object? parameter) => _service.CanRedo;
    public void Execute(object? parameter) => _service.Redo();
}

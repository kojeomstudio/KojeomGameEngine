using System.IO;
using System.Windows;
using System.Runtime.InteropServices;

namespace KojeomEditor;

public partial class App : Application
{
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool SetDllDirectory(string lpPathName);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool AddDllDirectory(string lpPathName);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool SetDefaultDllDirectories(uint DirectoryFlags);

    private const uint LOAD_LIBRARY_SEARCH_DEFAULT_DIRS = 0x00001000;

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        string appDir = AppDomain.CurrentDomain.BaseDirectory;
        AddDllDirectory(appDir);
        SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

        var mainWindow = new MainWindow();
        mainWindow.Show();
    }
}

using System.IO;
using System.Windows;
using System.Runtime.InteropServices;

namespace KojeomEditor;

public partial class App : Application
{
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool SetDllDirectory(string lpPathName);

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        string appDir = AppDomain.CurrentDomain.BaseDirectory;
        SetDllDirectory(appDir);

        var mainWindow = new MainWindow();
        mainWindow.Show();
    }
}

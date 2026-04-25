#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>

namespace CLIUtils
{
    struct FParsedArgs
    {
        std::string Command;
        std::unordered_map<std::string, std::string> Options;
        std::vector<std::string> PositionalArgs;
        bool bHelpRequested = false;
    };

    inline FParsedArgs ParseCommandLine(const std::string& CmdLine)
    {
        FParsedArgs Result;
        std::istringstream Stream(CmdLine);
        std::string Token;
        std::vector<std::string> Tokens;

        bool InQuotes = false;
        std::string Current;
        for (char C : CmdLine)
        {
            if (C == '"')
            {
                InQuotes = !InQuotes;
                continue;
            }
            if (C == ' ' && !InQuotes)
            {
                if (!Current.empty())
                {
                    Tokens.push_back(Current);
                    Current.clear();
                }
            }
            else
            {
                Current += C;
            }
        }
        if (!Current.empty())
        {
            Tokens.push_back(Current);
        }

        for (const auto& Tok : Tokens)
        {
            if (Tok == "--help" || Tok == "-h" || Tok == "/?")
            {
                Result.bHelpRequested = true;
            }
            else if (Tok.size() > 2 && Tok.substr(0, 2) == "--")
            {
                size_t EqPos = Tok.find('=');
                if (EqPos != std::string::npos)
                {
                    std::string Key = Tok.substr(2, EqPos - 2);
                    std::string Val = Tok.substr(EqPos + 1);
                    Result.Options[Key] = Val;
                }
                else
                {
                    Result.Options[Tok.substr(2)] = "true";
                }
            }
            else if (Tok.size() > 1 && Tok[0] == '-')
            {
                Result.Options[Tok.substr(1)] = "true";
            }
            else
            {
                if (Result.Command.empty())
                {
                    Result.Command = Tok;
                }
                else
                {
                    Result.PositionalArgs.push_back(Tok);
                }
            }
        }

        return Result;
    }

    inline FParsedArgs ParseCommandLine(LPCWSTR lpCmdLine)
    {
        std::string CmdLine;
        if (lpCmdLine)
        {
            int SizeNeeded = WideCharToMultiByte(CP_UTF8, 0, lpCmdLine, -1, NULL, 0, NULL, NULL);
            if (SizeNeeded > 0)
            {
                CmdLine.resize(SizeNeeded - 1);
                WideCharToMultiByte(CP_UTF8, 0, lpCmdLine, -1, &CmdLine[0], SizeNeeded, NULL, NULL);
            }
        }
        return ParseCommandLine(CmdLine);
    }

    inline bool HasOption(const FParsedArgs& Args, const std::string& Name)
    {
        return Args.Options.find(Name) != Args.Options.end();
    }

    inline std::string GetOption(const FParsedArgs& Args, const std::string& Name, const std::string& Default = "")
    {
        auto It = Args.Options.find(Name);
        return It != Args.Options.end() ? It->second : Default;
    }

    inline std::string GetHelpText()
    {
        return
            "KojeomEngine CLI Test Runner\n"
            "============================\n"
            "\n"
            "Usage: CLITestRunner.exe [command] [options]\n"
            "\n"
            "Commands:\n"
            "  all                Run all tests\n"
            "  rendering          Test rendering subsystem\n"
            "  model-loading      Test model loading (FBX/OBJ/GLTF)\n"
            "  serialization      Test serialization system\n"
            "  physics            Test physics subsystem\n"
            "  audio              Test audio subsystem\n"
            "  scene              Test scene/actor system\n"
            "  list               List available tests\n"
            "\n"
            "Options:\n"
            "  --help, -h         Show this help message\n"
            "  --verbose, -v      Enable verbose output\n"
            "  --file=<path>      Specify file path for model-loading test\n"
            "  --iterations=<n>   Number of test iterations (default: 1)\n"
            "  --width=<n>        Window width (default: 800)\n"
            "  --height=<n>       Window height (default: 600)\n"
            "\n"
            "Examples:\n"
            "  CLITestRunner.exe all\n"
            "  CLITestRunner.exe rendering --verbose\n"
            "  CLITestRunner.exe model-loading --file=model.obj\n"
            "  CLITestRunner.exe serialization --iterations=3\n";
    }

    inline std::string GetEditorHelpText()
    {
        return
            "KojeomEngine Editor\n"
            "===================\n"
            "\n"
            "Usage: KojeomEditor.exe [options]\n"
            "\n"
            "Options:\n"
            "  --help, -h         Show this help message\n"
            "  --version          Show version information\n"
            "  --check-modules    Verify all engine modules load correctly\n"
            "  --test             Run editor integration test and exit\n"
            "\n"
            "Examples:\n"
            "  KojeomEditor.exe\n"
            "  KojeomEditor.exe --check-modules\n"
            "  KojeomEditor.exe --test\n";
    }
}

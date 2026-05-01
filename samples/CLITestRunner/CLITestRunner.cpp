#include "../../Engine/Utils/Common.h"
#include "../../Engine/Utils/CLI.h"
#include "../../Engine/Core/Engine.h"
#include "CLITestRunnerTests.h"

int main(int argc, char* argv[])
{
    KEngine::SetupDebugEnvironment();

    std::string CmdLine;
    for (int i = 1; i < argc; ++i)
    {
        if (i > 1) CmdLine += " ";
        CmdLine += argv[i];
    }

    CLIUtils::FParsedArgs Args = CLIUtils::ParseCommandLine(CmdLine);

    if (Args.bHelpRequested)
    {
        std::cout << CLIUtils::GetHelpText() << std::endl;
        KEngine::CleanupDebugEnvironment();
        return 0;
    }

    CLITest::FTestRunner Runner;
    Runner.AddTest("rendering", CLITest::TestRenderingSubsystem);
    Runner.AddTest("serialization", CLITest::TestSerializationSubsystem);
    Runner.AddTest("physics", CLITest::TestPhysicsSubsystem);
    Runner.AddTest("audio", CLITest::TestAudioSubsystem);
    Runner.AddTest("scene", CLITest::TestSceneSubsystem);
    Runner.AddTest("model-loading", CLITest::TestModelLoadingSubsystem);
    Runner.AddTest("subsystem-registry", CLITest::TestSubsystemRegistry);
    Runner.AddTest("animation", CLITest::TestAnimationSubsystem);
    Runner.AddTest("path-utils", CLITest::TestPathUtils);
    Runner.AddTest("cli-parser", CLITest::TestCLIParser);
    Runner.AddTest("render-modules", CLITest::TestRenderModuleRegistry);
    Runner.AddTest("input", CLITest::TestInputSystem);
    Runner.AddTest("blend-tree", CLITest::TestBlendTree);
    Runner.AddTest("root-motion", CLITest::TestRootMotion);
    Runner.AddTest("animation-events", CLITest::TestAnimationEvents);
    Runner.AddTest("smoke", CLITest::TestSmokeMode);
    Runner.AddTest("validate-assets", CLITest::TestValidateAssets);
    Runner.AddTest("scene-dump", CLITest::TestSceneDump);

    std::string ResultJsonPath = CLIUtils::GetOption(Args, "result-json");
    if (!ResultJsonPath.empty())
    {
        Runner.SetResultJsonPath(ResultJsonPath);
    }

    std::string Command = Args.Command;
    if (Command.empty())
    {
        if (CLIUtils::HasOption(Args, "list"))
        {
            Runner.ListTests();
        }
        else
        {
            std::cout << CLIUtils::GetHelpText() << std::endl;
        }
        KEngine::CleanupDebugEnvironment();
        return 0;
    }

    int32 ExitCode = 0;

    if (Command == "all")
    {
        ExitCode = Runner.RunAll();
    }
    else if (Command == "list")
    {
        Runner.ListTests();
    }
    else
    {
        ExitCode = Runner.RunTest(Command);
    }

    KEngine::CleanupDebugEnvironment();

    if (ExitCode > 0) return 5;
    return 0;
}

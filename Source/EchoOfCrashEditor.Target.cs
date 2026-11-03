using UnrealBuildTool;
using System.Collections.Generic;

public class EchoOfCrashEditorTarget : TargetRules
{
    public EchoOfCrashEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("EchoOfCrash");
    }
}

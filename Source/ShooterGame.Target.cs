// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ShooterGameTarget : TargetRules
{
    public ShooterGameTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        bUsesSteam = true;

		ExtraModuleNames.Add("ShooterGame");
    }
}
